/** @file */
#include <math.h>
#include <assert.h>
#include <errno.h>
#include "stack_t.h"
#include <stdint.h>
#include "buffer_t.h"

#ifndef cpu_t_H_INCLUDED
#define cpu_t_H_INCLUDED

#define DEFINES_ONLY
#include "reg_address.h"
#undef DEFINES_ONLY

/// ESP register address
#define ESP   6 * REG_SIZE
/// Memory available for stack
#define STACK_SIZE 128
/// Memory size. As the stack is placed in the beginning, be sure to allocate enough space
#define MEM_SIZE 512
/// More comfortable dump
#define cpu_t_dump(This) cpu_t_dump_(This, #This)
/// More comfortable dump
#define memory_t_dump(This) memory_t_dump_(This, #This)

/// Default frags value
#define NO_FLAG  0x0
//Zero flag
#define ZRO_FLAG 0x1
//Negative flag
#define NEG_FLAG 0x2
//Overflow flag
#define OVF_FLAG 0x4

typedef struct cpu_t cpu_t;
typedef struct memory_t memory_t;

/**
@brief Memory controller emulation

Provides interfaces to memory an is used by processor to store data.
*/
struct memory_t
{
    char* storage;
    unsigned max_size;
    unsigned size;
    char* base;

    bool is_valid;
};

void memory_t_erase (memory_t* this)
{
    memset(this->storage, 0, this->max_size - STACK_SIZE);
}
void memory_t_destruct (memory_t* this)
{
    if (this->storage)
        free (this->storage);
    this->storage = NULL;
    this->size = UINT_MAX; // Poison
    this->max_size = UINT_MAX; // Poison
    this->base = NULL; // Posion
    this->is_valid = false;
}

bool memory_t_construct (memory_t* this, unsigned memory_size)
{
    this->storage = (char*)calloc(memory_size, 1);
    if (!this->storage){
        printf ("memory_t_construct: Can't allocate memory!\n");
        memory_t_destruct (this);

        return false;
    }
    this->max_size = MEM_SIZE;
    this->size = 0;
    this->base = this->storage; // The first free byte
    this->is_valid = true;

    return true;
}

bool memory_t_OK (const memory_t* this)
{
    return (this->size <= this->max_size && this->base && this->storage && this->is_valid);
}

void memory_t_dump_(const memory_t* this, const char name[])
{
    assert (this);
    printf ("%s = " ANSI_COLOR_BLUE "memory_t" ANSI_COLOR_RESET " (", name);
    if (memory_t_OK(this))
        printf (ANSI_COLOR_GREEN "ok" ANSI_COLOR_RESET ")\n");
    else
        printf (ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET ")\n");
    printf(ANSI_COLOR_YELLOW "-----------------------------------------------------" ANSI_COLOR_RESET "\n");
    printf("\t%-10s %u\n", "size", this->size);

    if (this->max_size != UINT_MAX)
        printf("\t%-10s %u\n", "max_size", this->max_size);
    else
        printf("\t%-10s (" ANSI_COLOR_RED "NONE" ANSI_COLOR_RESET ")\n", "max_size");
    if (this->base)
        printf("\t%-10s %p\n", "base", this->base);
    else
        printf("\t%-10s (" ANSI_COLOR_RED "NONE" ANSI_COLOR_RESET ")\n", "free");
    if (this->storage)
        printf("\t%-10s %p\n", "storage", this->storage);
    else
        printf("\t%-10s (" ANSI_COLOR_RED "NONE" ANSI_COLOR_RESET ")\n", "storage");

    printf(ANSI_COLOR_YELLOW "-----------------------------------------------------" ANSI_COLOR_RESET "\n");
}

unsigned memory_t_reserve (memory_t* this, unsigned nbytes)
{
    ASSERT_OK(memory_t, this);
    assert(nbytes);
    // Save the position of newly reserved memory
    unsigned address = this->size;

    this->size += nbytes;
    if (this->size > this->max_size){
        printf ("memory_t_reserve: Run out of memory!\n");
        memory_t_destruct(this);

        return (UINT_MAX);
    }
    this->base += nbytes;

    return (address);
}

bool memory_t_write (memory_t* this, unsigned address, const void* data, unsigned nbytes)
{
    ASSERT_OK(memory_t, this);
    assert(nbytes);

    unsigned end_pos = address + nbytes;
    #if defined(DEBUG)
    if (end_pos > this->size)
        printf ("memory_t_write: Warning! Writing to unreserved memory space!\n");
    #endif

    if (end_pos > this->max_size){
        printf ("memory_t_write: Run out of memory!\n");
        memory_t_destruct(this);

        return false;
    }
    memcpy (this->storage + address, data, nbytes);

    return true;
}

bool memory_t_read (const memory_t* this, unsigned address, void* data, unsigned nbytes)
{
    ASSERT_OK(memory_t, this);
    assert(nbytes);

    unsigned end_pos = address + nbytes;
    #if defined(DEBUG)
    if (end_pos > this->size)
        printf ("memory_t_read: Warning! Reading unreserved memory space!\n");
    #endif

    if (end_pos > this->max_size){
        printf ("memory_t_read: Address is out of borders!\n");

        return false;
    }
    memcpy (data, this->storage + address, nbytes);

    return true;
}

/**
@brief Simple stack processor.

Processor is a structure that containes a stack and a register to operate with.
*/
struct cpu_t
{
    bool is_debug;
    char flags; /** Flags register is separated from the main purpose registers */
    stack_t stack;/**< Operating stack. */
    char registers[REG_SIZE * REG_NUMBER];/**< Registers of the processor*/
    unsigned position; /** Instruction pointer (current instruction address in memory)  */
    memory_t memory; /** The memory controller interface emulation */

    bool state;/**< State of the cpu_t. true if ON, false if OFF. */
};

/**
*@brief Standard cpu_t constructor.
*
*Constructs cpu_t with empty stack and register.
*@param This Pointer to the cpu_t to be constructed.
*@return 1 (true) if success, 0 (false) otherwise.
*@todo Dynamic memory management, so its OK that only true is returned by now.
*/
bool cpu_t_construct (cpu_t* This);

/**
*@brief Copy cpu_t constructor.
*
*Constructs cpu_t as copy of other. Writes to errno.
*@param This Pointer to the cpu_t to be constructed.
*@param other The cpu_t to copy from.
*@return 1 (true) if success, 0 (false) otherwise.
*/
bool cpu_t_construct_copy (cpu_t* This, const cpu_t* other);

/**
*@brief Destructs the cpu_t.
*
*Destructs the cpu_t, setting its values to poison.
*@param This Pointer to the cpu_t to be destructed.
*/
void cpu_t_destruct (cpu_t* This);

/**
*@brief Validates the cpu_t.
*
*Checks if the cpu_t is correct according to its values.
*@param This Pointer to the cpu_t to be checked.
*@return true if the cpu_t is valid, false otherwise.
*/
bool cpu_t_OK (const cpu_t* This);

/**
*@brief Prints cpu_t's dump.
*
*Outputs the current state of cpu_t.
*@param This Pointer to the cpu_t to be dumped.
*/
void cpu_t_dump_ (const cpu_t* This, const char name[]);

/**
*@brief Pushes value to cpu_t stack.
*
*Puts the given value at the top of cpu_t stack.
*@param This Pointer to the cpu_t to perform operation on.
*@param value The value to be put is the stack.
*@return true if success, false otherwise.
*/
bool cpu_t_push (cpu_t* This);

/**
*@brief Pushes variable to cpu_t stack.
*
*Puts the value from the given register at the top of cpu_t stack.
*@param This Pointer to the cpu_t to perform operation on.
*@param registerN Register to read the value from.
*@return true if success, false otherwise.
*/
bool cpu_t_push_reg (cpu_t* This);

/**
*@brief Pops value from the stack.
*
*Pops the top element from the cpu_t and writes it to the given register.
*@param This Pointer to the cpu_t to perform operation on.
*@param registerN Register to write the value to.
*@return true if success, false otherwise. In case it wasn't successful, invalidates cpu_t.
*@warning Stack must contain at least one element.
*/
bool cpu_t_pop_reg (cpu_t* This);

/**
*@brief Summs the top two elements of the stack.
*
*Add the top element of stack to the previous and stroes the result in stack.
*Both top two elements are removed.
*@param This Pointer to the cpu_t to perform operation on.
*@return true if success, false otherwise. In case of fail invalidates cpu_t.
*@warning Stack must contain at least two elements.
*/
bool cpu_t_add (cpu_t* This);

/**
*@brief Subtracts the penult stack element from the top one.
*
*Gets the top element of stack and subtracts the penult stack element from it.
*Both top two elements are removed and the result is written to stack.
*@param This Pointer to the cpu_t to perform operation on.
*@return true if success, false otherwise. In case of fail invalidates cpu_t.
*@warning Stack must contain at least two elements.
*/
bool cpu_t_sub (cpu_t* This);

/**
*@brief Multiplies the top two elements of the stack.
*
*Multiplies the top element of stack to the previous and stroes the result in stack.
*Both top two elements are removed.
*@param This Pointer to the cpu_t to perform operation on.
*@return true if success, false otherwise. In case of fail invalidates cpu_t.
*@warning Stack must contain at least two elements.
*/
bool cpu_t_mul (cpu_t* This);

/**
*@brief Divides the top stack element by the previous.
*
*Gets the top element of stack and divides it by the penult stack element.
*Both top two elements are removed and the result is written to stack.
*@param This Pointer to the cpu_t to perform operation on.
*@return true if success, false otherwise. In case of fail invalidates cpu_t.
*@warning Stack must contain at least two elements.
*/
bool cpu_t_div (cpu_t* This);

/**
*@brief Raise the top element in the power of the penult one.
*
*Gets the top element of stack and raises it to power equal to the penult stack element.
*Both top two elements are removed and the result is written to stack.
*@param This Pointer to the cpu_t to perform operation on.
*@return true if success, false otherwise. In case of fail invalidates cpu_t.
*@warning Stack must contain at least two elements.
*/
bool cpu_t_pow (cpu_t* This);

/**
*@brief Executes the given program.
*
*Treats the given sequence of numbers as instructions and executes them.
*@param This Pointer to the cpu_t to perform operation on.
*@param program Array of instruction codes.
*@return true if no error has occured, false otherwise. In case of fail invalidates cpu_t.
*@warning End of the program must be marked with 'end' code. Undefined behaviour otherwise.
*/
bool cpu_t_execute (cpu_t* This);

/**
*@brief Prints the top stack element.
*
*@param This Pointer to the cpu_t to perform operation on.
*@return true if no error has occured, false otherwise. In case of fail invalidates cpu_t.
*/
bool cpu_t_out (cpu_t* This);

/**
*@brief Scans value and puts it to the top of the stack
*
*@param This Pointer to the cpu_t to perform operation on.
*@return true if no error has occured, false otherwise. In case of fail invalidates cpu_t.
*/
bool cpu_t_in (cpu_t* This);

bool cpu_t_jmp (cpu_t* This)
{
    ASSERT_OK(cpu_t, This);
    This->position++;
    //printf ("This position is: %u\n", This->position);
    unsigned new_position = *(unsigned*)(This->memory.storage + This->position);
    //printf ("New position is: %u\n", new_position);
    This->position = new_position;
    //printf ("jmp %d\n", new_position);

    return true;
}


// Some magic: (This->flags & (0xFF ^ 0x3)) resets last 2 bits of flags (cmp flags)
// Then we can set them again with (This->flags | FLAG)
#define CMP(_name, _type) \
bool cpu_t_ ## _name (cpu_t* This) \
{ \
    ASSERT_OK(cpu_t, This); \
    _type top = 0, prev = 0; \
    if (!stack_t_pop(&This->stack, &top, sizeof(_type)) || !stack_t_pop(&This->stack, &prev, sizeof(_type))){ \
        cpu_t_destruct(This); \
        ASSERT_OK(cpu_t, This); \
        return false; \
    } \
    else{ \
        *(unsigned*)(This->registers+ESP) += 2*sizeof(_type);\
        This->flags &= (0xFF ^ 0x3);\
        if (top < prev){ \
            This->flags |= NEG_FLAG; \
            /*printf ("NEG_FLAG is set!\n");*/\
        }\
        if (top == prev){ \
            This->flags |= ZRO_FLAG; \
            /*printf ("ZRO_FLAG is set!\n");*/\
        }\
        /*printf ("Flags: %02X\n", This->flags);*/\
        ASSERT_OK(cpu_t, This); \
        return true; \
    } \
}

CMP (cmp, int)
CMP (fcmp, float)
CMP (ccmp, char)

#define CON_JUMP(_name, _flags1, _flags2) \
bool cpu_t_ ## _name (cpu_t* This)\
{\
    ASSERT_OK(cpu_t, This);\
    This->position++;\
    unsigned new_position = *(unsigned*)(This->memory.storage + This->position);\
    assert (new_position < This->memory.max_size); \
    \
    /*printf ("(?) %02X == %02X, %02X\n", This->flags, _flags1, _flags2);*/\
    if (!((This->flags & 0x3) ^ _flags1) | !((This->flags & 0x3) ^ _flags2)){\
        This->position = new_position;\
        /*printf ("jmp %d\n", new_position);*/\
    }\
    else{\
        This->position += sizeof(unsigned);\
        /*printf ("jmp ++\n");*/\
    }\
    ASSERT_OK(cpu_t, This);\
    return true;\
}

CON_JUMP (ja,  NO_FLAG,  NO_FLAG)
CON_JUMP (jae, NO_FLAG,  ZRO_FLAG)
CON_JUMP (jb,  NEG_FLAG, NEG_FLAG)
CON_JUMP (jbe, NEG_FLAG, ZRO_FLAG)
CON_JUMP (je,  ZRO_FLAG, ZRO_FLAG)
CON_JUMP (jne, NEG_FLAG, NO_FLAG)

bool cpu_t_call(cpu_t* This)
{
    ASSERT_OK(cpu_t, This);
    This->position++;
    int new_position = *(unsigned*)(This->memory.storage+ This->position);
    This->position += sizeof(unsigned);
    if (!stack_t_push (&This->stack, &This->position, sizeof (unsigned)))
        return false;
    This->position = new_position;
    *(unsigned*)(This->registers+ESP) -= sizeof(unsigned);
    //printf ("call %d\n", new_position);
    return true;
}

bool cpu_t_err(cpu_t* This)
{
    (void)This;
    printf (ANSI_COLOR_RED "*BEEP-BEEP*"ANSI_COLOR_RESET"[program is corrupted]\n");
    return false;
}

bool cpu_t_ret(cpu_t* This)
{
    ASSERT_OK(cpu_t, This);
    unsigned ret_position = 0;
    if (!stack_t_pop (&This->stack, &ret_position, sizeof(unsigned)))
        return false;
    *(unsigned*)(This->registers+ESP) += sizeof(unsigned);
    This->position = ret_position;
    //printf ("ret %d\n", (int)ret_position);
    return true;
}
bool cpu_t_stop(cpu_t* This)
{
    (void)This;
    This->memory.storage[This->position] = 0;
    printf (ANSI_COLOR_RED"*BEEP*"ANSI_COLOR_RESET"[reached the end of the program]\n");
    return true;
}
// Overloaded
bool cpu_t_pop(cpu_t* This)
{
    (void)This;
    return false;
}

#define POP_MEM(_name, _size) \
bool cpu_t_pop_mem_ ## _name(cpu_t* This)\
{\
    ASSERT_OK(cpu_t, This);\
    This->position++;\
    unsigned address = *(unsigned*)(This->memory.storage+ This->position);\
    This->position += sizeof (unsigned);\
    char top[_size];\
    if (!stack_t_pop(&This->stack, top, _size))\
        return false;\
    if (!memory_t_write(&This->memory, address, top, _size))\
        return false;\
    *(unsigned*)(This->registers+ESP) += _size;\
    return true;\
}

#define VAR(_name, _nbytes) POP_MEM(_name, _nbytes)
#include "var_sizes.h"
#undef VAR

#define PUSH_MEM(_name, _size) \
bool cpu_t_push_mem_ ## _name(cpu_t* This) \
{\
    ASSERT_OK(cpu_t, This);\
    This->position++;\
    unsigned address = *(unsigned*)(This->memory.storage+ This->position);\
    This->position += sizeof(unsigned);\
    char data[_size];\
    if (!memory_t_read (&This->memory, address, data, _size))\
        return false;\
    if (!stack_t_push(&This->stack, data, _size))\
        return false;\
    *(unsigned*)(This->registers+ESP) -= _size;\
    ASSERT_OK(cpu_t, This);\
    return true;\
}

#define VAR(_name, _nbytes) PUSH_MEM(_name, _nbytes)
#include "var_sizes.h"
#undef VAR


bool cpu_t_load_program (cpu_t* This, const buffer_t* program)
{
    memory_t_erase(&This->memory);
    COMMENT ("Loading program...");
    if (!memory_t_write(&This->memory, 0, program->data, program->size))
        return false;
    This->position = 0;
    COMMENT ("Running...");
    return true;
}

bool cpu_t_construct (cpu_t* This)
{
    assert (This);
    if (!memory_t_construct(&This->memory, MEM_SIZE)){
        cpu_t_destruct(This);
        return false;
    }
    memory_t_reserve(&This->memory, This->memory.max_size);
    This->flags = 0;
    This->is_debug = false;
    memset (This->registers, 0, REG_SIZE * REG_NUMBER);
    This->position = 0;
    if (!stack_t_construct_no_alloc(&This->stack, This->memory.storage + This->memory.max_size - STACK_SIZE, STACK_SIZE)){
        cpu_t_destruct(This);
        return false;
    }
    *(unsigned*)(This->registers+ESP) = This->memory.max_size - 1;\
    This->state = true;
    printf (ANSI_COLOR_RED"*BEEP*"ANSI_COLOR_RESET"[processor was turned ON]\n");
    return true;
}

bool cpu_t_construct_copy(cpu_t* This, const cpu_t* other)
{
    assert (This);
    ASSERT_OK(cpu_t, other);
    This->position = other->position;
    memcpy (This->registers, other->registers, REG_SIZE * REG_NUMBER);
    if (!memory_t_construct(&This->memory, other->memory.max_size)){
        cpu_t_destruct(This);
        return false;
    }
    memory_t_reserve(&This->memory, This->memory.max_size);
    if (!stack_t_construct_no_alloc(&This->stack, This->memory.storage, other->stack.size)){
        cpu_t_destruct(This);
        return false;
    }
    This->is_debug = other->is_debug;
    This->flags = other->flags;
    This->state = true;
    printf (ANSI_COLOR_RED"*BEEP*"ANSI_COLOR_RESET"[processor was turned ON]\n");
    return true;
}

void cpu_t_destruct (cpu_t* This)
{
    assert (This);
    This->position = 0;
    memset (This->registers, 0, REG_SIZE * REG_NUMBER);
    This->flags = 0;
    This->is_debug = false;
    stack_t_destruct_no_alloc(&This->stack);
    memory_t_destruct(&This->memory);
    This->state = false;
    printf (ANSI_COLOR_RED"*BEEP*"ANSI_COLOR_RESET"[processor was turned OFF]\n");
}

bool cpu_t_OK (const cpu_t* This)
{
    assert (This);
    return This->state && stack_t_OK(&This->stack) && memory_t_OK(&This->memory) && This->registers;
}

void cpu_t_dump_ (const cpu_t* This, const char name[])
{
    DUMP_INDENT += INDENT_VALUE;
    assert (This);
    printf ("%s = " ANSI_COLOR_BLUE "stack_t" ANSI_COLOR_RESET " (", name);
    if (cpu_t_OK(This))
        printf (ANSI_COLOR_GREEN "ok" ANSI_COLOR_RESET ")\n");
    else
        printf (ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET ")\n");
    printf(ANSI_COLOR_YELLOW "-----------------------------------------------------" ANSI_COLOR_RESET "\n");
    printf ("%*sis_debug = %d\n", DUMP_INDENT, "", This->is_debug);
    printf ("%*smemory:\n", DUMP_INDENT, "");
    memory_t_dump(&This->memory);
    printf ("%*sregisters:\n", DUMP_INDENT, "");
    for (unsigned i = 0; i < REG_NUMBER; i++)
        printf ("%*s[%1u] %d\n", DUMP_INDENT, "", i, This->registers[i*REG_SIZE]);
    printf ("%*sprogram:\n", DUMP_INDENT, "");
    for (char* i = This->memory.storage; i != This->stack.data; i++)
        printf ("%02x ", (unsigned int)(*i));
    printf ("\n%*sstack:\n", DUMP_INDENT, "");
    stack_t_dump(&This->stack);
    printf(ANSI_COLOR_YELLOW "-----------------------------------------------------" ANSI_COLOR_RESET "\n");
    DUMP_INDENT -= INDENT_VALUE;
}
bool cpu_t_debug (const cpu_t* This)
{
    // Breaking const qualifier to save the logic of the program
    ASSERT_OK(cpu_t, This);
    ((cpu_t*)This)->is_debug = true;
    return true;
}

bool cpu_t_ndebug (const cpu_t* This)
{
    // Breaking const qualifier to save the logic of the program
    ASSERT_OK(cpu_t, This);
    ((cpu_t*)This)->is_debug = false;
    return true;
}

void cpu_t_show_info (const cpu_t* This)
{
    DUMP_INDENT += INDENT_VALUE;
    #define ADDRESS(_name, _address, _size, _cmd_offset) \
    if (_size == 4 && _address != ESP){\
        printf (ANSI_COLOR_YELLOW"%*s[" #_name "] "ANSI_COLOR_RESET, DUMP_INDENT, "");\
        for (unsigned i = 0; i < 4; i ++)\
            printf ("%02X ", (unsigned)(This->registers[_address+i] & 0xFF));\
        printf ("\n");\
    }
    #include "reg_address.h"
    #undef ADDRESS
    printf (ANSI_COLOR_YELLOW"%*s[esp] "ANSI_COLOR_RESET"%#0x\n", DUMP_INDENT, "", *(unsigned*)(This->registers+ESP));
    printf (ANSI_COLOR_YELLOW"%*s[eip] "ANSI_COLOR_RESET"%#0x\n", DUMP_INDENT, "", This->position);
    printf (ANSI_COLOR_YELLOW"%*s[fla]"ANSI_COLOR_RESET" %02X\n", DUMP_INDENT, "", (unsigned)(This->flags & 0xFF));
    printf ("\n%*sstack:\n", DUMP_INDENT, "");
    stack_t_dump(&This->stack);
    DUMP_INDENT -= INDENT_VALUE;
}

bool cpu_t_push (cpu_t* This)
{
    (void)This;
    return false;
}

#define PUSH_NUM(_type)\
bool cpu_t_push_ ## _type (cpu_t* This)\
{\
    ASSERT_OK(cpu_t, This);\
    This->position ++;\
    char* data = This->memory.storage + This->position;\
    This->position += sizeof(_type);\
\
    if (!stack_t_push(&This->stack, data, sizeof(_type))){\
        cpu_t_destruct(This);\
        return false;\
    }\
    *(unsigned*)(This->registers+ESP) -= sizeof(_type);\
    ASSERT_OK(cpu_t, This);\
    return true;\
}

PUSH_NUM(int)
PUSH_NUM(float)
PUSH_NUM(char)

#define DUP(_name, _nbytes)\
bool cpu_t_ ## _name ## dup (cpu_t* This)\
{\
    ASSERT_OK(cpu_t, This);\
    if (!stack_t_push(&This->stack, This->stack.top + 1, _nbytes)){\
        cpu_t_destruct(This);\
        return false;\
    }\
    *(unsigned*)(This->registers+ESP) -= _nbytes;\
    return true;\
}

#define VAR(_name, _nbytes) DUP(_name, _nbytes)
#include "var_sizes.h"
#undef VAR

#define DUPD(_name, _nbytes)\
bool cpu_t_ ## _name ## dupd (cpu_t* This)\
{\
    ASSERT_OK(cpu_t, This);\
    if (!stack_t_push(&This->stack, This->stack.top + 1, 2*_nbytes)){\
        cpu_t_destruct(This);\
        return false;\
    }\
    *(unsigned*)(This->registers+ESP) -= 2*_nbytes;\
    return true;\
}

#define VAR(_name, _nbytes) DUPD(_name, _nbytes)
#include "var_sizes.h"
#undef VAR

#define PUSH_REG(_name, _nbytes) \
bool cpu_t_push_reg_ ## _name (cpu_t* This)\
{\
    ASSERT_OK(cpu_t, This);\
    This->position ++;\
    char address = This->memory.storage[This->position];\
    This->position ++;\
\
    char* data = This->registers + address;\
    if (!stack_t_push(&This->stack, data, _nbytes)){\
        cpu_t_destruct(This);\
        return false;\
    }\
    *(unsigned*)(This->registers+ESP) -= _nbytes;\
    ASSERT_OK(cpu_t, This);\
    return true;\
}

#define VAR(_name, _nbytes) PUSH_REG(_name, _nbytes)
#include "var_sizes.h"
#undef VAR

#define POP_REG(_name, _nbytes) \
bool cpu_t_pop_reg_ ## _name (cpu_t* This)\
{\
    ASSERT_OK(cpu_t, This);\
    This->position ++;\
    char address = This->memory.storage[This->position];\
    This->position ++;\
\
    char* data = This->registers + address;\
    if (!stack_t_pop(&This->stack, data, _nbytes)){\
        cpu_t_destruct(This);\
        return false;\
    }\
    *(unsigned*)(This->registers+ESP) += _nbytes;\
    ASSERT_OK(cpu_t, This);\
    return true;\
}

#define VAR(_name, _nbytes) POP_REG(_name, _nbytes)
#include "var_sizes.h"
#undef VAR

#define ARITHM(_name, _op, _type) \
bool cpu_t_ ## _name (cpu_t* This) \
{ \
    ASSERT_OK(cpu_t, This); \
    _type a = 0, b = 0, res = 0; \
    \
    if (!stack_t_pop(&This->stack, &a, sizeof(_type))){ \
        cpu_t_destruct(This); \
        ASSERT_OK(cpu_t, This); \
        return false; \
    } \
    if (!stack_t_pop(&This->stack, &b, sizeof(_type))){ \
        cpu_t_destruct(This); \
        ASSERT_OK(cpu_t, This); \
        return false; \
    } \
    res = a _op b; \
    *(unsigned*)(This->registers+ESP) += sizeof(_type);\
    ASSERT_OK(cpu_t, This); \
    return stack_t_push(&This->stack, &res, sizeof(_type)); \
}

ARITHM(add, +, int)
ARITHM(sub, -, int)
ARITHM(mul, *, int)
ARITHM(div, /, int)
ARITHM(mod, %, int)
ARITHM(fadd, +, float)
ARITHM(fsub, -, float)
ARITHM(fmul, *, float)
ARITHM(fdiv, /, float)

#define OUT(_name, _type, _spec) \
bool cpu_t_ ## _name(cpu_t* This)\
{\
    ASSERT_OK(cpu_t, This);\
    _type top = 0;\
    if (!stack_t_pop(&This->stack, &top, sizeof(_type))){\
        cpu_t_destruct(This);\
        ASSERT_OK(cpu_t, This);\
        return false;\
    }\
    printf (ANSI_COLOR_YELLOW "OUT" ANSI_COLOR_GREEN "[" #_type "]"ANSI_COLOR_YELLOW">" ANSI_COLOR_RESET _spec "\n", top);\
    *(unsigned*)(This->registers+ESP) += sizeof(_type);\
    ASSERT_OK(cpu_t, This);\
    return true;\
}

OUT(out, int, "%d")
OUT(fout, float, "%g")
OUT(cout, char , "%c")

#define ABS(_name, _type) \
bool cpu_t_ ## _name(cpu_t* This)\
{\
    ASSERT_OK(cpu_t, This);\
    _type top = 0;\
    if (!stack_t_pop(&This->stack, &top, sizeof(_type))){\
        cpu_t_destruct(This);\
        ASSERT_OK(cpu_t, This);\
        return false;\
    }\
    _type res = (_type)fabs((float)top);\
    if (!stack_t_push(&This->stack, &res, sizeof(_type))){\
        cpu_t_destruct(This);\
        ASSERT_OK(cpu_t, This);\
        return false;\
    }\
    ASSERT_OK(cpu_t, This);\
    return true;\
}

ABS(fabs, float)
ABS(abs, int)

#define IN(_name, _type, _spec) \
bool cpu_t_ ## _name (cpu_t* This)\
{\
    ASSERT_OK(cpu_t, This);\
    _type input = 0;\
\
    printf (ANSI_COLOR_YELLOW "IN" ANSI_COLOR_GREEN "[" #_type "]" ANSI_COLOR_YELLOW ">" ANSI_COLOR_RESET);\
    if (!scanf (_spec, &input))\
        printf (ANSI_COLOR_RED "*BEEP-BEEP-BEEP*" ANSI_COLOR_RESET "[scanning error]\n");\
    if (!stack_t_push(&This->stack, &input, sizeof(_type))){\
        cpu_t_destruct(This);\
        ASSERT_OK(cpu_t, This);\
        return false;\
    }\
    *(unsigned*)(This->registers+ESP) -= sizeof(_type);\
    ASSERT_OK(cpu_t, This);\
    return true;\
}\

IN(in, int, "%d")
IN(fin, float, "%f")
IN(cin, char , "%c")
// TODO: all functions are written in processor, we are only calling them
//from define of each CMD_INFO with right argument
bool cpu_t_run (cpu_t* This)
{
    if (!This->state){
        printf (ANSI_COLOR_RED "*BEEP-BEEP*"ANSI_COLOR_RESET"[cpu is corrupted]\n");
        return false;
    }
    while (This->memory.storage[This->position])
    {
        bool is_done = false;
        if (This->is_debug) printf("\n[%u] ", This->position);
        #define CMD(name, key, shift, arguments) \
        if (!is_done && This->memory.storage[This->position] == key){\
            if (This->is_debug) printf (#name "\n");\
            if (!cpu_t_ ## name (This))\
                return false;\
            This->position += shift;\
            is_done = true;\
        }
        #include "commands.h"
        #undef CMD
        if (This->is_debug && !is_done){
            printf ("cpu_t_run: Command is not recognized!\n");
        }
        if (This->is_debug){
            cpu_t_show_info (This);
            getchar();
        }
        if (This->position >= This->memory.max_size){
            printf (ANSI_COLOR_RED "*BEEP-BEEP*"ANSI_COLOR_RESET"[instruction address is out of range]\n");
            This->state = false;
            return false;
        }
    }
    return true;
}

#endif // cpu_t_H_INCLUDED
