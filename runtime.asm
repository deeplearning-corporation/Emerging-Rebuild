; runtime.asm - Emerging Runtime Library
; Compile command: nasm -f win32 -o runtime.obj runtime.asm

global _print
global _println

section .data
    newline db 10, 0

section .text

; void print(char* str)
_print:
    push ebp
    mov ebp, esp
    
    ; 这里应该调用操作系统的输出函数
    ; 对于裸机系统，可能需要直接写显存或通过串口输出
    ; 这里简化为在调试器中查看
    
    pop ebp
    ret

; void println(char* str)
_println:
    push ebp
    mov ebp, esp
    
    ; 输出字符串后换行
    
    pop ebp
    ret