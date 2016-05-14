; QUADRATIC EQUATIONS SOLVER
;
; INPUT:   float a, b, c - the coefficients of the equation
; OUTPUT:  solution of the equation
; COMMENT: prints 0 if no solutions, -1 in case of infinite number of roots. Output is like M N K, where M = amount of roots, N, K - roots
.data
    descr: dword raw
    x1:    dword raw
    x2:    dword raw
    a:     dword raw
    b:     dword raw
    c:     dword raw
.code
BEGIN:
    fin ; Read a
    pop dword [a]; Store in eax
    fin ; Read b
    pop dword [b]; Store in ebx
    fin ; Read c
    pop dword [c]; Store in ecx
    call SOLVE_SQUARE
    jmp END

; Calculates sqrt of the eax and stores to ebx
SQRT:
    push 1.0 ; y = 1
    
    LOOP:
        pop ebx ; Get y
        push ebx ; y_0 = y
        dworddup ; y_0|y_0
        push eax ; x|y_0|y_0
        fdiv ; x/y_0|y_0
        fadd ; x/y_0 + y_0 
        push 0.5 ; 0.5 | x/y_0 + y_0
        fmul ; (x/y_0 + y_0)/2
        dworddup ; (x/y_0 + y_0)/2 | (x/y_0 + y_0)/2
        push ebx ; y | (x/y_0 + y_0)/2 | (x/y_0 + y_0)/2
        fsub ; y - (x/y_0 + y_0)/2 | (x/y_0 + y_0)/2
        fabs ; abs(y - (x/y_0 + y_0)/2)| (x/y_0 + y_0)/2
        push 0.000001
        fcmp
    jb LOOP
SQRT_END:
    pop ebx
    ret

; Solves square equation which coefficients a, b, c are in aex, abx, acx
SOLVE_SQUARE:
    push dword [a]
    fabs
    push 0.000001
    fcmp
    ja A_=_0
    call calcD
    push dword [descr]
    push 0
    fcmp
    ja NO_ROOTS
    push dword [descr]
    pop  eax
    call SQRT
    push ebx
    push 0.000001
    fcmp
    ja ONE_ROOT
    push 2 ; We have 2 roots now
    out
    call calcFirst
    call calcSecond
    push dword [x1]
    fout
    push dword [x2]
    fout
SOLVE_SQUARE_END:
    ret
    
; In case A = 0
A_=_0:
    push dword [b]
    fabs
    push 0.000001
    fcmp
    ja A_&_B_=_0 ; If B = 0
    push ebx ; Else x = -c/b
    push ecx
    push -1
    fmul
    fdiv
    push 1
    out ; Print number of roots
    fout ; Print root
    jmp SOLVE_SQUARE_END ; Returning
A_&_B_=_0:
    push dword [c]
    fabs
    push 0.000001
    fcmp
    ja INFINITE_ROOTS
    jmp NO_ROOTS
INFINITE_ROOTS:
    push -1
    out
    jmp SOLVE_SQUARE_END
NO_ROOTS:
    push 0
    out
    jmp SOLVE_SQUARE_END

ONE_ROOT:
    push 1
    out
    call calcFirst
    push dword [x1]
    fout
    jmp SOLVE_SQUARE_END 

calcD:
    push dword [a]
    push dword [c]
    fmul
    push 4.0
    fmul
    push dword [b]
    push dword [b]
    fmul
    fsub
    pop  dword [descr]
    ret

calcFirst:
    push 2.0
    push dword [a]
    fmul
    push eax
    push dword [b]
    fsub
    fdiv
    pop dword [x1]
    ret
calcSecond:
    push 2.0
    push dword [a]
    fmul
    push eax
    push -1.0
    fmul
    push dword [b]
    fsub
    fdiv
    pop dword [x2]
    ret
END:
    stop
