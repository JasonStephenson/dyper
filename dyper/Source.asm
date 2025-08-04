PUBLIC EnableSvme
PUBLIC DisableSvme
PUBLIC SetHSave
.code _text

EnableSvme PROC PUBLIC

    xor rax, rax
    xor rdx, rdx
    mov ecx, 0C0000080h     ; EFER
    rdmsr                   ; Page 438

    or eax, 1000h           ; Set Bit 12
    wrmsr                   ; Page 511
    
    ret
EnableSvme ENDP

DisableSvme PROC PUBLIC

    xor rax, rax
    xor rdx, rdx
    mov ecx, 0C0000080h     ; EFER
    rdmsr                   ; Page 438

    and eax, 0FFFFEFFFh     ; Unset Bit 12
    wrmsr                   ; Page 511
    
    ret
DisableSvme ENDP

SetHSave PROC PUBLIC

    mov rax, rcx
    mov rdx, rcx
    shr rdx, 32             ; Keep highest bits

    and eax, 0FFFFFFFFh     ; Mask off
    and edx, 0FFFFFFFFh

    mov ecx, 0C0010117h     ; VM_HSAVE_PA Vol2 Page 585
    wrmsr

    ret

SetHSave ENDP

END
