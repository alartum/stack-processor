#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "mylib.h"

#ifndef BUFFER_H_INCLUDED
#define BUFFER_H_INCLUDED

#define BUFFER_LINE_SIZE 16
#define BUFFER_DEFAULT_MULT 1.4

#define buffer_t_dump(This) buffer_t_dump_(This, #This)

typedef struct buffer_t buffer_t;
struct buffer_t
{
    // Allocate space if not enough
    bool do_alloc;
    // Number of times the buffer space will be increased during allocation
    double alloc_mult;
    // Array of symbols
    char* data;
    // Pointer to the end
    char* end;
    // Length of the array (amount of symbols)
    size_t size;
    // Maximum amount of bytes
    size_t max_size;
};

void buffer_t_destruct (buffer_t* This);
bool buffer_t_construct_filename (buffer_t* This, const char filename[]);
bool buffer_t_construct_file (buffer_t* This, FILE* f);
bool buffer_t_construct (buffer_t* This, size_t nbytes, bool do_alloc);
bool buffer_t_construct_copy (buffer_t* This, const buffer_t* other);
bool buffer_t_append (buffer_t* This, const char* data, size_t nbytes);
bool buffer_t_OK (const buffer_t* This);
void buffer_t_dump_ (const buffer_t* This, const char name[]);

void buffer_t_destruct (buffer_t* This)
{
    assert (This);
    if (This->data)
        free (This->data);
    // Just to be sure
    This->data = NULL;
    This->end = NULL;
    This->size = 0;
    This->max_size = 0;
    This->do_alloc = false;
    This->alloc_mult = 0;
}

bool buffer_t_OK (const buffer_t* This)
{
    assert(This);
    return This && This->data && This->end && (This->size <= This->max_size);
}

void buffer_t_dump_ (const buffer_t* This, const char name[])
{
    assert (This);
    DUMP_INDENT += INDENT_VALUE;
    printf ("%s = " ANSI_COLOR_BLUE "buffer_t" ANSI_COLOR_RESET " (", name);
    if (buffer_t_OK(This))
        printf (ANSI_COLOR_GREEN "ok" ANSI_COLOR_RESET ")\n");
    else
        printf (ANSI_COLOR_RED "ERROR" ANSI_COLOR_RESET ")\n");
    printf(ANSI_COLOR_YELLOW "-----------------------------------------------------" ANSI_COLOR_RESET "\n");
    printf ("%*smax_size = %lu\n", DUMP_INDENT, "", This->max_size);
    printf ("%*ssize = %lu\n", DUMP_INDENT, "", This->size);
    printf ("%*sdo_alloc = %d\n", DUMP_INDENT, "", This->do_alloc);
    printf ("%*salloc_mult = %lf\n", DUMP_INDENT, "", This->alloc_mult);
    printf ("%*sdata = %p\n", DUMP_INDENT, "", This->data);
    printf ("%*send  = %p\n", DUMP_INDENT, "", This->end);
    printf ("%*sdata: ", DUMP_INDENT, "");
    if (This->size == 0)
        printf (ANSI_COLOR_RED "[EMPTY]" ANSI_COLOR_RESET);
    else{
        // If set to true, will print chars if possible
        char line[4*BUFFER_LINE_SIZE+3];
        memset(line, ' ', 4*BUFFER_LINE_SIZE+2);
        line[3*(BUFFER_LINE_SIZE)] = '|';
        line[4*BUFFER_LINE_SIZE+2] = '\0';
        size_t counter = 0;
        for (char* i = This->data; counter < This->size; i++, counter++){
            if (!(counter % BUFFER_LINE_SIZE) && counter){
                printf ("\n%*s", DUMP_INDENT, "");
                printf ("[%6lu]%#10x:    ",  counter - BUFFER_LINE_SIZE, (unsigned)(counter - BUFFER_LINE_SIZE));
                printf ("%s", line);
                memset(line, ' ', 4*BUFFER_LINE_SIZE+2);
                line[3*(BUFFER_LINE_SIZE)] = '|';
                //getchar();
            }
            char hex_num[4];
            sprintf (hex_num, "%02X ", (unsigned int)((*i) & 0xFF));
            memcpy(line + 3*(counter % BUFFER_LINE_SIZE), hex_num, 3);
            line[3*(BUFFER_LINE_SIZE) + 2+(counter % BUFFER_LINE_SIZE)] = (*i >= 32)? *i:'.'; // The first char has number 32
        }
        printf ("\n%*s", DUMP_INDENT, "");
        printf ("[%6lu]%#10x:    ",  counter - BUFFER_LINE_SIZE, (unsigned)(counter - BUFFER_LINE_SIZE));
        printf ("%s", line);
    }
    printf(ANSI_COLOR_YELLOW "\n-----------------------------------------------------" ANSI_COLOR_RESET "\n");
    DUMP_INDENT -= INDENT_VALUE;
}

bool buffer_t_construct_file (buffer_t* This, FILE* f)
{
    assert(This);
    assert(f);
    struct stat st;
    fstat(fileno(f), &st);
    buffer_t_construct(This, st.st_size, 0);

    This->size = This->max_size;
    if (fread (This->data, 1, This->size, f) != This->size){
        errno = EIO;
        perror("buffer_t_construct:");
        buffer_t_destruct (This);
        fclose (f);
        return false;
    }

    return true;
}

bool buffer_t_construct_filename (buffer_t* This, const char filename[])
{
    assert(This);
    FILE *f = fopen (filename, "rb");
    if (!f){ // fopen() writes to errno automatically
        perror("buffer_t_construct: (can't open file)");
        buffer_t_destruct (This);
        return false;
    }
    buffer_t_construct_file(This, f);
    if (fclose (f)){
        perror("buffer_t_construct: (can't close file)");
        buffer_t_destruct (This);
        return false;
    }

    return true;
}

bool buffer_t_construct (buffer_t* This, size_t max_size, bool do_alloc)
{
    assert(This);
    This->size = 0;
    This->max_size = max_size;
    This->data = (char*)malloc(This->max_size);
    if (!This->data){
        errno = ENOMEM;
        perror("buffer_t_construct_empty:");
        buffer_t_destruct (This);
        return false;
    }
    This->end = This->data + This->size - 1;
    memset (This->data, 0, This->max_size);
    This->do_alloc = do_alloc;
    This->alloc_mult = BUFFER_DEFAULT_MULT;

    return true;
}

bool buffer_t_construct_copy (buffer_t* This, const buffer_t* other)
{
    assert(This);
    ASSERT_OK(buffer_t, other);
    buffer_t_construct(This, other->max_size, other->do_alloc);
    This->alloc_mult = other->alloc_mult;
    This->size = other->size;
    memcpy (This->data, other->data, This->max_size);
    return true;
}

bool buffer_t_append(buffer_t* This, const char* data, size_t nbytes)
{
    ASSERT_OK(buffer_t, This);
    assert(data);
    if (!nbytes)
        return true;
    if (This->size + nbytes > This->max_size){
        if (!This->do_alloc){
            printf ("buffer_t_append: Error! The buffer is in no-allocation mode\n");
            return false;
        }
        size_t alloc_size = MAX(This->size + nbytes, This->alloc_mult*This->size);

        This->data = (char*)realloc(This->data, alloc_size);
        This->max_size = alloc_size;
        This->end = This->data + This->max_size - 1;
    }
    if (!This->data){
        perror("buffer_t_append: (can't realloc)");
        buffer_t_destruct(This);
        return false;
    }
    memcpy(This->data + This->size, data, nbytes);
    This->size += nbytes;

    return true;
}

#endif // BUFFER_H_INCLUDED
