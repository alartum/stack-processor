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
#include "stack_t.h"
#include "cpu_t.h"
#include <time.h>

int main (int argc, char* argv[])
{
    CHECK_DEFAULT_ARGS();
    char prog_name[NAME_MAX] = {};
    //bool is_debug = false;
    switch (argc)
    {
    case 2:
        strcpy (prog_name, argv[1]);
        break;
    /*case 3:
        strcpy (prog_name, argv[1]);
        if (!strcmp ("--debug", argv[2])){
            is_debug = true;
            break;
        }//*/
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

    buffer_t program;
    buffer_t_construct_filename (&program, prog_name);
    cpu_t cpu;
    cpu_t_construct(&cpu);
    if (!cpu_t_load_program(&cpu, &program)){
        COMMENT("Loading problem!");
        goto ERROR;
    }
    clock_t begin, end;
    begin = clock();
    if (!cpu_t_run(&cpu)){
        COMMENT ("Runtime error occured!");
        goto ERROR;
    }
    end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf ("Emulated: %lfms\n", time_spent*1000);

    return NO_ERROR;
ERROR:
    cpu_t_destruct (&cpu);
    buffer_t_destruct (&program);

    return WRONG_RESULT;
}
