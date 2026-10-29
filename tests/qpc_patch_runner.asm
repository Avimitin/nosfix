; Execute a copy of only the original/replacement arithmetic block. No game
; function, cue, imported API, or audio device is invoked by this harness.
.code
PUBLIC RunQpcBlock
RunQpcBlock PROC FRAME
    push rbx
    .pushreg rbx
    push rdi
    .pushreg rdi
    sub rsp, 68h
    .allocstack 68h
    movaps xmmword ptr [rsp+30h], xmm6
    .savexmm128 xmm6, 30h
    movaps xmmword ptr [rsp+40h], xmm9
    .savexmm128 xmm9, 40h
    .endprolog
    ; inputs: now, cue-start, frequency (three consecutive int64s).
    mov rax, qword ptr [rdx]
    mov qword ptr [rsp+18h], rax ; becomes [rsp+20h] after call's return address
    lea rbx, [rdx-20h]           ; [rbx+28h] is inputs.start
    lea rdi, [rdx-0c478h]        ; [rdi+0c488h] is inputs.frequency
    mov eax, 447a0000h           ; float(1000)
    movd xmm9, eax
    call rcx
    movaps xmm0, xmm6
    movaps xmm6, xmmword ptr [rsp+30h]
    movaps xmm9, xmmword ptr [rsp+40h]
    add rsp, 68h
    pop rdi
    pop rbx
    ret
RunQpcBlock ENDP
END
