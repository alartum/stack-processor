#ifndef COMMANDS_H_INCLUDED
#define COMMANDS_H_INCLUDED
/** @file */
/// Maximum number of instructions in the program.
#define MAX_PROGRAM_LENGTH 1024

//Position argument (unsigned int)
#define ARG_POS 0x1
//Number argument (int or float)
#define ARG_NUM 0x2
//address of memory
#define ARG_MEM 0x4
//register
#define ARG_REG 0x8
//label
#define ARG_LBL 0x10
//overloaded
#define ARG_OVL 0x20
//no arguments
#define ARG_NO  0x40
//memory size: db, w, dw
#define ARG_SIZ 0x80


#endif // COMMANDS_H_INCLUDED
//! FLAG TO INT AND FLOAT OPERATIONS
/// Codes of CPU instructions
#ifndef DEFINES_ONLY
// CMD(name, key_value, shift_to_the_right, arguments_type)
//! 'key_value' of the instruction is it's line number

//^^^^^^^^^^^^^^^^^^^^^^^^
// NO ARGUMENTS
//^^^^^^^^^^^^^^^^^^^^^^^^
#define RAW_CMD(name, shift_to_the_right, arguments_type)  CMD(name, __LINE__, shift_to_the_right, arguments_type)
RAW_CMD (debug, 1, ARG_NO) //Starts the debug mode
RAW_CMD (ndebug,1, ARG_NO) //Stops the debug mode
RAW_CMD (stop, 0, ARG_NO) //End of the program
RAW_CMD (err, 0, ARG_NO) //Error indicator
RAW_CMD (out, 1, ARG_NO) //Standard output
RAW_CMD (fout, 1, ARG_NO) //Standard output
RAW_CMD (cout, 1, ARG_NO) //Standard output
RAW_CMD (add, 1, ARG_NO) //Integer addition
RAW_CMD (sub, 1, ARG_NO) //Integer subtraction
RAW_CMD (mul, 1, ARG_NO) //Integer multiplication
RAW_CMD (div, 1, ARG_NO) //Integer division [rounds down]
RAW_CMD (fadd, 1, ARG_NO) //Float addition
RAW_CMD (fsub, 1, ARG_NO) //Float subtraction
RAW_CMD (fmul, 1, ARG_NO) //Float multiplication
RAW_CMD (fdiv, 1, ARG_NO) //Float division [rounds down]
RAW_CMD (ret, 0, ARG_NO) //Returns from the function by popping it's address from the function stack
RAW_CMD (bytedup, 1, ARG_NO) //Duplicates the top byte of stack
RAW_CMD (worddup, 1, ARG_NO) //Duplicates (doubles) top 2 bytes
RAW_CMD (dworddup, 1, ARG_NO) //Duplicates the top 4 bytes of stack
RAW_CMD (bytedupd, 1, ARG_NO) //Duplicates 2 top 1 byte elements of the stack
RAW_CMD (worddupd, 1, ARG_NO) //Duplicates 2 top 2 byte elements of the stack
RAW_CMD (dworddupd, 1, ARG_NO) //Duplicates 2 top 4 byte elements of the stack
RAW_CMD (in , 1, ARG_NO) //Standard input [int]
RAW_CMD (fin , 1, ARG_NO) //Standard input [float]
RAW_CMD (cin , 1, ARG_NO) //Standard input [char]
RAW_CMD (abs, 1, ARG_NO) //Absolute value [int]
RAW_CMD (fabs, 1, ARG_NO) //Absolute value [float]
RAW_CMD (cmp, 1, ARG_NO) //Compares the TOP element with the PREVIOUS [int]
RAW_CMD (fcmp, 1, ARG_NO) //Compares the TOP element with the PREVIOUS [float]
RAW_CMD (ccmp, 1, ARG_NO) //Compares the TOP element with the PREVIOUS [char]
RAW_CMD (mod, 1, ARG_NO) //Reminder from dividing the TOP element by the PREVIOUS [int]

//^^^^^^^^^^^^^^^^^^^^^^^^
// OVERLOADED commands
//^^^^^^^^^^^^^^^^^^^^^^^^
RAW_CMD (push,      0, (ARG_NUM | ARG_REG | ARG_MEM | ARG_SIZ | ARG_LBL)) // Pushes something to the stack
RAW_CMD (push_mem_byte,  0, ARG_OVL) // Pushes value from the given address to the stack [1 byte]
RAW_CMD (push_mem_word ,  0, ARG_OVL) // Pushes value from the given address to the stack [2 bytes]
RAW_CMD (push_mem_dword,  0, ARG_OVL) // Pushes value from the given address to the stack [4 bytes]
RAW_CMD (push_reg_byte,  0, ARG_OVL) // Pushes value from the given register to the stack [1 byte]
RAW_CMD (push_reg_word,  0, ARG_OVL) // Pushes value from the given register to the stack [2 byte]
RAW_CMD (push_reg_dword,  0, ARG_OVL) // Pushes value from the given register to the stack [4 byte]
RAW_CMD (push_int,  0, ARG_OVL) // Pushes the given [int] value to the stack
RAW_CMD (push_float,  0, ARG_OVL) // Pushes the given [float] value to the stack
RAW_CMD (push_char,  0, ARG_OVL) // Pushes the given [char] value to the stack (format: 'a')
// Dummy command
RAW_CMD  (pop,        0, (ARG_MEM | ARG_REG | ARG_SIZ)) // Poppes something from the stack
RAW_CMD  (pop_mem_byte,  0, ARG_OVL) // Poppes value from the stack to the given address [1 byte]
RAW_CMD  (pop_mem_word,   0, ARG_OVL) // Poppes value from the stack to the given address [2 bytes]
RAW_CMD  (pop_mem_dword,  0, ARG_OVL) // Poppes value from the stack to the given address [4 bytes]
RAW_CMD  (pop_reg_byte,    0, ARG_OVL) // Poppes value from the stack to the given register [1 byte]
RAW_CMD  (pop_reg_word,    0, ARG_OVL) // Poppes value from the stack to the given register [2 byte]
RAW_CMD  (pop_reg_dword,    0, ARG_OVL) // Poppes value from the stack to the given register [4 byte]
//^^^^^^^^^^^^^^
// LABEL commands
//^^^^^^^^^^^^^^
// Use cmp first to compare the top two elements and get the result in flags register
// T = TOP, P = PREVIOUS
RAW_CMD   (ja,   0, (ARG_POS | ARG_LBL)) //Jump if T >  P
RAW_CMD   (jae,  0, (ARG_POS | ARG_LBL)) //Jump if T >= P
RAW_CMD   (jb,   0, (ARG_POS | ARG_LBL)) //Jump if T <  P
RAW_CMD   (jbe,  0, (ARG_POS | ARG_LBL)) //Jump if T >= P
RAW_CMD   (je,   0, (ARG_POS | ARG_LBL)) //Jump if T == P
RAW_CMD   (jne,  0, (ARG_POS | ARG_LBL)) //Jump if T != P
RAW_CMD   (jmp,  0, (ARG_POS | ARG_LBL)) //Jump [no condition]
RAW_CMD   (call, 0, ARG_LBL            ) //Push the function address to the stack and then  call it
#endif
