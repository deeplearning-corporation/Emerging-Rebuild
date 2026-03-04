// linker.cpp - Emerging 语言链接器
// 将多个 .obj 文件链接成平面二进制可执行文件
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <cstdint>
#include <cstring>
#include <algorithm>
using namespace std;

#pragma pack(push, 1)
struct COFF_FileHeader {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
};

struct COFF_SectionHeader {
    char Name[8];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLineNumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLineNumbers;
    uint32_t Characteristics;
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
    uint16_t Type;
};
#pragma pack(pop)

struct InputFile {
    string filename;
    vector<uint8_t> data;
    COFF_FileHeader header;
    vector<COFF_SectionHeader> sections;
    vector<COFF_Symbol> symbols;
    vector<string> symbolNames;
    vector<vector<COFF_Relocation>> relocations;
};

struct ResolvedSymbol {
    uint32_t value;        // 最终地址
    uint16_t section;       // 所在段 (1=text, 2=data)
    string objFile;         // 定义该符号的目标文件
    bool isDefined;
};

class Linker {
    vector<InputFile> objFiles;
    map<string, ResolvedSymbol> globalSymbols;
    vector<uint8_t> outputImage;
    uint32_t textBase;
    uint32_t dataBase;
    uint32_t entryPoint;
    
    void readCOFF(InputFile& obj) {
        const uint8_t* base = obj.data.data();
        
        // 读取文件头
        memcpy(&obj.header, base, sizeof(COFF_FileHeader));
        
        // 读取节头
        const uint8_t* sectionBase = base + sizeof(COFF_FileHeader);
        for (int i = 0; i < obj.header.NumberOfSections; i++) {
            COFF_SectionHeader sh;
            memcpy(&sh, sectionBase + i * sizeof(COFF_SectionHeader), sizeof(COFF_SectionHeader));
            obj.sections.push_back(sh);
        }
        
        // 读取符号表
        const uint8_t* symBase = base + obj.header.PointerToSymbolTable;
        for (int i = 0; i < obj.header.NumberOfSymbols; i++) {
            COFF_Symbol sym;
            memcpy(&sym, symBase + i * sizeof(COFF_Symbol), sizeof(COFF_Symbol));
            obj.symbols.push_back(sym);
            
            // 解析符号名称
            if (sym.N.ShortName[0] != 0) {
                char name[9] = {0};
                memcpy(name, sym.N.ShortName, 8);
                obj.symbolNames.push_back(name);
            } else {
                // 长名称在字符串表中，简化实现中跳过
                obj.symbolNames.push_back("(long)");
            }
        }
        
        // 读取重定位表
        for (int i = 0; i < obj.header.NumberOfSections; i++) {
            auto& sh = obj.sections[i];
            vector<COFF_Relocation> rels;
            
            if (sh.NumberOfRelocations > 0) {
                const uint8_t* relBase = base + sh.PointerToRelocations;
                for (int j = 0; j < sh.NumberOfRelocations; j++) {
                    COFF_Relocation rel;
                    memcpy(&rel, relBase + j * sizeof(COFF_Relocation), sizeof(COFF_Relocation));
                    rels.push_back(rel);
                }
            }
            obj.relocations.push_back(rels);
        }
    }
    
    void collectSymbols() {
        // 第一遍：收集所有定义的符号
        for (auto& obj : objFiles) {
            for (int i = 0; i < obj.symbols.size(); i++) {
                auto& sym = obj.symbols[i];
                string name = obj.symbolNames[i];
                
                if (sym.SectionNumber > 0) {  // 定义的符号
                    if (globalSymbols.count(name)) {
                        cerr << "Error: duplicate symbol '" << name << "'" << endl;
                        exit(1);
                    }
                    
                    ResolvedSymbol rs;
                    rs.value = sym.Value;
                    rs.section = sym.SectionNumber;
                    rs.objFile = obj.filename;
                    rs.isDefined = true;
                    globalSymbols[name] = rs;
                }
            }
        }
        
        // 第二遍：检查未定义的符号
        for (auto& obj : objFiles) {
            for (int i = 0; i < obj.symbols.size(); i++) {
                auto& sym = obj.symbols[i];
                string name = obj.symbolNames[i];
                
                if (sym.SectionNumber == 0 && name != "") {  // 外部符号
                    if (!globalSymbols.count(name)) {
                        cerr << "Error: undefined symbol '" << name << "'" << endl;
                        exit(1);
                    }
                }
            }
        }
    }
    
    void layout() {
        // 计算各段基址
        textBase = 0;
        dataBase = 0;
        
        // 计算每个段的总大小
        uint32_t textSize = 0;
        uint32_t dataSize = 0;
        
        for (auto& obj : objFiles) {
            for (int i = 0; i < obj.sections.size(); i++) {
                auto& sh = obj.sections[i];
                if (strncmp(sh.Name, ".text", 5) == 0) {
                    textSize += sh.SizeOfRawData;
                } else if (strncmp(sh.Name, ".data", 5) == 0) {
                    dataSize += sh.SizeOfRawData;
                }
            }
        }
        
        dataBase = textSize;  // 数据段紧跟代码段
        
        // 调整符号地址
        for (auto& obj : objFiles) {
            uint32_t textOffset = 0;
            uint32_t dataOffset = 0;
            
            for (int i = 0; i < obj.sections.size(); i++) {
                auto& sh = obj.sections[i];
                if (strncmp(sh.Name, ".text", 5) == 0) {
                    for (auto& sym : obj.symbols) {
                        if (sym.SectionNumber == i + 1) {
                            sym.Value += textBase + textOffset;
                        }
                    }
                    textOffset += sh.SizeOfRawData;
                } else if (strncmp(sh.Name, ".data", 5) == 0) {
                    for (auto& sym : obj.symbols) {
                        if (sym.SectionNumber == i + 1) {
                            sym.Value += dataBase + dataOffset;
                        }
                    }
                    dataOffset += sh.SizeOfRawData;
                }
            }
        }
    }
    
    void mergeSections() {
        outputImage.clear();
        
        // 合并所有 .text 节
        for (auto& obj : objFiles) {
            for (int i = 0; i < obj.sections.size(); i++) {
                auto& sh = obj.sections[i];
                if (strncmp(sh.Name, ".text", 5) == 0) {
                    uint32_t oldSize = outputImage.size();
                    outputImage.resize(oldSize + sh.SizeOfRawData);
                    
                    const uint8_t* data = obj.data.data() + sh.PointerToRawData;
                    memcpy(outputImage.data() + oldSize, data, sh.SizeOfRawData);
                    
                    // 应用重定位
                    applyRelocations(obj, i, oldSize);
                }
            }
        }
        
        // 合并所有 .data 节
        for (auto& obj : objFiles) {
            for (int i = 0; i < obj.sections.size(); i++) {
                auto& sh = obj.sections[i];
                if (strncmp(sh.Name, ".data", 5) == 0) {
                    uint32_t oldSize = outputImage.size();
                    outputImage.resize(oldSize + sh.SizeOfRawData);
                    
                    const uint8_t* data = obj.data.data() + sh.PointerToRawData;
                    memcpy(outputImage.data() + oldSize, data, sh.SizeOfRawData);
                    
                    // 数据段也可能有重定位（如字符串地址）
                    applyRelocations(obj, i, oldSize);
                }
            }
        }
    }
    
    void applyRelocations(InputFile& obj, int sectionIdx, uint32_t baseOffset) {
        auto& sh = obj.sections[sectionIdx];
        auto& rels = obj.relocations[sectionIdx];
        
        for (auto& rel : rels) {
            uint32_t* patchAddr = (uint32_t*)(outputImage.data() + baseOffset + rel.VirtualAddress);
            auto& sym = obj.symbols[rel.SymbolTableIndex];
            string symName = obj.symbolNames[rel.SymbolTableIndex];
            
            uint32_t symAddr = sym.Value;
            
            if (rel.Type == 0x14) {  // DIR32 - 绝对地址
                *patchAddr = symAddr;
            } else if (rel.Type == 0x04) {  // REL32 - 相对地址
                uint32_t callAddr = baseOffset + rel.VirtualAddress + 4;  // call 指令的下一条指令地址
                *patchAddr = symAddr - callAddr;
            }
            
            // 检查是否是入口点 (_main)
            if (symName == "_main") {
                entryPoint = symAddr;
            }
        }
    }
    
public:
    bool link(const vector<string>& objFilesList, const string& outputFile) {
        // 读取所有目标文件
        for (const auto& f : objFilesList) {
            InputFile obj;
            obj.filename = f;
            
            ifstream in(f, ios::binary);
            if (!in) {
                cerr << "Cannot open file: " << f << endl;
                return false;
            }
            
            obj.data.assign((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
            in.close();
            
            readCOFF(obj);
            objFiles.push_back(obj);
        }
        
        // 收集全局符号
        collectSymbols();
        
        // 布局
        layout();
        
        // 合并段
        mergeSections();
        
        // 写入输出文件
        ofstream out(outputFile, ios::binary);
        if (!out) {
            cerr << "Cannot create output file: " << outputFile << endl;
            return false;
        }
        
        // 写入4字节入口点地址
        out.write((char*)&entryPoint, 4);
        
        // 写入合并后的代码和数据
        out.write((char*)outputImage.data(), outputImage.size());
        
        out.close();
        
        cout << "Linking successful: " << outputFile << endl;
        cout << "Entry point: 0x" << hex << entryPoint << dec << endl;
        cout << "Total size: " << outputImage.size() << " bytes" << endl;
        
        return true;
    }
};

// ==================== 主函数 ====================
int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: linker output.exe input1.obj [input2.obj ...]" << endl;
        return 1;
    }
    
    string outputFile = argv[1];
    vector<string> inputFiles;
    for (int i = 2; i < argc; i++) {
        inputFiles.push_back(argv[i]);
    }
    
    Linker linker;
    if (!linker.link(inputFiles, outputFile)) {
        return 1;
    }
    
    return 0;
}
