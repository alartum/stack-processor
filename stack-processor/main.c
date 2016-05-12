/// Author name
#define AUTHOR "Alartum"
/// Project name
#define PROJECT "StackProcessor"
/// Version
#define VERSION "2.1"
#define DEBUG

#include <stdio.h>
#include "mylib.h"
#include <string.h>
#include <limits.h>
#include "stack.h"
#include "CPU.h"



int main (int argc, char* argv[])
{
    CHECK_DEFAULT_ARGS();
    char prog_name[NAME_MAX] = {};
    switch (argc)
    {
    case 2:
        strcpy (prog_name, argv[1]);
        break;
    default:
        WRITE_WRONG_USE();
    }
    // Testing staff
    /*
    stack_t stack;
    stack_t_construct(&stack);
    char a = 'a', b = 'b';
    stack_t_push(&stack, &a, 1);
    stack_t_dump(&stack);
    stack_t_push(&stack, &b, 1);
    stack_t_dump(&stack);
    stack_t_pop(&stack, &a, 1);
    stack_t_dump(&stack);
    printf ("a = %c, b = %c\n", a, b);
    stack_t_destruct(&stack);
    */

    Buffer program;
    buffer_construct (&program, prog_name);
    cpu_t cpu;
    cpu_t_construct(&cpu);
    cpu_t_set_program(&cpu, &program);
    if (!cpu_t_execute(&cpu))
    {
        cpu_t_destruct(&cpu);
        COMMENT ("Runtime error occured!");

        return WRONG_RESULT;
    }
    cpu_t_destruct (&cpu);
    buffer_destruct (&program);

    return NO_ERROR;
}
