; runtime_linux.asm - Linux 运行时库
; 汇编: nasm -f elf32 -o ../obj/runtime_linux.obj runtime_linux.asm

global _print
global _println
global _print_int
global _print_char
global _malloc
global _free
global _exit

section .data
    newline db 10, 0
    heap_start dd 0
    heap_end dd 0

section .text

; 系统调用宏
%define SYS_WRITE 4
%define SYS_EXIT 1
%define SYS_BRK 45

; void print(char* str)
_print:
    push ebp
    mov ebp, esp
    
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
    
    ; 系统调用 write
    mov eax, SYS_WRITE
    mov ebx, 1          ; stdout
    mov edx, ecx        ; 长度
    mov ecx, esi        ; 字符串指针
    int 0x80
    
    pop esi
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
    
    ; 初始化堆
    cmp dword [heap_start], 0
    jnz .alloc
    
    mov eax, SYS_BRK
    xor ebx, ebx
    int 0x80
    mov [heap_start], eax
    mov [heap_end], eax
    
.alloc:
    ; 对齐到8字节
    mov eax, [ebp + 8]
    add eax, 7
    and eax, 0xFFFFFFF8
    
    ; 分配内存
    mov ebx, [heap_end]
    mov [heap_end], eax
    add [heap_end], ebx
    
    ; 扩展堆
    mov eax, SYS_BRK
    mov ebx, [heap_end]
    int 0x80
    
    mov eax, ebx
    pop ebp
    ret

; void free(void* ptr)
_free:
    ; Linux简单实现，不释放内存
    ret

; void exit(int status)
_exit:
    push ebp
    mov ebp, esp
    
    mov eax, SYS_EXIT
    mov ebx, [ebp + 8]
    int 0x80
    
    ; never reached
    pop ebp
    ret