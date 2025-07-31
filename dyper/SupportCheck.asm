PUBLIC SupportCheckIsAMD
PUBLIC SupportCheckMSR
PUBLIC SupportCheckSVM

.code _text

; Look for "AuthenticAMD" vendor string
SupportCheckIsAMD PROC
    push rbx ; preserve non volatile ebx affected by cpuid

    xor eax, eax
    cpuid
    
    cmp ebx, 68747541h ; "Auth"
    jne notamd
    cmp ecx, 444D4163h ; "cAMD"
    jne notamd
    cmp edx, 69746E65h ; "enti"
    jne notamd

    mov eax, 1
    pop rbx
    ret

notamd:
    xor eax, eax
    pop rbx
    ret
SupportCheckIsAMD ENDP

; Support for the RDMSR/WRMSR instructions are indicated by 
; CPUID Fn0000_0001_EDX[MSR] = 1 ORCPUID Fn8000_0001_EDX[MSR] = 1
SupportCheckMSR PROC
    push rbx ; preserve non volatile ebx affected by cpuid
    
    mov eax, 0001h
    cpuid
    test edx, 20h ; Bit 5 (MSR)
    jnz supported

    mov eax, 80000001h
    cpuid
    test edx, 20h ; Bit 5 (MSR)
    jnz supported

    xor eax, eax
    pop rbx
    ret

supported:
    mov eax, 1
    pop rbx
    ret
SupportCheckMSR ENDP

; Check CPU supports SVM instructions
SupportCheckSVM PROC
    push rbx ; preserve non volatile ebx affected by cpuid

    mov eax, 80000001h
    cpuid

    test ecx, 4h ; Bit 2(SVM)
    jnz supported

    xor eax, eax
    pop rbx
    ret

supported:
    mov eax, 1
    pop rbx
    ret
SupportCheckSVM ENDP

END
