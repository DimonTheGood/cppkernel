.global isr0
isr0:
    pusha
    call isr0_handler
    popa
    iret
