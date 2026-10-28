    	PUBLIC _spoofer_stub
     
    .code
     
    _spoofer_stub PROC
        pop r11
        pop r10
        mov rax, [rsp + 24]
        
        mov r10, [rax]
        mov [rsp], r10
        
        mov r10, [rax + 8]
        mov [rax + 8], r11
     
        mov [rax + 16], rbx
        lea rbx, fixup
        mov [rax], rbx
        mov rbx, rax
        
        jmp r10
     
    fixup:
        sub rsp, 16
        mov rcx, rbx
        mov rbx, [rcx + 16]
        jmp QWORD PTR [rcx + 8]
    _spoofer_stub ENDP
    
    NtProtectVirtualMemory PROC
    mov r10, rcx                                                ; Move first parameter from rcx to r10 (syscall convention)
    mov eax, 50h                                                ; Set syscall number for NtProtectVirtualMemory (Windows 10 x64)
    test byte ptr [7FFE0308h], 1                                ; Check KUSER_SHARED_DATA flag
    jnz use_int2e                                               ; If flag is set, use int 2Eh
    syscall                                                     ; Execute 64-bit system call
    ret                                                         ; Return with NTSTATUS in eax
    use_int2e:
    int 2Eh                                                     ; Execute 32-bit system call (compatibility)
    ret                                                         ; Return with NTSTATUS in eax
    NtProtectVirtualMemory ENDP
     
    END