[BITS 64]

extern exception_handler

section .text

global divideException
global debugException
global breakpointException
global overflowException
global boundRangeExceededException
global invalidOpcodeException
global deviceNotAvaliableException
global doubleFaultException
global coprocessorSegmentOverrunException
global invalidTSSException
global segmentNotPresentException
global stackSegmentFaultException
global generalProtectionFaultException
global pageFaultException
global floatingPointException
global alignmentCheckException
global machineCheckException
global simdFloatingPointException
global virtualisationException

align 0x08, db 0x00
divideException:
    push 0
    push 0
    jmp baseHandler

align 0x08, db 0x00
debugException:
    push 0
    push 1
    jmp baseHandler

align 0x08, db 0x00
breakpointException:
    push 0
    push 3
    jmp baseHandler

align 0x08, db 0x00
overflowException:
    push 0
    push 4
    jmp baseHandler

align 0x08, db 0x00
boundRangeExceededException:
    push 0
    push 5
    jmp baseHandler

align 0x08, db 0x00
invalidOpcodeException:
    push 0
    push 6
    jmp baseHandler

align 0x08, db 0x00
deviceNotAvaliableException:
    push 0
    push 7
    jmp baseHandler

align 0x08, db 0x00
doubleFaultException:
    push 8
    jmp baseHandler

align 0x08, db 0x00
coprocessorSegmentOverrunException:
    push 0
    push 9
    jmp baseHandler

align 0x08, db 0x00
invalidTSSException:
    push 10
    jmp baseHandler

align 0x08, db 0x00
segmentNotPresentException:
    push 11
    jmp baseHandler

align 0x08, db 0x00
stackSegmentFaultException:
    push 12
    jmp baseHandler

align 0x08, db 0x00
generalProtectionFaultException:
    push 13
    jmp baseHandler

align 0x08, db 0x00
pageFaultException:
    push 14
    jmp baseHandler

align 0x08, db 0x00
floatingPointException:
    push 0
    push 16
    jmp baseHandler

align 0x08, db 0x00
alignmentCheckException:
    push 17
    jmp baseHandler

align 0x08, db 0x00
machineCheckException:
    push 0
    push 18
    jmp baseHandler

align 0x08, db 0x00
simdFloatingPointException:
    push 0
    push 19
    jmp baseHandler

align 0x08, db 0x00
virtualisationException:
    push 0
    push 20
    jmp baseHandler

align 0x08, db 0x00
baseHandler:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov rdi, rax
    mov rax, cr2
    push rax
    cld
    mov rdi, rsp
    call exception_handler 
    add rsp, 8
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp, 0x10
    iretq
