// emerging.cpp - Emerging 语言编译器
// 将 .emg 文件编译为机器码，生成 .obj 文件（COFF格式）
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <cstdint>
#include <algorithm>
using namespace std;

// ==================== COFF 结构定义 ====================
#pragma pack(push, 1)
struct COFF_FileHeader {
    uint16_t Machine;          // 机器类型: 0x14C for i386
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
};

struct COFF_SectionHeader {
    char Name[8];              // 节名称
    uint32_t VirtualSize;       // 虚拟大小
    uint32_t VirtualAddress;    // 虚拟地址
    uint32_t SizeOfRawData;     // 原始数据大小
    uint32_t PointerToRawData;  // 指向原始数据的指针
    uint32_t PointerToRelocations;
    uint32_t PointerToLineNumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLineNumbers;
    uint32_t Characteristics;   // 属性
};

struct COFF_Symbol {
    union {
        char ShortName[8];
        struct {
            uint32_t Zeroes;
            uint32_t Offset;
        } Name;
    } N;
    uint32_t Value;
    uint16_t SectionNumber;
    uint16_t Type;
    uint8_t StorageClass;
    uint8_t NumberOfAuxSymbols;
};

struct COFF_Relocation {
    uint32_t VirtualAddress;
    uint32_t SymbolTableIndex;
    uint16_t Type;              // 0x14: DIR32, 0x4: REL32
};
#pragma pack(pop)

// ==================== 词法分析 ====================
enum TokenType {
    TOK_EOF, TOK_IDENT, TOK_NUMBER, TOK_STRING,
    TOK_USE, TOK_INT, TOK_VOID, TOK_PRINT, TOK_PRINTLN,
    TOK_RET, TOK_LBRACE, TOK_RBRACE, TOK_LPAREN, TOK_RPAREN,
    TOK_SEMICOLON, TOK_END, TOK_COMMA, TOK_LT, TOK_GT,
    TOK_STRING_LITERAL
};

struct Token {
    TokenType type;
    string text;
    int line;
};

class Lexer {
    string input;
    size_t pos;
    int line;
public:
    Lexer(const string& src) : input(src), pos(0), line(1) {}

    Token nextToken() {
        while (pos < input.size() && isspace(input[pos])) {
            if (input[pos] == '\n') line++;
            pos++;
        }
        if (pos >= input.size()) return {TOK_EOF, "", line};

        char c = input[pos];
        if (isalpha(c) || c == '_') {
            size_t start = pos;
            while (pos < input.size() && (isalnum(input[pos]) || input[pos] == '_')) pos++;
            string word = input.substr(start, pos - start);
            TokenType type = TOK_IDENT;
            if (word == "use") type = TOK_USE;
            else if (word == "int") type = TOK_INT;
            else if (word == "void") type = TOK_VOID;
            else if (word == "print") type = TOK_PRINT;
            else if (word == "println") type = TOK_PRINTLN;
            else if (word == "ret") type = TOK_RET;
            else if (word == "end") type = TOK_END;
            return {type, word, line};
        }
        if (c == '"') {
            pos++;
            size_t start = pos;
            while (pos < input.size() && input[pos] != '"') pos++;
            string str = input.substr(start, pos - start);
            pos++;
            return {TOK_STRING_LITERAL, str, line};
        }
        if (isdigit(c)) {
            size_t start = pos;
            while (pos < input.size() && isdigit(input[pos])) pos++;
            return {TOK_NUMBER, input.substr(start, pos - start), line};
        }
        pos++;
        switch (c) {
            case '{': return {TOK_LBRACE, "{", line};
            case '}': return {TOK_RBRACE, "}", line};
            case '(': return {TOK_LPAREN, "(", line};
            case ')': return {TOK_RPAREN, ")", line};
            case ';': return {TOK_SEMICOLON, ";", line};
            case ',': return {TOK_COMMA, ",", line};
            case '<': return {TOK_LT, "<", line};
            case '>': return {TOK_GT, ">", line};
            default: return {TOK_EOF, "", line};
        }
    }
};

// ==================== 代码生成 ====================
class CodeGenerator {
    vector<uint8_t> textSection;      // 代码段
    vector<uint8_t> dataSection;      // 数据段
    vector<COFF_Relocation> relocations;
    
    // 符号表
    struct Symbol {
        string name;
        uint32_t value;
        uint16_t section;
        uint8_t storageClass;
        bool isExternal;
    };
    vector<Symbol> symbols;
    map<string, int> symbolIndex;     // 名称到符号表索引
    
    // 字符串常量
    struct StringConstant {
        string str;
        uint32_t offset;               // 在数据段中的偏移
    };
    vector<StringConstant> strings;
    
    // 当前函数信息
    string currentFunction;
    uint32_t currentFunctionStart;
    
public:
    CodeGenerator() {
        // 添加预定义符号：print和println在运行时库中
        addExternalSymbol("_print");
        addExternalSymbol("_println");
    }
    
    int addExternalSymbol(const string& name) {
        if (symbolIndex.count(name)) return symbolIndex[name];
        
        Symbol sym;
        sym.name = name;
        sym.value = 0;
        sym.section = 0;  // 外部符号，段号为0表示未定义
        sym.storageClass = 0x02;  // IMAGE_SYM_CLASS_EXTERNAL
        sym.isExternal = true;
        
        int index = symbols.size();
        symbols.push_back(sym);
        symbolIndex[name] = index;
        return index;
    }
    
    int addLocalSymbol(const string& name, uint32_t value, uint16_t section) {
        Symbol sym;
        sym.name = name;
        sym.value = value;
        sym.section = section;
        sym.storageClass = 0x03;  // IMAGE_SYM_CLASS_STATIC
        sym.isExternal = false;
        
        int index = symbols.size();
        symbols.push_back(sym);
        symbolIndex[name] = index;
        return index;
    }
    
    void beginFunction(const string& name) {
        currentFunction = "_" + name;
        currentFunctionStart = textSection.size();
        
        // 添加函数符号
        addLocalSymbol(currentFunction, currentFunctionStart, 1);  // section 1 = .text
        
        // 函数序言
        emitByte(0x55);                          // push ebp
        emitByte(0x89); emitByte(0xE5);          // mov ebp, esp
    }
    
    void endFunction() {
        // 如果没有显式的ret，添加默认返回
        if (textSection.size() == currentFunctionStart + 2) { // 只有push ebp; mov ebp, esp
            emitRet();
        }
    }
    
    void emitRet() {
        emitByte(0x31); emitByte(0xC0);          // xor eax, eax
        emitByte(0x5D);                           // pop ebp
        emitByte(0xC3);                           // ret
    }
    
    void emitPrintln(const string& str) {
        // 将字符串添加到数据段
        uint32_t strOffset = addString(str + "\n");
        
        // call _println
        emitByte(0xE8);  // call rel32
        
        // 添加重定位
        COFF_Relocation rel;
        rel.VirtualAddress = textSection.size();
        rel.SymbolTableIndex = symbolIndex["_println"];
        rel.Type = 0x04;  // IMAGE_REL_I386_REL32
        relocations.push_back(rel);
        
        // 占位4字节，稍后链接器填充
        emitDword(0);
    }
    
    void emitPrint(const string& str) {
        uint32_t strOffset = addString(str);
        
        // push offset string
        emitByte(0x68);  // push imm32
        
        COFF_Relocation rel;
        rel.VirtualAddress = textSection.size();
        rel.SymbolTableIndex = symbolIndex[str];  // 字符串标签
        rel.Type = 0x14;  // IMAGE_REL_I386_DIR32
        relocations.push_back(rel);
        
        emitDword(0);
        
        // call _print
        emitByte(0xE8);
        
        COFF_Relocation rel2;
        rel2.VirtualAddress = textSection.size();
        rel2.SymbolTableIndex = symbolIndex["_print"];
        rel2.Type = 0x04;  // IMAGE_REL_I386_REL32
        relocations.push_back(rel2);
        
        emitDword(0);
        
        // add esp, 4
        emitByte(0x83); emitByte(0xC4); emitByte(0x04);
    }
    
    uint32_t addString(const string& str) {
        // 检查是否已存在
        for (auto& s : strings) {
            if (s.str == str) return s.offset;
        }
        
        // 创建字符串标签
        string label = "_str" + to_string(strings.size());
        uint32_t offset = dataSection.size();
        
        // 添加字符串数据
        for (char c : str) {
            dataSection.push_back(c);
        }
        dataSection.push_back(0);  // null terminator
        
        // 添加到符号表
        addLocalSymbol(label, offset, 2);  // section 2 = .data
        
        strings.push_back({str, offset});
        return offset;
    }
    
    void emitByte(uint8_t b) {
        textSection.push_back(b);
    }
    
    void emitDword(uint32_t d) {
        textSection.push_back(d & 0xFF);
        textSection.push_back((d >> 8) & 0xFF);
        textSection.push_back((d >> 16) & 0xFF);
        textSection.push_back((d >> 24) & 0xFF);
    }
    
    void writeCOFF(const string& filename) {
        ofstream out(filename, ios::binary);
        
        // 文件头
        COFF_FileHeader header;
        header.Machine = 0x14C;  // i386
        header.NumberOfSections = 2;  // .text 和 .data
        header.TimeDateStamp = 0;
        header.PointerToSymbolTable = 0;  // 稍后填充
        header.NumberOfSymbols = symbols.size();
        header.SizeOfOptionalHeader = 0;
        header.Characteristics = 0;
        
        // 节头表起始位置
        uint32_t sectionTablePos = sizeof(COFF_FileHeader);
        
        // 计算各节位置
        uint32_t textRelocPos = sectionTablePos + 2 * sizeof(COFF_SectionHeader);
        uint32_t textDataPos = textRelocPos + relocations.size() * sizeof(COFF_Relocation);
        uint32_t dataDataPos = textDataPos + textSection.size();
        uint32_t symbolTablePos = dataDataPos + dataSection.size();
        
        header.PointerToSymbolTable = symbolTablePos;
        
        // 写入文件头
        out.write((char*)&header, sizeof(header));
        
        // .text 节头
        COFF_SectionHeader textHeader;
        memset(&textHeader, 0, sizeof(textHeader));
        memcpy(textHeader.Name, ".text", 5);
        textHeader.VirtualSize = textSection.size();
        textHeader.VirtualAddress = 0;
        textHeader.SizeOfRawData = textSection.size();
        textHeader.PointerToRawData = textDataPos;
        textHeader.PointerToRelocations = textRelocPos;
        textHeader.NumberOfRelocations = relocations.size();
        textHeader.Characteristics = 0x60000020;  // CODE | EXECUTE | READ
        
        // .data 节头
        COFF_SectionHeader dataHeader;
        memset(&dataHeader, 0, sizeof(dataHeader));
        memcpy(dataHeader.Name, ".data", 5);
        dataHeader.VirtualSize = dataSection.size();
        dataHeader.VirtualAddress = 0;
        dataHeader.SizeOfRawData = dataSection.size();
        dataHeader.PointerToRawData = dataDataPos;
        dataHeader.Characteristics = 0xC0000040;  // INITIALIZED_DATA | READ | WRITE
        
        out.write((char*)&textHeader, sizeof(textHeader));
        out.write((char*)&dataHeader, sizeof(dataHeader));
        
        // 写入重定位表
        for (auto& rel : relocations) {
            out.write((char*)&rel, sizeof(rel));
        }
        
        // 写入 .text 节数据
        out.write((char*)textSection.data(), textSection.size());
        
        // 写入 .data 节数据
        out.write((char*)dataSection.data(), dataSection.size());
        
        // 写入符号表
        for (auto& sym : symbols) {
            COFF_Symbol csym;
            memset(&csym, 0, sizeof(csym));
            
            if (sym.name.size() <= 8) {
                memcpy(csym.N.ShortName, sym.name.c_str(), sym.name.size());
            } else {
                csym.N.Name.Zeroes = 0;
                csym.N.Name.Offset = 0;  // 简单实现，不处理长名称
            }
            
            csym.Value = sym.value;
            csym.SectionNumber = sym.section;
            csym.Type = 0x20;  // 函数类型
            csym.StorageClass = sym.storageClass;
            
            out.write((char*)&csym, sizeof(csym));
        }
    }
};

// ==================== 语法分析器 ====================
class Parser {
    Lexer lexer;
    Token current;
    CodeGenerator& codeGen;
    bool hasMain;
    
    void error(const string& msg) {
        cerr << "Error at line " << current.line << ": " << msg << endl;
        exit(1);
    }
    
    void advance() {
        current = lexer.nextToken();
    }
    
    bool match(TokenType type) {
        if (current.type == type) {
            advance();
            return true;
        }
        return false;
    }
    
    void expect(TokenType type, const string& err) {
        if (!match(type)) error(err);
    }
    
    void parseUse() {
        if (match(TOK_LT)) {
            if (current.type != TOK_IDENT) error("expected header name");
            string header = current.text + ".emh";
            advance();
            expect(TOK_GT, "expected '>' after header");
            // 这里可以实际处理头文件包含，但简化实现中忽略
        } else if (current.type == TOK_STRING_LITERAL) {
            string header = current.text;
            advance();
            // 忽略用户头文件
        } else {
            error("expected <header> or \"header\" after use");
        }
        if (current.type == TOK_SEMICOLON) advance();
    }
    
    void parseFunction() {
        bool isInt = match(TOK_INT);
        bool isVoid = match(TOK_VOID);
        if (!isInt && !isVoid) error("expected return type (int/void)");
        
        if (current.type != TOK_IDENT) error("expected function name");
        string funcName = current.text;
        advance();
        
        if (funcName == "main") hasMain = true;
        
        expect(TOK_LPAREN, "expected '(' after function name");
        // 解析参数列表（忽略）
        while (current.type != TOK_RPAREN) {
            if (current.type == TOK_IDENT) advance();
            else if (current.type == TOK_COMMA) advance();
            else error("expected parameter or ')'");
        }
        expect(TOK_RPAREN, "expected ')' after parameters");
        expect(TOK_LBRACE, "expected '{' to start function body");
        
        // 开始生成函数代码
        codeGen.beginFunction(funcName);
        
        // 解析语句直到 '}'
        while (current.type != TOK_RBRACE && current.type != TOK_EOF) {
            parseStatement();
        }
        
        codeGen.endFunction();
        
        expect(TOK_RBRACE, "expected '}' at end of function");
    }
    
    void parseStatement() {
        if (match(TOK_PRINTLN)) {
            expect(TOK_LPAREN, "expected '(' after println");
            if (current.type != TOK_STRING_LITERAL) error("expected string literal");
            string str = current.text;
            advance();
            expect(TOK_RPAREN, "expected ')' after string");
            
            codeGen.emitPrintln(str);
            
            // 可选 end
            if (current.type == TOK_END) advance();
            if (current.type == TOK_SEMICOLON) advance();
        }
        else if (match(TOK_PRINT)) {
            expect(TOK_LPAREN, "expected '(' after print");
            if (current.type != TOK_STRING_LITERAL) error("expected string literal");
            string str = current.text;
            advance();
            expect(TOK_RPAREN, "expected ')' after string");
            
            codeGen.emitPrint(str);
            
            if (current.type == TOK_END) advance();
            if (current.type == TOK_SEMICOLON) advance();
        }
        else if (match(TOK_RET)) {
            codeGen.emitRet();
            if (current.type == TOK_SEMICOLON) advance();
        }
        else {
            // 跳过未知语句
            advance();
        }
    }
    
public:
    Parser(Lexer& l, CodeGenerator& cg) : lexer(l), codeGen(cg), hasMain(false) {
        advance();
    }
    
    void parse() {
        while (current.type != TOK_EOF) {
            if (current.type == TOK_USE) {
                advance();
                parseUse();
            } else if (current.type == TOK_INT || current.type == TOK_VOID) {
                parseFunction();
            } else {
                error("unexpected token");
            }
        }
        
        if (!hasMain) {
            cerr << "Warning: no main function found" << endl;
        }
    }
};

// ==================== 主函数 ====================
int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: emerging input.emg output.obj" << endl;
        return 1;
    }
    
    // 读取源文件
    ifstream in(argv[1]);
    if (!in) {
        cerr << "Cannot open input file: " << argv[1] << endl;
        return 1;
    }
    
    string source((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    in.close();
    
    // 词法分析、语法分析、代码生成
    Lexer lexer(source);
    CodeGenerator codeGen;
    Parser parser(lexer, codeGen);
    
    try {
        parser.parse();
    } catch (const exception& e) {
        cerr << "Compilation failed: " << e.what() << endl;
        return 1;
    }
    
    // 生成COFF目标文件
    codeGen.writeCOFF(argv[2]);
    
    cout << "Compilation successful: " << argv[2] << endl;
    return 0;
}
