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
    bool is_debug = false;
    switch (argc)
    {
    case 2:
        strcpy (prog_name, argv[1]);
        break;
    case 3:
        strcpy (prog_name, argv[1]);
        if (!strcmp ("--debug", argv[2])){
            is_debug = true;
            break;
        }
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
    if (!cpu_t_load_program(&cpu, &program)){
        COMMENT("Loading problem!");
        goto ERROR;
    }
    if (!cpu_t_run(&cpu, is_debug)){
        COMMENT ("Runtime error occured!");
        goto ERROR;
    }

    return NO_ERROR;
ERROR:
    cpu_t_destruct (&cpu);
    buffer_destruct (&program);

    return WRONG_RESULT;
}
