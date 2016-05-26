/// Author name
#define AUTHOR "Alartum"
/// Project name
#define PROJECT "Translator"
/// Version
#define VERSION "1.0"
#define DEBUG
//#undef DEBUG
#define VERBOSE
//#undef VERBOSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <limits.h>
#include "mylib.h"
#include "image_t.h"
#include "buffer_t.h"
#include <time.h>


#define STACK_OFFSET 128
// Gets data section and writes it to the stack.
// RBP must be saved for correct work
// ATTENTION: obviously, it spoiles rsp
void load_data_section (const void* data, size_t nbytes);
// Gets code section and writes it to the allocated memory
// ATTENTION: obviously, it spoiles rsp
// Returns pointer to the newly created memory
void* load_code_section (const void* data, size_t nbytes);
// Just runs the code situated in the given area
void run_code (void* address);
void stack_dump (size_t nbytes);

int main (int argc, char* argv[])
{
    //No args by default
    /*CHECK_DEFAULT_ARGS();//*/
    char prog_name[NAME_MAX] = "program.bin";
    /*switch (argc)
    {
    case 2:
        strcpy (prog_name, argv[1]);
        break;
    default:
        WRITE_WRONG_USE();
    }//*/
    //run_code(load_code_section(0,0));
    buffer_t binary;
    buffer_t_construct_filename(&binary, prog_name);
    image_t image;

    image_t_construct(&image, &binary);
    image_t_translate(&image);
    //clock_t begin = clock();
    image_t_execute(&image);
    /*clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf ("Translated: %lfms\n", time_spent*1000); //*/
    image_t_destruct(&image);
    buffer_t_destruct(&binary);
    return NO_ERROR;
}

void* load_code_section (const void* data, size_t nbytes)
{
    (void)data;
    printf ("Loading code...");

    // Init memory
    //uint8_t secret_place = mmap (NULL, nbytes, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    uint8_t* secret_place = mmap (NULL, 1024, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memset(secret_place, 0xc3, 1024);

    // Load code
    // Address of mov arg
    uint8_t* arg = secret_place + 10;
    uint8_t cmd[] = {0xCC,
                     0x48, 0xB8};
    /*for (uint8_t i = 0, i <= 127, i++){
        cmd[1]=
    }*/
    memcpy(secret_place, cmd, sizeof(cmd));
    //memcpy((char*)secret_place + 6, &arg, 6);
    memcpy(secret_place + 3, &arg, 8);
    uint8_t mov_rax_rax[] = {0x48, 0x8B, 0x00};
    memcpy(secret_place + 11, mov_rax_rax, 3);
    //__asm__ ("int $0x3;");
    printf ("ok\n");
    return secret_place;
}

// As we don't want to spoil RSP and RBP, we need to make it inline
void load_data_section (const void* data, size_t nbytes)
{
    printf ("Loading data...");
    printf ("ok\n");
    //memcpy(free, data, nbytes);
}

void run_code (void* address)
{
    void (*inject)() = (void(*)())address;
    inject();
}

void stack_dump (size_t nbytes)
{
    printf ("Stack dump:\n\n");
    void* stack = __builtin_frame_address(0);

    size_t i = 0;
    char data_color[10] = ANSI_COLOR_RESET;
    char offset[4] = {'\0', ' ', '\0', '\0'};

    for (char* p = (char*)stack; i < nbytes; p--, i++){
        if (i % 4 == 0)
            strcpy(data_color, (!strcmp(data_color, ANSI_COLOR_RESET))? ANSI_COLOR_GREEN : ANSI_COLOR_RESET);
        if (i % 8 == 0)
            offset[0] = (offset[0] == '\0')? ' ':'\0';
        printf ("0x%lx:    %s%s%02X  char:[%c]", (uint64_t)p, offset, data_color, (unsigned)((*(char*)p) & 0xFF), (*(char*)p) & 0xFF);
        if (i % 4 == 0){
            printf ("  int:%d  float:%g", *((int*)p), *((float*)p));
            printf ("  addr:0x%lx  double:%g", *((uint64_t*)p), *((double*)p));
        }
        printf (ANSI_COLOR_RESET"\n");
    }
}
