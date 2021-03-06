#include "mylib.h"
#include <assert.h>
#include <errno.h>
#include <string.h>

#ifndef STACK_H_INCLUDED
#define STACK_H_INCLUDED

#define MAX_PRINTED 32
/// More comfortable dump
#define stack_t_dump(This) stack_t_dump_(This, #This)

// Indent of the dump is declared in mylib
extern int DUMP_INDENT;

/**
@brief Simple stack of integer
stack_t contains integer numbers and provides some operations with them
*/
typedef struct stack_t stack_t;
struct stack_t
{
    char* data;/**< Array of values. */
    char* top;/**< Pointer to the first byte of free space> */
    size_t size;/**< Number of elements in the stack. */
    size_t max_size;

    bool is_valid;/**< State of the stack. true if valid, false otherwise. */
};

/**
*@brief Standard stack constructor.
*
*Constructs stack with the top pointer at NULL. Writes to errno.
*@param This Pointer to the stack to be constructed.
*@return 1 (true) if success, 0 (false) otherwise.
*@todo Dynamic memory management, so its OK that only true is returned by now
*/
bool stack_t_construct (stack_t* This, size_t nbytes);

/**
*@brief Copy stack constructor.
*
*Constructs stack as copy of other. Writes to errno.
*@param This Pointer to the stack to be constructed.
*@param other The stack to copy from.
*@return 1 (true) if success, 0 (false) otherwise.
*/
bool stack_t_construct_copy (stack_t* This, const stack_t* other);

/**
*@brief Destructs the stack.
*
*Destructs the stack, setting its values to poison.
*@param This Pointer to the stack to be destructed.
*/
void stack_t_destruct (stack_t* This);

/**
*@brief Validates the stack.
*
*Checks if the stack is correct according to its values.
*@param This Pointer to the stack to be checked.
*@return true if the stack is valid, false otherwise.
*/
bool stack_t_OK (const stack_t* This);

/**
*@brief Prints stack's dump.
*
*Outputs the current state of stack.
*@param This Pointer to the stack to be dumped.
*/
void stack_t_dump_ (const stack_t* This, const char name[]);

/**
*@brief Pushes value to stack.
*
*Put the given value to stack as top element.
*@param This Pointer to the stack to perform operation on.
*@param value Value to be pushed.
*@return true if success, false otherwise
*/
bool stack_t_push (stack_t* This, const void* data, size_t size);

/**
*@brief Pops value from the stack.
*
*Pops the top element from the stack and returnes it.
*@param This Pointer to the stack to perform operation on.
*@param value Value that was popped. In case it wasn't successful, invalidates stack.
*/
bool stack_t_pop (stack_t* This, void* dest, size_t size);

bool stack_t_construct_no_alloc (stack_t *This, char* data, size_t nbytes)
{
    assert (This);
    This->data = data;
    This->top = This->data + nbytes - 1;
    This->size = 0;
    This->max_size = nbytes;
    This->is_valid= true;

    return true;
}
bool stack_t_construct (stack_t *This, size_t nbytes)
{
    assert (This);
    This->data = (char*)malloc(nbytes);
    if (!This->data){
        printf ("stack_t_construct: Allocation error!\n");
        return false;
    }
    This->top = This->data + nbytes - 1;
    This->size = 0;
    This->max_size = nbytes;
    This->is_valid= true;

    return true;
}

bool stack_t_construct_copy (stack_t* This, const stack_t* other)
{
    assert (This);
    ASSERT_OK(stack_t, other);
    This->data = (char*)malloc(other->size);
    if (!This->data){
        printf ("stack_t_construct: Allocation error!\n");
        return false;
    }
    if (other->size == 0)
    {
        This->top = This->data + This->max_size - 1;
        This->is_valid = true;
        This->size = 0;

        return true;
    }
    else
    {
        memcpy (This->data, other->data, other->size);
        if (!This->data)
        {
            errno = EFAULT;
            stack_t_destruct(This);
            return false;
        }
        This->size = other->size;
        This->top = This->data + This->max_size - 1 - This->size;
        This->is_valid = true;

        return true;
    }
    ASSERT_OK(stack_t, This);
    ASSERT_OK(stack_t, other);
}

void stack_t_destruct (stack_t* This)
{
    assert (This);
    free(This->data);
    This->top = NULL;
    This->size = 0;
    // Destruction must work always
    This->is_valid = false;
}

void stack_t_destruct_no_alloc (stack_t* This)
{
    assert (This);
    This->top = NULL;
    This->size = 0;
    // Destruction must work always
    This->is_valid = false;
}

bool stack_t_OK (const stack_t* This)
{
    assert (This);
    return This->data && This->is_valid;
}

void stack_t_dump_ (const stack_t* This, const char name[])
{
    DUMP_INDENT += INDENT_VALUE;
    assert (This);
    printf ("%*s%s = " ANSI_COLOR_BLUE "stack_t" ANSI_COLOR_RESET " (", DUMP_INDENT, "", name);
    if (stack_t_OK(This))
        printf (ANSI_COLOR_GREEN "ok" ANSI_COLOR_RESET ")\n");
    else
        printf (ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET ")\n");
    printf(ANSI_COLOR_YELLOW "-----------------------------------------------------" ANSI_COLOR_RESET "\n");
    printf ("%*stop = %p\n", DUMP_INDENT, "", This->top);
    printf ("%*ssize = %lu\n", DUMP_INDENT, "", This->size);
    /// The stack data is void now, we don't know the type of the elements
    printf ("%*sdata:\n", DUMP_INDENT, "");
    DUMP_INDENT += INDENT_VALUE;
    if (This->top != NULL)
    {
        size_t i = This->max_size - 1;
        for (; (This->data + i)!= This->top && i >= This->max_size - MAX_PRINTED; i --)
            printf ("%*s" ANSI_COLOR_MAGENTA ">[%3lu]" ANSI_COLOR_RESET " %02X\n", DUMP_INDENT-1, "", i, (unsigned int)(This->data[i] & 0xFF));
        // Printing top
        printf ("%*s" ANSI_COLOR_CYAN ">>[%3lu]" ANSI_COLOR_RESET " %02X\n", DUMP_INDENT-2, "", i, (unsigned int)(This->data[i] & 0xFF));
        i--;
        for (; i > This->size - MAX_PRINTED; i --)
            printf ("%*s[%3lu] %02X\n", DUMP_INDENT, "", i, (unsigned int)(This->data[i] & 0xFF));
    }
    else
        printf ("%*s" ANSI_COLOR_YELLOW "[EMPTY]" ANSI_COLOR_RESET "\n", DUMP_INDENT, "");
    DUMP_INDENT -= INDENT_VALUE;
    printf(ANSI_COLOR_YELLOW "-----------------------------------------------------" ANSI_COLOR_RESET "\n");
    DUMP_INDENT -= INDENT_VALUE;
}

bool stack_t_push (stack_t* This, const void* data, size_t nbytes)
{
    ASSERT_OK(stack_t, This);
    This->size += nbytes;
    if (This->size <= This->max_size){
        This->top -= nbytes;
        memcpy(This->top + 1, data, nbytes);
        ASSERT_OK(stack_t, This);
        return true;
    }
    else{
        stack_t_destruct(This);
        return false;
    }
    ASSERT_OK(stack_t, This);

    return true;
}

bool stack_t_pop (stack_t* This, void* dest, size_t nbytes)
{
    assert (This);
    ASSERT_OK(stack_t, This);

    if (This->size >= nbytes){
        memcpy(dest, This->top + 1, nbytes);
        This->size -= nbytes;
        This->top += nbytes;
        return true;
    }
    else{
        stack_t_destruct(This);
        return false;
    }
}

#endif // STACK_H_INCLUDED
