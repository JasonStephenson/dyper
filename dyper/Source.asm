PUBLIC EnableSvme
.code _text

EnableSvme PROC PUBLIC

    xor rax, rax
    xor rdx, rdx
    mov ecx, 0C0000080h   ; EFER
    rdmsr                 ; Page 438

    or eax, 1000h         ; Bit 12
    wrmsr                 ; Page 511
    
    ret
EnableSvme ENDP

END
