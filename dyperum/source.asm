; source.asm - x64 Windows, MS x64 calling convention
section .text
global cpuid_call

; void cpuid_call(uint32_t eax_in, uint32_t ecx_in, uint32_t* out);
; RCX = eax_in
; RDX = ecx_in
; R8  = out (pointer to 4 uint32_t values)

cpuid_call:
    push rbx                  ; preserve non-volatile rbx
    push rdi                  ; preserve non-volatile rdi

    mov eax, ecx              ; eax_in -> eax
    mov ecx, edx              ; ecx_in -> ecx
    cpuid

    mov rdi, r8               ; out -> rdi
    mov [rdi], eax
    mov [rdi+4], ebx
    mov [rdi+8], ecx
    mov [rdi+12], edx

    pop rdi                   ; restore rdi
    pop rbx                   ; restore rbx
    ret
