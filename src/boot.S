.section .text
.global _start
.type _start, @function
.extern kmain

_start:
    #cli-выкл аппаратных коллебаний cld-направление строковых операций вперёд(инициализация окружения)
    cli
    cld

    #создание собственного стека
    movl $stack_top, %esp
    andl $-16, %esp
    subl $8, %esp
    # GRUB Multiboot2 передаёт:
    # EAX = magic number
    # EBX = адрес структуры Multiboot information
    # Аргументы C-функции кладём справа налево:
    pushl %ebx
    pushl %eax
    call kmain

.hang:
    cli
    hlt
    jmp .hang

.section .bss
.align 16
stack_bottom:
    .skip 16384
stack_top: