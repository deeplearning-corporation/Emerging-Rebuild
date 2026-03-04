; runtime_win.asm - Windows 运行时库
; 使用直接系统调用（syscall）方式
; 汇编: nasm -f win32 -o runtime_win.obj runtime_win.asm

global _print
global _println
global _print_int
global _print_char
global _malloc
global _free
global _exit

section .data
    newline db 13, 10, 0  ; Windows换行是 CRLF

section .text

; Windows系统调用号 (通过 int 0x2e 或 sysenter)
; 这里使用更简单的方式：直接调用内核32.dll中的函数
; 但需要手动解析导入表，比较复杂

; 简化的实现：使用内联汇编调用Windows API
; 注意：这种方法需要在链接时链接 kernel32.lib

; void print(char* str)
_print:
    push ebp
    mov ebp, esp
    sub esp, 4
    
    ; 获取字符串长度
    push esi
    mov esi, [ebp + 8]
    xor ecx, ecx
.count_loop:
    cmp byte [esi + ecx], 0
    jz .count_done
    inc ecx
    jmp .count_loop
.count_done:
    
    ; 调用 WriteConsole
    push 0                  ; lpReserved
    lea eax, [ebp - 4]      ; lpNumberOfCharsWritten
    push eax
    push ecx                ; nNumberOfCharsToWrite
    push esi                ; lpBuffer
    push -11                ; STD_OUTPUT_HANDLE
    call _GetStdHandle@4
    push eax                ; hConsoleOutput
    call _WriteConsoleA@20
    
    pop esi
    mov esp, ebp
    pop ebp
    ret

; void println(char* str)
_println:
    push ebp
    mov ebp, esp
    
    ; 输出原字符串
    push dword [ebp + 8]
    call _print
    add esp, 4
    
    ; 输出换行
    push newline
    call _print
    add esp, 4
    
    pop ebp
    ret

; void print_int(int num)
_print_int:
    push ebp
    mov ebp, esp
    sub esp, 16
    
    mov eax, [ebp + 8]
    mov ecx, 10
    lea edi, [ebp - 1]
    mov byte [edi], 0
    
    test eax, eax
    jnz .convert
    mov byte [edi - 1], '0'
    dec edi
    jmp .print
    
.convert:
    mov ebx, eax
    test ebx, ebx
    jns .positive
    neg ebx
.positive:
    mov eax, ebx
.convert_loop:
    xor edx, edx
    div ecx
    add dl, '0'
    dec edi
    mov [edi], dl
    test eax, eax
    jnz .convert_loop
    
    test dword [ebp + 8], 0x80000000
    jz .print
    dec edi
    mov byte [edi], '-'
    
.print:
    push edi
    call _print
    add esp, 4
    
    mov esp, ebp
    pop ebp
    ret

; void print_char(char c)
_print_char:
    push ebp
    mov ebp, esp
    
    mov eax, [ebp + 8]
    push eax
    call _print_int
    add esp, 4
    
    pop ebp
    ret

; void* malloc(int size)
_malloc:
    push ebp
    mov ebp, esp
    
    push dword [ebp + 8]    ; dwBytes
    push 8                  ; dwFlags (HEAP_ZERO_MEMORY)
    call _GetProcessHeap@0
    push eax                ; hHeap
    call _HeapAlloc@12
    
    pop ebp
    ret

; void free(void* ptr)
_free:
    push ebp
    mov ebp, esp
    
    push dword [ebp + 8]    ; lpMem
    push 0                  ; dwFlags
    call _GetProcessHeap@0
    push eax                ; hHeap
    call _HeapFree@12
    
    pop ebp
    ret

; void exit(int status)
_exit:
    push ebp
    mov ebp, esp
    
    push dword [ebp + 8]    ; uExitCode
    call _ExitProcess@4
    
    ; never reached
    pop ebp
    ret

; 导入Windows API
extern _GetStdHandle@4
extern _WriteConsoleA@20
extern _GetProcessHeap@0
extern _HeapAlloc@12
extern _HeapFree@12
extern _ExitProcess@4