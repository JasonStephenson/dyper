PUBLIC SupportCheckIsAMD
PUBLIC SupportCheckMSR
PUBLIC SupportCheckCanEnableSVM
PUBLIC SupportCheckHasSLAT
PUBLIC SupportCheckHasVmcbClean

.code _text

; Look for "AuthenticAMD" vendor string
SupportCheckIsAMD PROC
    push rbx                        ; Preserve non volatile ebx affected by cpuid

    xor eax, eax
    cpuid
    
    cmp ebx, 68747541h              ; "Auth"
    jne notamd
    cmp ecx, 444D4163h              ; "cAMD"
    jne notamd
    cmp edx, 69746E65h              ; "enti"
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
    push rbx                        ; Preserve non volatile ebx affected by cpuid
    
    mov eax, 0001h
    cpuid
    test edx, 20h                   ; Bit 5 (MSR)
    jnz supported

    mov eax, 80000001h
    cpuid
    test edx, 20h                   ; Bit 5 (MSR)
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
; Implementation from Section 15.4 of Vol 2
SupportCheckCanEnableSVM PROC
    push rbx                        ; Preserve non volatile ebx affected by cpuid

    mov eax, 80000001h
    cpuid

    test ecx, 4h                    ; Bit 2 (SVM)
    jz not_supported

    mov ecx, 0C0010114h             ; VM_CR
    rdmsr
    test eax, 10h                   ; Bit 4 (SVMDIS)
    jz supported

    mov eax, 8000000Ah
    cpuid
    test edx, 4h                    ; Bit 2 (SVML)
    jz not_supported                ; Disabled at BIOS, not unlockable

                                    ; May be unlockable. Fallthrough to not_supported
not_supported:
    xor eax, eax
    pop rbx
    ret

supported:
    mov eax, 1
    pop rbx
    ret
SupportCheckCanEnableSVM ENDP

SupportCheckHasSLAT PROC
    push rbx                        ; Preserve non volatile ebx affected by cpuid

    mov ecx, 8000000Ah
    cpuid

    xor eax, eax
    test edx, 1                     ; Bit 0 (NP)
    setnz al

    pop rbx
    ret

SupportCheckHasSLAT ENDP

SupportCheckHasVmcbClean PROC
    push rbx                        ; Preserve non volatile ebx affected by cpuid

    mov ecx, 8000000Ah
    cpuid

    xor eax, eax
    test edx, 20h                   ; Bit 5 (VmcbClean)
    setnz al

    pop rbx
    ret

SupportCheckHasVmcbClean ENDP

END
