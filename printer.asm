section .bss
    digitSpace resb 22     ; reserve space for a number
    digitSpacePos resb 8    ; reserver space for a pointer

section .text
    global _print_int

_print_int:
    mov rcx, digitSpace         ; move a pointer pointing to the memory space reserved for a number to rcx
    mov rbx, 10                 ; move a newline character to rbx
    mov [rcx], rbx              ; move a newline character to the number memory space
    inc rcx                     ; increment a digitSpace pointer
    mov [digitSpacePos], rcx    ; save the pointer to the variable

_printRAXLoop:
    mov rdx, 0                  ; divide a number in rax by 10
    mov rbx, 10                 ; |
    div rbx                     ; |
    push rax                    ; save it to the memory
    add rdx, 48                 ; add remainder and ascii '0' to get a digit character

    mov rcx, [digitSpacePos]    ; move pointer to the digitSpacePos to rcx
    mov [rcx], dl               ; move the ascii character to the pointer
    inc rcx                     ; increment the pointer
    mov [digitSpacePos], rcx    ; save the new pointer value

    pop rax                     ; restore the number divided by 10
    cmp rax, 0                  ; check if it's 0
    jne _printRAXLoop           ; if it's not - continue the loop

_printRAXLoop2:
    mov rcx, [digitSpacePos]    ; move the pointer to the rcx

    mov rax, 1                  ; print incruction
    mov rdi, 1                  ; |
    mov rsi, rcx                ; value to print
    mov rdx, 1                  ; len
    syscall

    mov rcx, [digitSpacePos]    ; move the pointer to the rcx
    dec rcx                     ; decrement the pointer
    mov [digitSpacePos], rcx    ; save the pointer

    cmp rcx, digitSpace         ; check if it's the end of the string
    jge _printRAXLoop2          ; if it's not - continue the loop

    ret                         ; ret from the call
