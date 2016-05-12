#ifndef REG_ADDRESS_INCLUDED
#define REG_ADDRESS_INCLUDED
// Right way to define: ADD_INFO(name, address, size, cmd_offset)
// Current register size is 4 bytes = 32 bits.
// So, it's the example of x32 architecture stack processor.
#define REG_SIZE 4
#define REG_NUMBER 8

#endif

#ifndef DEFINES_ONLY
///Accumulator register
ADDRESS(eax, 0 * REG_SIZE, 4, 6)
///Base register
ADDRESS(ebx, 1 * REG_SIZE, 4, 6)
///Counter register
ADDRESS(ecx, 2 * REG_SIZE, 4, 6)
///Data register
ADDRESS(edx, 3 * REG_SIZE, 4, 6)
///Source index register
ADDRESS(esi, 4 * REG_SIZE, 4, 6)
///Destination index register
ADDRESS(edi, 5 * REG_SIZE, 4, 6)
///Stack pointer register
ADDRESS(esp, 6 * REG_SIZE, 4, 6)
///Base pointer register
ADDRESS(ebp, 7 * REG_SIZE, 4, 6)

///Accumulator register
ADDRESS(ax, 0 * REG_SIZE, 2, 5)
///Base register
ADDRESS(bx, 1 * REG_SIZE, 2, 5)
///Counter register
ADDRESS(cx, 2 * REG_SIZE, 2, 5)
///Data register
ADDRESS(dx, 3 * REG_SIZE, 2, 5)
///Source index register
ADDRESS(si, 4 * REG_SIZE, 2, 5)
///Destination index register
ADDRESS(di, 5 * REG_SIZE, 2, 5)
///Stack pointer register
ADDRESS(sp, 6 * REG_SIZE, 2, 5)
///Base pointer register
ADDRESS(bp, 7 * REG_SIZE, 2, 5)

///Accumulator register
ADDRESS(al, 0 * REG_SIZE, 1, 4)
///Base register
ADDRESS(bl, 1 * REG_SIZE, 1, 4)
///Counter register
ADDRESS(cl, 2 * REG_SIZE, 1, 4)
///Data register
ADDRESS(dl, 3 * REG_SIZE, 1, 4)
///Source index register
ADDRESS(sil, 4 * REG_SIZE, 1, 4)
///Destination index register
ADDRESS(dil, 5 * REG_SIZE, 1, 4)
///Stack pointer register
ADDRESS(spl, 6 * REG_SIZE, 1, 4)
///Base pointer register
ADDRESS(bpl, 7 * REG_SIZE, 1, 4)

///Accumulator register
ADDRESS(ah, 0 * REG_SIZE + 1, 1, 4)
///Base register
ADDRESS(bh, 1 * REG_SIZE + 1, 1, 4)
///Counter register
ADDRESS(ch, 2 * REG_SIZE + 1, 1, 4)
///Data register
ADDRESS(dh, 3 * REG_SIZE + 1, 1, 4)
///Source index register
ADDRESS(sih, 4 * REG_SIZE + 1, 1, 4)
///Destination index register
ADDRESS(dih, 5 * REG_SIZE + 1, 1, 4)
///Stack pointer register
ADDRESS(sph, 6 * REG_SIZE + 1, 1, 4)
///Base pointer register
ADDRESS(bph, 7 * REG_SIZE + 1, 1, 4)

#endif
