.data
    max_value: dword 10
.code
    push 0
    pop  eax
BEGIN:
    push dword [max_value]
    push eax
    cmp
    ja END
    push eax
    push eax
    mul
    push eax
    mul
    out
    push 1
    push eax
    add
    pop eax
    jmp BEGIN
END:
    stop
