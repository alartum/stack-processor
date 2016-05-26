#ifndef REG_ADDRESS_INCLUDED
#define REG_ADDRESS_INCLUDED
// Right way to define: ADDRESS(name, address, size, cmd_offset)
// Current register size is 4 bytes = 32 bits.
// So, it's the example of x32 architecture stack processor.
#define REG_SIZE 4
#define REG_NUMBER 8

#endif

#ifndef DEFINES_ONLY
///Accumulator register
ADDRESS(eax, 0 * REG_SIZE, 4, 6)
///Counter register
ADDRESS(ecx, 1 * REG_SIZE, 4, 6)
///Data register
ADDRESS(edx, 2 * REG_SIZE, 4, 6)
///Base register
ADDRESS(ebx, 3 * REG_SIZE, 4, 6)
///Stack pointer register
ADDRESS(esp, 4 * REG_SIZE, 4, 6)
///Base pointer register
ADDRESS(ebp, 5 * REG_SIZE, 4, 6)
///Source index register
ADDRESS(esi, 6 * REG_SIZE, 4, 6)
///Destination index register
ADDRESS(edi, 7 * REG_SIZE, 4, 6)

///Accumulator register
ADDRESS(ax, 0 * REG_SIZE, 2, 5)
///Counter register
ADDRESS(cx, 1 * REG_SIZE, 2, 5)
///Data register
ADDRESS(dx, 2 * REG_SIZE, 2, 5)
///Base register
ADDRESS(bx, 3 * REG_SIZE, 2, 5)
///Stack pointer register
ADDRESS(sp, 4 * REG_SIZE, 2, 5)
///Base pointer register
ADDRESS(bp, 5 * REG_SIZE, 2, 5)
///Source index register
ADDRESS(si, 6 * REG_SIZE, 2, 5)
///Destination index register
ADDRESS(di, 7 * REG_SIZE, 2, 5)

///Accumulator register
ADDRESS(al, 0 * REG_SIZE, 1, 4)
///Counter register
ADDRESS(cl, 1 * REG_SIZE, 1, 4)
///Data register
ADDRESS(dl, 2 * REG_SIZE, 1, 4)
///Base register
ADDRESS(bl, 3 * REG_SIZE, 1, 4)
///Stack pointer register
ADDRESS(spl, 4 * REG_SIZE, 1, 4)
///Base pointer register
ADDRESS(bpl, 5 * REG_SIZE, 1, 4)
///Source index register
ADDRESS(sil, 6 * REG_SIZE, 1, 4)
///Destination index register
ADDRESS(dil, 7 * REG_SIZE, 1, 4)
#endif
