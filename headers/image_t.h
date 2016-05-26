#include "mylib.h"
#include "buffer_t.h"
#include "list_t.h"
#include "commands_enum.h"

#define _GNU_SOURCE
#define _BSD_SOURCE

#ifndef IMAGE_T_H_INCLUDED
#define IMAGE_T_H_INCLUDED

#define image_t_dump(This) image_t_dump_(This, #This)
#define OFFSET (This->binary.data)
//###################################
//#####     Storage policy     ######
//###################################
//Breaf info:                       #
//    (0) Offset is stored in R14   #
//    (1) RDI->RSI->RDX->RCX->R8->R9#
//    (2) R10 and R11 will be broken#
//    (3) Others will be saved      #
//    (4) Temporary data register is#
//        R13                       #
//###################################
enum IMAGE_T_STATE {RUNNING, STOPPED, INTERRUPTED};
enum IMAGE_T_SIGNAL {SIG_STOP, SIG_OUT, SIG_IN};
enum IMAGE_T_TYPE {INT, FLOAT, CHAR};
typedef struct image_t image_t;
//Resizable image of source code that can be run
struct image_t
{
    char state;
    bool is_mapped; // If the map is fully loaded
    unsigned* map; // The map provides connections between the source code and the the translation
    char* resume_pos;
    buffer_t source; // Source binary
    buffer_t binary; // Translated binary
    char in_stream[8];
    char out_stream[8]; // First byte signals the size or error
};

// Constructs the image
bool image_t_construct(image_t* This, const buffer_t* source);
// Destroys the image
void image_t_destruct (image_t* This);
// Checks wether the struct is ok
bool image_t_OK (const image_t* This);
// Dump info
void image_t_dump_ (const image_t* This, const char name[]);
// Loading data section into the image
size_t image_t_load_data(image_t* This);
size_t image_t_get_jmp (image_t* This, const char source[]);
void image_t_write_offset (image_t* This);
inline void image_t_update_offset (image_t* This);
void image_t_execute (image_t* This);
bool image_t_iterate(image_t* This);
bool image_t_translate(image_t* This);
void image_t_handle_stream(image_t* This);
void image_t_call_handler(image_t* This);
#define CMD(name, key, shift_to_the_right, arguments_type) \
size_t image_t_get_##name(image_t* This, const char source[]);
#include "commands.h"
#undef CMD//*/
void image_t_handle_stream(image_t* This)
{
    /*int counter = 0;
    printf ("Info:\n");
    for (char* i = This->out_stream; counter < 8; i++, counter++){
        printf ("%02X ", (unsigned int)((*i) & 0xFF));
    }
    printf ("Info:\n");
    counter = 0;
    for (char* i = This->in_stream; counter < 8; i++, counter++){
        printf ("%02X ", (unsigned int)((*i) & 0xFF));
    }
    printf ("Info:\n");//*/
    switch (This->out_stream[0])
    {
    case SIG_IN:
        switch (This->out_stream[1])
        {
        case INT:
            printf (ANSI_COLOR_YELLOW "IN" ANSI_COLOR_GREEN "[" "int" "]" ANSI_COLOR_YELLOW ">" ANSI_COLOR_RESET);
            scanf ("%d", (int*)This->in_stream);
            break;
        case FLOAT:
            printf (ANSI_COLOR_YELLOW "IN" ANSI_COLOR_GREEN "[" "float" "]" ANSI_COLOR_YELLOW ">" ANSI_COLOR_RESET);
            scanf ("%f", (float*)This->in_stream);
            break;
        case CHAR:
            printf (ANSI_COLOR_YELLOW "IN" ANSI_COLOR_GREEN "[" "char" "]" ANSI_COLOR_YELLOW ">" ANSI_COLOR_RESET);
            scanf ("%c", (char*)This->in_stream);
            break;
        }
        break;
    case SIG_OUT:
        switch (This->out_stream[1])
        {
        case INT:
            printf (ANSI_COLOR_YELLOW "OUT" ANSI_COLOR_GREEN "[" "int" "]"ANSI_COLOR_YELLOW">" ANSI_COLOR_RESET "%d" "\n", *((int*)(This->out_stream + 2)));
            break;
        case FLOAT:
            printf (ANSI_COLOR_YELLOW "OUT" ANSI_COLOR_GREEN "[" "float" "]"ANSI_COLOR_YELLOW">" ANSI_COLOR_RESET "%f" "\n", *((float*)(This->out_stream + 2)));
            break;
        case CHAR:
            printf (ANSI_COLOR_YELLOW "OUT" ANSI_COLOR_GREEN "[" "char" "]"ANSI_COLOR_YELLOW">" ANSI_COLOR_RESET "%c" "\n", *((char*)(This->out_stream + 2)));
            break;
        }
        break;
    case SIG_STOP:
        This->state = INTERRUPTED;
        printf (ANSI_COLOR_RED "*BEEP-BEEP*\n" ANSI_COLOR_RESET);
        break;
    }
    //__asm__ ("int $0x3;");
}
bool image_t_translate(image_t* This)
{
    // Loading the map
    image_t_iterate(This);
    // Writing all addresses
    return (image_t_iterate(This));
}

void image_t_destruct (image_t* This)
{
    assert (This);
    if (This->map)
        free(This->map);
    This->resume_pos = NULL;
    This->is_mapped = false;
    buffer_t_destruct(&This->binary);
}

bool image_t_OK (const image_t* This)
{
    return (This && buffer_t_OK(&This->binary) && buffer_t_OK(&This->source));
}

void image_t_dump_ (const image_t* This, const char name[])
{
    assert (This);
    DUMP_INDENT += INDENT_VALUE;
    printf ("%s = " ANSI_COLOR_BLUE "image_t" ANSI_COLOR_RESET " (", name);
    if (image_t_OK(This))
        printf (ANSI_COLOR_GREEN "ok" ANSI_COLOR_RESET ")\n");
    else
        printf (ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET ")\n");
    printf(ANSI_COLOR_YELLOW "-----------------------------------------------------" ANSI_COLOR_RESET "\n");
    printf ("%*sis_mapped = %d\n", DUMP_INDENT, "", This->is_mapped);
    printf ("%*smap = %p\n", DUMP_INDENT, "", This->map);
    printf ("%*ssource: ", DUMP_INDENT, "");
    //buffer_t_dump(&This->source);
    printf ("%*sbinary: ", DUMP_INDENT, "");
    //buffer_t_dump(&This->binary);
    printf(ANSI_COLOR_YELLOW "\n-----------------------------------------------------" ANSI_COLOR_RESET "\n");
    DUMP_INDENT -= INDENT_VALUE;
}

bool image_t_construct(image_t* This, const buffer_t* source)
{
    assert(This);
    This->state = STOPPED;
    This->resume_pos = NULL;
    if (!buffer_t_construct_copy(&This->source, source))
        return false;
    This->map = (unsigned*)malloc(This->source.size*sizeof(unsigned));
    if (!This->map){
        perror("image_t_construct: Can't allocate map!");
        return false;
    }
    memset(This->map, 0x0, This->source.size*sizeof(unsigned));
    memset(This->in_stream, 0x0, 8);
    memset(This->out_stream, 0x0, 8);
    This->is_mapped = false;
    return (buffer_t_construct(&This->binary, source->size, true));
}

// In case you want some checks :)
/*if (This->is_mapped){ \
                printf ("Translating: " #name "\n"); \
}//*/
bool image_t_iterate(image_t* This)
{
    ASSERT_OK(image_t, This);
    ASSERT_OK(buffer_t, &This->source);
    buffer_t_destruct(&This->binary);
    buffer_t_construct(&This->binary, 2, 1);
    // Load data section into the image
    size_t pos = image_t_load_data(This);
    while (pos < This->source.size){
        bool is_done = false;
        This->map[pos] = This->binary.size - 14;
        //if (This->is_mapped) printf ("This->source.data[%lu] == %hu\n", pos, This->source.data[pos]);
        #define CMD(name, key, shift_to_the_right, arguments_type) \
        if (!is_done && This->source.data[pos] == key){ \
            pos++;\
            pos += image_t_get_##name(This, This->source.data+pos); \
            is_done = true; \
        }
        #include "commands.h"
        #undef CMD
        /*if (This->is_mapped){
            buffer_t_dump(&This->binary);
            getchar();
        }//*/
    }
    //*/
    This->is_mapped = true;
    image_t_update_offset (This);
    return true;
}

size_t image_t_load_data(image_t* This)
{
    ASSERT_OK(image_t, This);
    #if defined(VERBOSE)
    if (This->is_mapped) printf ("Loading data section...\n");
    #endif // VERBOSE
    //55                   	push   %rbp
    //48 89 e5             	mov    %rsp,%rbp

    char offset_loader[] = {0x55, 0x48, 0x89, 0xe5, 0x49, 0xBE, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    buffer_t_append(&This->binary, offset_loader, 14);
    if (*This->source.data != cmd_jmp){
        printf ("image_t_load_data: Error! The data section is corrupted!\n");
        return 0;
    }
    unsigned jmp_pos = *((unsigned*)(This->source.data+1));
    // Substract first 2 bytes
    size_t nbytes = jmp_pos - 2;
    // Loading the jump
    image_t_get_jmp (This, This->source.data+1);
    // Loading the data itself
    buffer_t_append(&This->binary, This->source.data+2, nbytes);
    return (size_t)jmp_pos;
}

inline void image_t_update_offset (image_t* This)
{
    // Program begins after first 10 bytes of the This->binary
    char* prog_offset = This->binary.data + 14;
    // Loading offset
    memcpy(This->binary.data + 6, (char*)(&prog_offset), sizeof(char*));
}

void image_t_execute (image_t* This)
{
    //buffer_t_dump(&This->binary);
     //uint8_t secret_place = mmap (NULL, nbytes, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    char* executable = mmap (NULL, This->binary.size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    // For debugging
    //memset(executable, This->binary.size, 0xC3);
    memcpy(executable, This->binary.data, This->binary.size);
    // Program begins after first 10 bytes of the This->binary
    char* prog_offset = executable + 14;
    // Loading offset
    memcpy(executable + 6, (char*)(&prog_offset), sizeof(char*));
    void (*inject)() = (void(*)())executable;
    //__asm__ ("int $0x3;");
    This->state = RUNNING;
    inject();
    munmap(executable, This->binary.size);
    //printf ("Done!\n");
}

// Some magic: (This->flags & (0xFF ^ 0x3)) resets last 2 bits of flags (cmp flags)
// Then we can set them again with (This->flags | FLAG)
#define CMP(_name, _intel_codes) \
bool image_t_get_ ## _name (image_t* This, const char source[]) \
{\
    char intel_opcodes[] = {_intel_codes}; \
    buffer_t_append(&This->binary, intel_opcodes, sizeof(intel_opcodes));\
    return 0; \
}

//44 8b 6c 24 04        mov    0x4(%rsp),%r13d
//44 39 2c 24           cmp    %r13d,(%rsp)
//48 83 c4 08           add    $0x8,%rsp
size_t image_t_get_cmp (image_t* This, const char source[])
{
    char intel_opcodes[] = {0x44, 0x8b, 0x6c, 0x24, 0x04, 0x44, 0x39, 0x2c, 0x24, 0x48, 0x83, 0xc4, 0x08};
    buffer_t_append(&This->binary, intel_opcodes, sizeof(intel_opcodes));
    return 0;
}
//49 89 ed             	mov    %rbp,%r13
//48 89 e5             	mov    %rsp,%rbp
//f3 0f 10 45 00       	movss  0x0(%rbp),%xmm0
//48 83 c4 08          	add    $0x8,%rsp
//0f 2e 45 fc          	ucomiss -0x4(%rbp),%xmm0
//4c 89 ed             	mov    %r13,%rbp
size_t image_t_get_fcmp (image_t* This, const char source[])
{
    char intel_opcodes[] = {0x49, 0x89, 0xed, 0x48, 0x89, 0xe5, 0xf3, 0x0f, 0x10, 0x45, 0x00, 0x48, 0x83, 0xc4, 0x08, 0x0f, 0x2e, 0x45, 0xfc, 0x4c, 0x89, 0xed};
    buffer_t_append(&This->binary, intel_opcodes, sizeof(intel_opcodes));
    return 0;
}
//44 8a 2c 24          	mov    (%rsp),%r13b
//44 3a 6c 24 01       	cmp    0x1(%rsp),%r13b
//48 83 c4 02          	add    $0x2,%rsp
size_t image_t_get_ccmp (image_t* This, const char source[])
{
    char intel_opcodes[] = {0x44, 0x8a, 0x2c, 0x24, 0x44, 0x3a, 0x6c, 0x24, 0x01, 0x48, 0x83, 0xc4, 0x02};
    buffer_t_append(&This->binary, intel_opcodes, sizeof(intel_opcodes));
    return 0;
}

size_t image_t_get_jmp (image_t* This, const char source[])
{
    // The offset is stored in r11. We'll load effective addresses using it
    char intel_load[] = {0x4D, 0x8D, 0xAE}; // lea    0x...(%r14),%r13
    char intel_jmp[] = {0x41, 0xFF, 0xE5}; // jmpq   *%r13
    // Loading the position
    unsigned jmp_pos = This->map[*((unsigned*)source)];
    // Loading offset into
    buffer_t_append(&This->binary, intel_load, sizeof(intel_load));
    buffer_t_append(&This->binary, (char*)(&jmp_pos), sizeof(unsigned));
    buffer_t_append(&This->binary, intel_jmp, sizeof(intel_jmp));

    // We've moved 4 bytes forward
    return (sizeof(unsigned));
}

//4d 8d ae ff ff ff 0f    lea    0x.......(%r14),%r13
//41 ff e5                jmpq   *%r13
#define CON_JUMP(_name, _jmp_code) \
size_t image_t_get_ ## _name (image_t* This, const char source[])\
{\
    char intel_con_jump[] = {_jmp_code, 0x0a}; \
    char intel_load[] = {0x4D, 0x8D, 0xAE}; \
    char intel_jmp[] = {0x41, 0xFF, 0xE5}; \
    unsigned jmp_pos = This->map[*((unsigned*)source)];\
    buffer_t_append(&This->binary, intel_con_jump, sizeof(intel_con_jump));\
    buffer_t_append(&This->binary, intel_load, sizeof(intel_load));\
    buffer_t_append(&This->binary, (char*)(&jmp_pos), sizeof(unsigned));\
    buffer_t_append(&This->binary, intel_jmp, sizeof(intel_jmp));\
\
    return (sizeof(unsigned));\
}
// 0x0a is to jump over the jmp command
//77 0a                   ja    ...+10
CON_JUMP (ja, 0x77)
//73 0a                   jae    ...+10
CON_JUMP (jae, 0x73)
//72 0a                   jb    ...+10
CON_JUMP (jb, 0x72)
//76 0a                   jae    ...+10
CON_JUMP (jbe, 0x76)
//74 0a                   je    ...+10
CON_JUMP (je, 0x74)
//75 0a                   jne    ...+10
CON_JUMP (jne, 0x75)

//4D 8D AE                lea    0x...(%r14),%r13
//48 83 ec 04             sub    $0x4,%rsp
//c7 04 24 .. .. .. ..    movl   $0x...,(%rsp)
//41`FF E5                jmpq   *%r13
size_t image_t_get_call(image_t* This, const char source[])
{
    // The offset is stored in r11. We'll load effective addresses using it
    char intel_load[] = {0x4D, 0x8D, 0xAE}; // lea    0x...(%r14),%r13
    char intel_jmp[] = {0x41, 0xFF, 0xE5}; // jmpq   *%r13
    // Loading the position
    unsigned jmp_pos = This->map[*((unsigned*)source)];
    // Loading offset into
    buffer_t_append(&This->binary, intel_load, sizeof(intel_load));
    buffer_t_append(&This->binary, (char*)(&jmp_pos), sizeof(unsigned));
    char intel_push_addr[] = {0x48, 0x83, 0xec, 0x04, 0xc7, 0x04, 0x24};
    buffer_t_append(&This->binary, intel_push_addr, sizeof(intel_push_addr));
    // Pushing the return position
    unsigned ret_pos = (unsigned)This->binary.size - 15 + 3 + 5;
    buffer_t_append(&This->binary, (char*)(&ret_pos), sizeof(unsigned));
    buffer_t_append(&This->binary, intel_jmp, sizeof(intel_jmp));
    // We've moved 4 bytes forward
    return (sizeof(unsigned));
}

size_t image_t_get_err(image_t* This, const char source[])
{
    char load_out_stream[] = {0x49, 0xbd};
    buffer_t_append(&This->binary, load_out_stream, sizeof(load_out_stream));
    buffer_t_append(&This->binary, (char*)(&This->out_stream), sizeof(char*));
    char load_mod[] = {0x66, 0x41, 0xc7, 0x45, 0x00};
    buffer_t_append(&This->binary, load_mod, sizeof(load_mod));
    char mod[] = {SIG_STOP, 0x0};
    buffer_t_append(&This->binary, mod, sizeof(mod));
    image_t_call_handler(This);

    return 0;
}

//4d 31 ed                xor    %r13,%r13
//44 8b 2c 24             mov    (%rsp),%r13d
//48 83 c4 04             add    $0x4,%rsp
//4f 8d 6c 35 00          lea    0x0(%r13,%r14,1),%r13
//41 ff e5                jmpq   *%r13

size_t image_t_get_ret(image_t* This, const char source[])
{
    char intel_opcodes[] = {0x4d, 0x31, 0xed, 0x44, 0x8b, 0x2c, 0x24, 0x48, 0x83, 0xc4, 0x04, 0x4f, 0x8d, 0x6c, 0x35, 0x00, 0x41, 0xff, 0xe5};
    buffer_t_append(&This->binary, intel_opcodes, sizeof(intel_opcodes));

    return 0;
}
size_t image_t_get_stop(image_t* This, const char source[])
{
    //8 89 ec             	mov    %rbp,%rsp
    //5d                   	pop    %rbp
    char stop[] = {0x48, 0x89, 0xec, 0x5d, 0xc3};
    buffer_t_append(&This->binary, stop, sizeof(stop));
    return 0;
}
// Overloaded
size_t image_t_get_load(image_t* This, const char source[])
{
    (void)This;
    (void)source;
    return 0;
}
// Overloaded
size_t image_t_get_store(image_t* This, const char source[])
{
    (void)This;
    (void)source;
    return 0;
}
// Overloaded
size_t image_t_get_pop(image_t* This, const char source[])
{
    (void)This;
    (void)source;
    return 0;
}
// Overloaded
size_t image_t_get_push (image_t* This, const char source[])
{
    (void)This;
    (void)source;
    return 0;
}

//44 8b 3c 24             mov    (%rsp),%r15d
//48 83 c4 04             add    $0x4,%rsp
//4d 8d ae .. .. .. ..    lea    0xfffffff(%r14),%r13
//45 89 7d 00             mov    %r15d,0x0(%r13)
size_t image_t_get_pop_mem_dword(image_t* This, const char source[])
{
    char first_part[] = {0x44, 0x8b, 0x3c, 0x24, 0x48, 0x83, 0xc4, 0x04, 0x4d, 0x8d, 0xae};
    char second_part[] = {0x45, 0x89, 0x7d, 0x00};
    unsigned store_pos = *((unsigned*)source);
    buffer_t_append(&This->binary, first_part, sizeof(first_part));
    buffer_t_append(&This->binary, (char*)(&store_pos), sizeof(unsigned));
    buffer_t_append(&This->binary, second_part, sizeof(second_part));

    return sizeof(unsigned);
}

//66 44 8b 3c 24          mov    (%rsp),%r15w
//48 83 c4 02             add    $0x2,%rsp
//4d 8d ae .. .. .. ..    lea    0x......(%r14),%r13
//66 45 89 7d 00          mov    %r15w,0x0(%r13)
size_t image_t_get_pop_mem_word(image_t* This, const char source[])
{
    char first_part[] = {0x66, 0x44, 0x8b, 0x3c, 0x24, 0x48, 0x83, 0xc4, 0x02, 0x4d, 0x8d, 0xae};
    char second_part[] = {0x66, 0x45, 0x89, 0x7d, 0x00};
    unsigned store_pos = *((unsigned*)source);
    buffer_t_append(&This->binary, first_part, sizeof(first_part));
    buffer_t_append(&This->binary, (char*)(&store_pos), sizeof(unsigned));
    buffer_t_append(&This->binary, second_part, sizeof(second_part));

    return sizeof(unsigned);
}
//44 8a 3c 24             mov    (%rsp),%r15b
//48 83 c4 01             add    $0x1,%rsp
//4d 8d ae .. .. .. ..    lea    0x........(%r14),%r13
//45 88 7d 00             mov    %r15b,0x0(%r13)
size_t image_t_get_pop_mem_byte(image_t* This, const char source[])
{
    char first_part[] = {0x44, 0x8a, 0x3c, 0x24, 0x48, 0x83, 0xc4, 0x01, 0x4d, 0x8d, 0xae};
    char second_part[] = {0x45, 0x88, 0x7d, 0x00};
    unsigned store_pos = *((unsigned*)source);
    buffer_t_append(&This->binary, first_part, sizeof(first_part));
    buffer_t_append(&This->binary, (char*)(&store_pos), sizeof(unsigned));
    buffer_t_append(&This->binary, second_part, sizeof(second_part));

    return sizeof(unsigned);
}

//4d 8d ae .. .. .. ..    lea    0x........(%r14),%r13
//48 83 ec 04             sub    $0x4,%rsp
//45 8b 7d 00             mov    0x0(%r13),%r15d
//44 89 3c 24             mov    %r15d,(%rsp)
size_t image_t_get_push_mem_dword(image_t* This, const char source[])
{
    char first_part[] = {0x4d, 0x8d, 0xae};
    char second_part[] = {0x48, 0x83, 0xec, 0x04, 0x45, 0x8b, 0x7d, 0x00, 0x44, 0x89, 0x3c, 0x24};
    unsigned store_pos = *((unsigned*)source);
    buffer_t_append(&This->binary, first_part, sizeof(first_part));
    buffer_t_append(&This->binary, (char*)(&store_pos), sizeof(unsigned));
    buffer_t_append(&This->binary, second_part, sizeof(second_part));

    return sizeof(unsigned);
}

//4d 8d ae .. .. .. ..    lea    0x.........(%r14),%r13
//48 83 ec 02             sub    $0x2,%rsp
//66 45 8b 7d 00          mov    0x0(%r13),%r15w
//66 44 89 3c 24          mov    %r15w,(%rsp)
size_t image_t_get_push_mem_word(image_t* This, const char source[])
{
    char first_part[] = {0x4d, 0x8d, 0xae};
    char second_part[] = {0x48, 0x83, 0xec, 0x02, 0x66, 0x45, 0x8b, 0x7d, 0x00, 0x66, 0x44, 0x89, 0x3c, 0x24};
    unsigned store_pos =*((unsigned*)source);
    buffer_t_append(&This->binary, first_part, sizeof(first_part));
    buffer_t_append(&This->binary, (char*)(&store_pos), sizeof(unsigned));
    buffer_t_append(&This->binary, second_part, sizeof(second_part));

    return sizeof(unsigned);
}

//4d 8d ae .. .. .. ..    lea    0x........(%r14),%r13
//48 83 ec 01             sub    $0x1,%rsp
//45 8a 7d 00             mov    0x0(%r13),%r15b
//44 88 3c 24             mov    %r15b,(%rsp)
size_t image_t_get_push_mem_byte(image_t* This, const char source[])
{
    char first_part[] = {0x4d, 0x8d, 0xae};
    char second_part[] = {0x48, 0x83, 0xec, 0x01, 0x45, 0x8a, 0x7d, 0x00, 0x44, 0x88, 0x3c, 0x24};
    unsigned store_pos = *((unsigned*)source);
    buffer_t_append(&This->binary, first_part, sizeof(first_part));
    buffer_t_append(&This->binary, (char*)(&store_pos), sizeof(unsigned));
    buffer_t_append(&This->binary, second_part, sizeof(second_part));

    return sizeof(unsigned);
}

//48 83 ec 04             sub    $0x4,%rsp
//c7 04 24 .. .. .. ..    movl   $0x........,(%rsp)
size_t image_t_get_push_int(image_t* This, const char source[])
{
    char intel_opcodes[] = {0x48, 0x83, 0xec, 0x04, 0xc7, 0x04, 0x24};
    int value = *((int*)source);
    buffer_t_append(&This->binary, intel_opcodes, sizeof(intel_opcodes));
    buffer_t_append(&This->binary, (char*)(&value), sizeof(int));

    return sizeof(int);
}

//48 83 ec 04             sub    $0x4,%rsp
//c7 04 24 .. .. .. ..    movl   $0x........,(%rsp)
size_t image_t_get_push_float(image_t* This, const char source[])
{
    char intel_opcodes[] = {0x48, 0x83, 0xec, 0x04, 0xc7, 0x04, 0x24};
    float value = *((float*)source);
    buffer_t_append(&This->binary, intel_opcodes, sizeof(intel_opcodes));
    buffer_t_append(&This->binary, (char*)(&value), sizeof(float));

    return sizeof(float);
}

//48 83 ec 01             sub    $0x1,%rsp
//c6 04 24 ..             movb   $0x..,(%rsp)
size_t image_t_get_push_char(image_t* This, const char source[])
{
    char intel_opcodes[] = {0x48, 0x83, 0xec, 0x01, 0xc6, 0x04, 0x24};
    char value = *((char*)source);
    buffer_t_append(&This->binary, intel_opcodes, sizeof(intel_opcodes));
    buffer_t_append(&This->binary, (char*)(&value), sizeof(char));

    return sizeof(char);
}

//44 8b 2c 24             mov    (%rsp),%r13d
//48 83 ec 04             sub    $0x4,%rsp
//44 89 2c 24             mov    %r13d,(%rsp)
size_t image_t_get_dworddup (image_t* This, const char source[])
{
    char intel_opcode[] = {0x44, 0x8b, 0x2c, 0x24, 0x48, 0x83, 0xec, 0x04, 0x44, 0x89, 0x2c, 0x24};
    buffer_t_append(&This->binary, intel_opcode, sizeof(intel_opcode));
    return 0;
}

//66 44 8b 2c 24          mov    (%rsp),%r13w
//48 83 ec 02             sub    $0x2,%rsp
//66 44 89 2c 24          mov    %r13w,(%rsp)
size_t image_t_get_worddup (image_t* This, const char source[])
{
    char intel_opcode[] = {0x66, 0x44, 0x8b, 0x2c, 0x24, 0x48, 0x83, 0xec, 0x02, 0x66, 0x44, 0x89, 0x2c, 0x24};
    buffer_t_append(&This->binary, intel_opcode, sizeof(intel_opcode));
    return 0;
}

//44 8a 2c 24             mov    (%rsp),%r13b
//48 83 ec 01             sub    $0x1,%rsp
//44 88 2c 24             mov    %r13b,(%rsp)
size_t image_t_get_bytedup (image_t* This, const char source[])
{
    char intel_opcode[] = {0x44, 0x8a, 0x2c, 0x24, 0x48, 0x83, 0xec, 0x01, 0x44, 0x88, 0x2c, 0x24};
    buffer_t_append(&This->binary, intel_opcode, sizeof(intel_opcode));
    return 0;
}

//44 8b 2c 24             mov    (%rsp),%r13
//48 83 ec 08             sub    $0x8,%rsp
//44 89 2c 24             mov    %r13d,(%rsp)
size_t image_t_get_dworddupd (image_t* This, const char source[])
{
    char intel_opcode[] = {0x44, 0x8b, 0x2c, 0x24, 0x48, 0x83, 0xec, 0x08, 0x44, 0x89, 0x2c, 0x24};
    buffer_t_append(&This->binary, intel_opcode, sizeof(intel_opcode));
    return 0;
}

//44 8b 2c 24             mov    (%rsp),%r13d
//48 83 ec 04             sub    $0x4,%rsp
//44 89 2c 24             mov    %r13d,(%rsp)
size_t image_t_get_worddupd (image_t* This, const char source[])
{
    char intel_opcode[] = {0x44, 0x8b, 0x2c, 0x24, 0x48, 0x83, 0xec, 0x04, 0x44, 0x89, 0x2c, 0x24};
    buffer_t_append(&This->binary, intel_opcode, sizeof(intel_opcode));
    return 0;
}

//66 44 8b 2c 24          mov    (%rsp),%r13w
//48 83 ec 02             sub    $0x2,%rsp
//66 44 89 2c 24          mov    %r13w,(%rsp)
size_t image_t_get_bytedupd (image_t* This, const char source[])
{
    char intel_opcode[] = {0x66, 0x44, 0x8b, 0x2c, 0x24, 0x48, 0x83, 0xec, 0x02, 0x66, 0x44, 0x89, 0x2c, 0x24};
    buffer_t_append(&This->binary, intel_opcode, sizeof(intel_opcode));
    return 0;
}

//8b .. 24                mov    (%rsp),%...
//48 83 c4 04             add    $0x4,%rsp
size_t image_t_get_pop_reg_dword(image_t* This, const char source[])
{
    char reg_addr = (*source)/4*8 + 4;
    char intel_opcodes[] = {0x8b, 0x0, 0x24, 0x48, 0x83, 0xc4, 0x04};
    memcpy(intel_opcodes + 1, &reg_addr, 1);
    buffer_t_append(&This->binary, intel_opcodes, sizeof(intel_opcodes));

    return sizeof(char);
}

//66 8b .. 24             mov    (%rsp),%...
//48 83 c4 02             add    $0x2,%rsp
size_t image_t_get_pop_reg_word(image_t* This, const char source[])
{
    char reg_addr = (*source)/4*8 + 4;
    char intel_opcodes[] = {0x66, 0x8b, 0x0, 0x24, 0x48, 0x83, 0xc4, 0x02};
    memcpy(intel_opcodes + 2, &reg_addr, 1);
    buffer_t_append(&This->binary, intel_opcodes, sizeof(intel_opcodes));

    return sizeof(char);
}


//40 8a .. 24             mov    (%rsp),%...
//48 83 c4 01             add    $0x1,%rsp
size_t image_t_get_pop_reg_byte(image_t* This, const char source[])
{
    char reg_addr = (*source)/4*8 + 4;
    if (reg_addr > 3*8 + 4){
        char mod[] = {0x40};
        buffer_t_append(&This->binary, mod, sizeof(mod));
    }
    char intel_opcodes[] = {0x8a, 0x0, 0x24, 0x48, 0x83, 0xc4, 0x01};
    memcpy(intel_opcodes + 1, &reg_addr, 1);
    buffer_t_append(&This->binary, intel_opcodes, sizeof(intel_opcodes));

    return sizeof(char);
}

//48 83 ec 04             sub    $0x4,%rsp
//89 .. 24                mov    %..,(%rsp)
size_t image_t_get_push_reg_dword(image_t* This, const char source[])
{
    char reg_addr = (*source)/4*8 + 4;
    char intel_opcodes[] = {0x48, 0x83, 0xec, 0x04, 0x89, 0x0, 0x24};
    memcpy(intel_opcodes + 5, &reg_addr, 1);
    buffer_t_append(&This->binary, intel_opcodes, sizeof(intel_opcodes));

    return sizeof(char);
}

//48 83 ec 02             sub    $0x2,%rsp
//66 89 .. 24             mov    %..,(%rsp)
size_t image_t_get_push_reg_word(image_t* This, const char source[])
{
    char reg_addr = (*source)/4*8 + 4;
    char intel_opcodes[] = {0x48, 0x83, 0xec, 0x04, 0x66, 0x89, 0x0, 0x24};
    memcpy(intel_opcodes + 6, &reg_addr, 1);
    buffer_t_append(&This->binary, intel_opcodes, sizeof(intel_opcodes));

    return sizeof(char);
}

//48 83 ec 01             sub    $0x1,%rsp
//88 .. 24                mov    %..,(%rsp)
size_t image_t_get_push_reg_byte(image_t* This, const char source[])
{
    char reg_addr = (*source)/4*8 + 4;
    char first_part[] = {0x48, 0x83, 0xec, 0x01};
    buffer_t_append(&This->binary, first_part, sizeof(first_part));
    if (reg_addr > 3*8 + 4){
        char mod[] = {0x40};
        buffer_t_append(&This->binary, mod, sizeof(mod));
    }
    char intel_opcodes[] = {0x88, 0x0, 0x24};
    memcpy(intel_opcodes + 1, &reg_addr, 1);
    buffer_t_append(&This->binary, intel_opcodes, sizeof(intel_opcodes));

    return sizeof(char);
}

#define ARITHM(_name, _op, _type) \
bool image_t_get_ ## _name (image_t* This, const char source[]) \
{ \
    ASSERT_OK(image_t, This); \
    _type a = 0, b = 0, res = 0; \
    \
    if (!stack_t_pop(&This->stack, &a, sizeof(_type))){ \
        image_t_get_destruct(This); \
        ASSERT_OK(image_t, This); \
        return false; \
    } \
    if (!stack_t_pop(&This->stack, &b, sizeof(_type))){ \
        image_t_get_destruct(This); \
        ASSERT_OK(image_t, This); \
        return false; \
    } \
    res = a _op b; \
    *(unsigned*)(This->registers+ESP) += sizeof(_type);\
    ASSERT_OK(image_t, This); \
    return stack_t_push(&This->stack, &res, sizeof(_type)); \
}

//44 8b 2c 24             mov    (%rsp),%r13d
//44 03 6c 24 04          add    0x4(%rsp),%r13d
//48 83 c4 04             add    $0x4,%rsp
//44 89 2c 24             mov    %r13d,(%rsp)
size_t image_t_get_add(image_t* This, const char source[])
{
    char intel_opcode[] = {0x44, 0x8b, 0x2c, 0x24, 0x44, 0x03, 0x6c, 0x24, 0x04, 0x48, 0x83, 0xc4, 0x04, 0x44, 0x89, 0x2c, 0x24};
    buffer_t_append(&This->binary, intel_opcode, sizeof(intel_opcode));
    return 0;
}

//44 8b 2c 24             mov    (%rsp),%r13d
//44 2b 6c 24 04          sub    0x4(%rsp),%r13d
//48 83 c4 04             add    $0x4,%rsp
//44 89 2c 24             mov    %r13d,(%rsp)
size_t image_t_get_sub(image_t* This, const char source[])
{
    char intel_opcode[] = {0x44, 0x8b, 0x2c, 0x24, 0x44, 0x2b, 0x6c, 0x24, 0x04, 0x48, 0x83, 0xc4, 0x04, 0x44, 0x89, 0x2c, 0x24};
    buffer_t_append(&This->binary, intel_opcode, sizeof(intel_opcode));
    return 0;
}

//44 8b 2c 24             mov    (%rsp),%r13d
//44 0f af 6c 24 04       imul   0x4(%rsp),%r13d
//48 83 c4 04             add    $0x4,%rsp
//44 89 2c 24             mov    %r13d,(%rsp)
size_t image_t_get_mul(image_t* This, const char source[])
{
    char intel_opcode[] = {0x44, 0x8b, 0x2c, 0x24, 0x44, 0x0f, 0xaf, 0x6c, 0x24, 0x04, 0x48, 0x83, 0xc4, 0x04, 0x44, 0x89, 0x2c, 0x24};
    buffer_t_append(&This->binary, intel_opcode, sizeof(intel_opcode));
    return 0;
}

//49 89 c5                mov    %rax,%r13
//48 31 c0                xor    %rax,%rax
//8b 04 24                mov    (%rsp),%eax
//f7 7c 24 04             idivl  0x4(%rsp)
//48 83 c4 04             add    $0x4,%rsp
//89 04 24                mov    %eax,(%rsp)
//4c 89 e8                mov    %r13,%rax
size_t image_t_get_div(image_t* This, const char source[])
{
    char intel_opcode[] = {0x49, 0x89, 0xc5, 0x49, 0x89, 0xd7, 0x48, 0x31, 0xc0, 0x8b, 0x04, 0x24, 0xf7, 0x7c, 0x24, 0x04, 0x48, 0x83, 0xc4, 0x04, 0x89, 0x04, 0x24, 0x4c, 0x89, 0xfa, 0x4c, 0x89, 0xe8};
    buffer_t_append(&This->binary, intel_opcode, sizeof(intel_opcode));
    return 0;
}

//49 89 c5                mov    %rax,%r13
//49 89 d7                mov    %rdx,%r15
//48 31 c0                xor    %rax,%rax
//8b 04 24                mov    (%rsp),%eax
//f7 7c 24 04             idivl  0x4(%rsp)
//48 83 c4 04             add    $0x4,%rsp
//89 14 24                mov    %edx,(%rsp)
//4c 89 fa                mov    %r15,%rdx
//4c 89 e8                mov    %r13,%rax
size_t image_t_get_mod(image_t* This, const char source[])
{
    char intel_opcode[] = {0x49, 0x89, 0xc5, 0x49, 0x89, 0xd7, 0x48, 0x31, 0xc0, 0x8b, 0x04, 0x24, 0xf7, 0x7c, 0x24, 0x04, 0x48, 0x83, 0xc4, 0x04, 0x89, 0x14, 0x24, 0x4c, 0x89, 0xfa, 0x4c, 0x89, 0xe8};
    buffer_t_append(&This->binary, intel_opcode, sizeof(intel_opcode));
    return 0;
}


//f3 0f 10 04 24       	movss  (%rsp),%xmm0
//48 83 c4 04          	add    $0x4,%rsp
//f3 0f 58 04 24       	addss  (%rsp),%xmm0
//f3 0f 11 04 24       	movss  %xmm0,(%rsp)
size_t image_t_get_fadd(image_t* This, const char source[])
{
    char intel_opcode[] = {0xf3, 0x0f, 0x10, 0x04, 0x24, 0x48, 0x83, 0xc4, 0x04, 0xf3, 0x0f, 0x58, 0x04, 0x24, 0xf3, 0x0f, 0x11, 0x04, 0x24};
    buffer_t_append(&This->binary, intel_opcode, sizeof(intel_opcode));
    return 0;
}

//f3 0f 10 04 24       	movss  (%rsp),%xmm0
//48 83 c4 04          	add    $0x4,%rsp
//f3 0f 5c 04 24       	subss  (%rsp),%xmm0
//f3 0f 11 04 24       	movss  %xmm0,(%rsp)
size_t image_t_get_fsub(image_t* This, const char source[])
{
    char intel_opcode[] = {0xf3, 0x0f, 0x10, 0x04, 0x24, 0x48, 0x83, 0xc4, 0x04, 0xf3, 0x0f, 0x5c, 0x04, 0x24, 0xf3, 0x0f, 0x11, 0x04, 0x24};
    buffer_t_append(&This->binary, intel_opcode, sizeof(intel_opcode));
    return 0;
}

//f3 0f 10 04 24       	movss  (%rsp),%xmm0
//48 83 c4 04          	add    $0x4,%rsp
//f3 0f 59 04 24       	mulss  (%rsp),%xmm0
//f3 0f 11 04 24       	movss  %xmm0,(%rsp)
size_t image_t_get_fmul(image_t* This, const char source[])
{
    char intel_opcode[] = {0xf3, 0x0f, 0x10, 0x04, 0x24, 0x48, 0x83, 0xc4, 0x04, 0xf3, 0x0f, 0x59, 0x04, 0x24, 0xf3, 0x0f, 0x11, 0x04, 0x24};
    buffer_t_append(&This->binary, intel_opcode, sizeof(intel_opcode));
    return 0;
}

//f3 0f 10 04 24       	movss  (%rsp),%xmm0
//48 83 c4 04          	add    $0x4,%rsp
//f3 0f 5e 04 24       	divss  (%rsp),%xmm0
//f3 0f 11 04 24       	movss  %xmm0,(%rsp)
size_t image_t_get_fdiv(image_t* This, const char source[])
{
    char intel_opcode[] = {0xf3, 0x0f, 0x10, 0x04, 0x24, 0x48, 0x83, 0xc4, 0x04, 0xf3, 0x0f, 0x5e, 0x04, 0x24, 0xf3, 0x0f, 0x11, 0x04, 0x24};
    buffer_t_append(&This->binary, intel_opcode, sizeof(intel_opcode));
    return 0;
}

size_t image_t_get_ndebug(image_t* This, const char source[])
{
    // Not supported
    return 0;
}

size_t image_t_get_debug(image_t* This, const char source[])
{
    char intel_opcode[] = {0xCC};
    buffer_t_append(&This->binary, intel_opcode, sizeof(intel_opcode));
    return 0;
}

//49 89 ef                mov    %rbp,%r15
//48 83 ec 04             sub    $0x4,%rsp
//c7 04 24 ff ff ff 7f    movl   $0x7fffffff,(%rsp)
//48 89 e5                mov    %rsp,%rbp
//f3 0f 10 45 00       	  movss  0x0(%rbp),%xmm0
//48 83 c4 04             add    $0x4,%rsp
//48 89 e5                mov    %rsp,%rbp
//f3 0f 10 4d 00          movss  0x0(%rbp),%xmm1
//0f 54 c1                andps  %xmm1,%xmm0
//f3 0f 11 45 00          movss  %xmm0,0x0(%rbp)
//4c 89 fd                mov    %r15,%rbp
size_t image_t_get_fabs(image_t* This, const char source[])
{
    char intel_opcode[] = {0x49, 0x89, 0xef, 0x48, 0x83, 0xec, 0x04, 0xc7, 0x04, 0x24, 0xff, 0xff, 0xff, 0x7f, 0x48, 0x89, 0xe5, 0xf3, 0x0f, 0x10, 0x45, 0x00, 0x48, 0x83, 0xc4, 0x04, 0x48, 0x89, 0xe5, 0xf3, 0x0f, 0x10, 0x4d, 0x00, 0x0f, 0x54, 0xc1, 0xf3, 0x0f, 0x11, 0x45, 0x00, 0x4c, 0x89, 0xfd};
    buffer_t_append(&This->binary, intel_opcode, sizeof(intel_opcode));
    return 0;
}

//44 8b 3c 24             mov    (%rsp),%r15d
//41 f7 df                neg    %r15d
//78 fb                   js     <current_pos-4?>
//44 89 3c 24             mov    %r15d,(%rsp)
size_t image_t_get_abs(image_t* This, const char source[])
{
    char intel_opcode[] = {0x44, 0x8b, 0x3c, 0x24, 0x41, 0xf7, 0xdf, 0x78, 0xfb, 0x44, 0x89, 0x3c, 0x24};
    buffer_t_append(&This->binary, intel_opcode, sizeof(intel_opcode));
    return 0;
}

//50                      push   %rax
//51                      push   %rcx
//52                      push   %rdx
//53                      push   %rbx
//56                      push   %rsi
//57                      push   %rdi
//9c                      pushfq
//ff 75 08                pushq  0x8(%rbp)
//48 8b 6d 00             mov    0x0(%rbp),%rbp
//c3                      retq
//9d                      popfq
//5f                      pop    %rdi
//5e                      pop    %rsi
//5b                      pop    %rbx
//5a                      pop    %rdx
//59                      pop    %rcx
//58                      pop    %rax
//49 bd af af af ff fa    movabs $0xfafafaffafafaf,%r13
//41 ff d5             	  callq  *%r13
//48 83 ec 08             sub    $0x8,%rsp
//4c 89 2c 24             mov    %r13,(%rsp)

void image_t_call_handler(image_t* This)
{
    char save_flags[] = {0x50, 0x51, 0x52, 0x53, 0x56, 0x57, 0x9c};
    buffer_t_append(&This->binary, save_flags, sizeof(save_flags));
    char save_addr[] = {0x48, 0xbf};
    buffer_t_append(&This->binary, save_addr, sizeof(save_addr));
    image_t* This_p = This;
    buffer_t_append(&This->binary, (char*)&This_p, sizeof(image_t*));
    char load_call_addr[] = {0x49, 0xbd};
    buffer_t_append(&This->binary, load_call_addr, sizeof(load_call_addr));
    void (*stream_handler)(image_t*) = &image_t_handle_stream;
    buffer_t_append(&This->binary, (char*)&stream_handler, sizeof(char*));
    char call_handler[] = {0x41, 0xff, 0xd5};
    buffer_t_append(&This->binary, call_handler, sizeof(call_handler));
    char load_flags[] = {0x9d, 0x5f, 0x5e, 0x5b, 0x5a, 0x59, 0x58};
    buffer_t_append(&This->binary, load_flags, sizeof(load_flags));
}

//44 8b 3c 24          	            mov    (%rsp),%r15d
//48 83 c4 04          	            add    $0x4,%rsp
//49 bd .. .. .. .. .. .. .. .. 	movabs $0xfffffffafffafaff,%r13
//66 41 c7 45 00 02 00 	            movw   $0x2,0x0(%r13)
//45 89 7d 02          	            mov    %r15d,0x2(%r13)
size_t image_t_get_out(image_t* This, const char source[])
{
    char load_r13[] = {0x44, 0x8b, 0x3c, 0x24, 0x48, 0x83, 0xc4, 0x04};
    buffer_t_append(&This->binary, load_r13, sizeof(load_r13));
    char load_out_stream[] = {0x49, 0xbd};
    buffer_t_append(&This->binary, load_out_stream, sizeof(load_out_stream));
    char* out_stream_address = This->out_stream;
    buffer_t_append(&This->binary, (char*)&out_stream_address, sizeof(char*));
    char load_mod[] = {0x66, 0x41, 0xc7, 0x45, 0x00};
    buffer_t_append(&This->binary, load_mod, sizeof(load_mod));
    char mod[] = {SIG_OUT, INT};
    buffer_t_append(&This->binary, mod, sizeof(mod));
    char load_value[] = {0x45, 0x89, 0x7d, 0x02};
    buffer_t_append(&This->binary, load_value, sizeof(load_value));
    image_t_call_handler(This);

    return 0;
}

//44 8b 3c 24          	            mov    (%rsp),%r15d
//48 83 c4 04          	            add    $0x4,%rsp
//49 bd .. .. .. .. .. .. .. .. 	movabs $0xfffffffafffafaff,%r13
//66 41 c7 45 00 02 00 	            movw   $0x2,0x0(%r13)
//45 89 7d 02          	            mov    %r15d,0x2(%r13)
size_t image_t_get_fout(image_t* This, const char source[])
{
    char load_r13[] = {0x44, 0x8b, 0x3c, 0x24, 0x48, 0x83, 0xc4, 0x04};
    buffer_t_append(&This->binary, load_r13, sizeof(load_r13));
    char load_out_stream[] = {0x49, 0xbd};
    buffer_t_append(&This->binary, load_out_stream, sizeof(load_out_stream));
    char* out_stream_address = This->out_stream;
    buffer_t_append(&This->binary, (char*)&out_stream_address, sizeof(char*));
    char load_mod[] = {0x66, 0x41, 0xc7, 0x45, 0x00};
    buffer_t_append(&This->binary, load_mod, sizeof(load_mod));
    char mod[] = {SIG_OUT, FLOAT};
    buffer_t_append(&This->binary, mod, sizeof(mod));
    char load_value[] = {0x45, 0x89, 0x7d, 0x02};
    buffer_t_append(&This->binary, load_value, sizeof(load_value));
    image_t_call_handler(This);

    return 0;
}

//44 8a 3c 24          	            mov    (%rsp),%r15b
//48 83 c4 01          	            add    $0x1,%rsp
//49 bd .. .. .. .. .. .. .. .. 	movabs $0xfffffffafffafaff,%r13
//66 41 c7 45 00 02 00 	            movw   $0x2,0x0(%r13)
//45 8a 7d 02                    	mov    0x2(%r13),%r15b
size_t image_t_get_cout(image_t* This, const char source[])
{
    char load_r13[] = {0x44, 0x8a, 0x3c, 0x24, 0x48, 0x83, 0xc4, 0x01};
    buffer_t_append(&This->binary, load_r13, sizeof(load_r13));
    char load_out_stream[] = {0x49, 0xbd};
    buffer_t_append(&This->binary, load_out_stream, sizeof(load_out_stream));
    char* out_stream_address = This->out_stream;
    buffer_t_append(&This->binary, (char*)&out_stream_address, sizeof(char*));
    char load_mod[] = {0x66, 0x41, 0xc7, 0x45, 0x00};
    buffer_t_append(&This->binary, load_mod, sizeof(load_mod));
    char mod[] = {SIG_OUT, FLOAT};
    buffer_t_append(&This->binary, mod, sizeof(mod));
    char load_value[] = {0x45, 0x8a, 0x7d, 0x02};
    buffer_t_append(&This->binary, load_value, sizeof(load_value));
    image_t_call_handler(This);

    return 0;
}

//49 bd .. .. .. .. .. .. .. .. 	movabs $0xfffffffafffafaff,%r13
//66 41 c7 45 00 02 00 	            movw   $0x2,0x0(%r13)
//call handler
//49 bd .. .. .. .. .. .. .. .. 	movabs $0xfffffffafffafaff,%r13
//45 8b 6d 00          	            mov    0x0(%r13),%r13d
//48 83 ec 04          	            sub    $0x4,%rsp
//44 89 2c 24          	            mov    %r13d,(%rsp)
size_t image_t_get_in(image_t* This, const char source[])
{
    char load_out_stream[] = {0x49, 0xbd};
    buffer_t_append(&This->binary, load_out_stream, sizeof(load_out_stream));
    char* out_stream_address = This->out_stream;
    buffer_t_append(&This->binary, (char*)&out_stream_address, sizeof(char*));
    char load_mod[] = {0x66, 0x41, 0xc7, 0x45, 0x00};
    buffer_t_append(&This->binary, load_mod, sizeof(load_mod));
    char mod[] = {SIG_IN, INT};
    buffer_t_append(&This->binary, mod, sizeof(mod));
    image_t_call_handler(This);
    char load_in_stream[] = {0x49, 0xbd};
    buffer_t_append(&This->binary, load_in_stream, sizeof(load_in_stream));
    char* in_stream_address = This->in_stream;
    buffer_t_append(&This->binary, (char*)&in_stream_address, sizeof(char*));
    char store_value[] = {0x45, 0x8b, 0x6d, 0x00, 0x48, 0x83, 0xec, 0x04, 0x44, 0x89, 0x2c, 0x24};
    buffer_t_append(&This->binary, store_value, sizeof(store_value));

    return 0;
}

//49 bd .. .. .. .. .. .. .. .. 	movabs $0xfffffffafffafaff,%r13
//66 41 c7 45 00 02 00 	            movw   $0x2,0x0(%r13)
//call handler
//49 bd .. .. .. .. .. .. .. .. 	movabs $0xfffffffafffafaff,%r13
//45 8b 6d 00          	            mov    0x0(%r13),%r13d
//48 83 ec 04          	            sub    $0x4,%rsp
//44 89 2c 24          	            mov    %r13d,(%rsp)
size_t image_t_get_fin(image_t* This, const char source[])
{
    char load_out_stream[] = {0x49, 0xbd};
    buffer_t_append(&This->binary, load_out_stream, sizeof(load_out_stream));
    char* out_stream_address = This->out_stream;
    buffer_t_append(&This->binary, (char*)&out_stream_address, sizeof(char*));
    char load_mod[] = {0x66, 0x41, 0xc7, 0x45, 0x00};
    buffer_t_append(&This->binary, load_mod, sizeof(load_mod));
    char mod[] = {SIG_IN, FLOAT};
    buffer_t_append(&This->binary, mod, sizeof(mod));
    image_t_call_handler(This);
    char load_in_stream[] = {0x49, 0xbd};
    buffer_t_append(&This->binary, load_in_stream, sizeof(load_in_stream));
    char* in_stream_address = This->in_stream;
    buffer_t_append(&This->binary, (char*)&in_stream_address, sizeof(char*));
    char store_value[] = {0x45, 0x8b, 0x6d, 0x00, 0x48, 0x83, 0xec, 0x04, 0x44, 0x89, 0x2c, 0x24};
    buffer_t_append(&This->binary, store_value, sizeof(store_value));

    return 0;
}

//49 bd .. .. .. .. .. .. .. .. 	movabs $0xfffffffafffafaff,%r13
//66 41 c7 45 00 02 00 	            movw   $0x2,0x0(%r13)
//call handler
//49 bd .. .. .. .. .. .. .. .. 	movabs $0xfffffffafffafaff,%r13
//45 8a 6d 00          	            mov    0x0(%r13),%r13b
//48 83 ec 01          	            sub    $0x1,%rsp
//44 88 2c 24          	            mov    %r13b,(%rsp)
size_t image_t_get_cin(image_t* This, const char source[])
{
    char load_out_stream[] = {0x49, 0xbd};
    buffer_t_append(&This->binary, load_out_stream, sizeof(load_out_stream));
    buffer_t_append(&This->binary, (char*)&(This->out_stream), sizeof(char*));
    char load_mod[] = {0x66, 0x41, 0xc7, 0x45, 0x00};
    buffer_t_append(&This->binary, load_mod, sizeof(load_mod));
    char mod[] = {SIG_IN, CHAR};
    buffer_t_append(&This->binary, mod, sizeof(mod));
    image_t_call_handler(This);
    char load_in_stream[] = {0x49, 0xbd};
    buffer_t_append(&This->binary, load_in_stream, sizeof(load_in_stream));
    char* in_stream_address = This->in_stream;
    buffer_t_append(&This->binary, (char*)&in_stream_address, sizeof(char*));
    char store_value[] = {0x45, 0x8a, 0x6d, 0x00, 0x48, 0x83, 0xec, 0x01, 0x44, 0x88, 0x2c, 0x24};
    buffer_t_append(&This->binary, store_value, sizeof(store_value));

    return 0;
}
#endif  // IMAGE_T_H_INCLUDED
