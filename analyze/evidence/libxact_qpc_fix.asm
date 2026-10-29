; Replacement for libxact.dll RVA 410A..4135, SHA256 6cee...fb9e.
; Live inputs: [rsp+20h]=QPC now, [rbx+28h]=cue QPC anchor,
; [rdi+0c488h]=QPC frequency, xmm9.low=float(1000).
; Live output: xmm6.low=float(elapsed milliseconds).
; No calls, stack changes, or writes to the original cue record.
.code
clock_fix PROC
    mov rax, qword ptr [rsp+20h]
    sub rax, qword ptr [rbx+28h]
    cvtsi2sd xmm6, rax
    cvtsi2sd xmm0, qword ptr [rdi+0c488h]
    cvtss2sd xmm1, xmm9
    mulsd xmm6, xmm1
    divsd xmm6, xmm0
    cvtsd2ss xmm6, xmm6
    nop
    nop
    nop
    nop
clock_fix ENDP
END
