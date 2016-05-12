//#define DEBUG
/// Author name
#define AUTHOR "Alartum"
/// Project name
#define PROJECT "Assembler"
/// Version
#define VERSION "1"

#include <stdio.h>
#include "mylib.h"
#include "commands_enum.h"
#include <string.h>
#include <limits.h>
#include <stdint.h>

#define DEFINES_ONLY
#include "commands.h"
#undef DEFINES_ONLY

enum PARSING_STATE
{
    CMD,
    ARG,
    DONE
};

typedef struct label_t label_t;
struct label_t
{
    char* name;
    size_t position; //Position in bytes
    short counter;
};

size_t assemble (const Buffer* source, Buffer* assembled, label_t* labels, bool show_warn);

int main (int argc, char* argv[])
{
    // Not using it
    (void)DUMP_INDENT;
    //^^^^^^^^^^^^^^^^^^^^^^^^^
    //Default part BEGIN
    //^^^^^^^^^^^^^^^^^^^^^^^^^
    CHECK_DEFAULT_ARGS();
    char inName[NAME_MAX] = {}, outName[NAME_MAX] = {};
    switch (argc)
    {
    case 3:
        strcpy (inName, argv[1]);
        strcpy (outName, argv[2]);
        break;
    case 2:
        strcpy (inName, argv[1]);
        strcpy (outName, "program.bin");
        break;
    default:
        WRITE_WRONG_USE();
    }

    //^^^^^^^^^^^^^^^^^^^^^^^^^
    //Default part END
    //Buffer BEGIN
    //^^^^^^^^^^^^^^^^^^^^^^^^^
    Buffer buffer;
    if (!buffer_construct (&buffer, inName))
    {
        perror ("#Input error");
        return WRONG_RESULT;
    }
    Buffer assembled;
    buffer_construct_empty (&assembled, buffer.length);


    int charcount = 1; // 1 because we need a stop label, so we need one more of them
    for(int m=0; buffer.chars[m]; m++)
        if(buffer.chars[m] == ':')
            charcount ++;
    label_t* labels = (label_t*) calloc (charcount, sizeof (*labels));
    for (int i = 0; i < charcount; i ++){
        labels[i].counter = 0;
        labels[i].position = SIZE_MAX;
        labels[i].name = NULL;
    }

    //printf ("FIRST RUN:\n");
    assemble(&buffer, &assembled, labels, false);
    //printf ("SECOND RUN:\n");
    size_t writing_pos = assemble(&buffer, &assembled, labels, true);
    if (!writing_pos)
    {
        printf ("Assemble error\n");
        open_file (out, outName, "wb", "#Output error");
        fwrite (assembled.chars, sizeof (char), assembled.length, out);
        buffer_destruct(&assembled);
        close_file (out);
        return WRONG_RESULT;
    }

    /*printf ("Labels:\n");
    for (int i = 0; labels[i].position != -1; i ++)
    {
        printf ("[%s] = %d;\n", labels[i].name, labels[i].position);
        free(labels[i].name);
    }*/
    //^^^^^^^^^^^^^^^^^^^^^^^^^
    //Parse END
    //Output BEGIN
    //^^^^^^^^^^^^^^^^^^^^^^^^^
    free (labels);
    buffer_destruct(&buffer);
    //printf ("AMOUNT: %lu\n", writing_pos);
    /*for (size_t i = 0; i < writing_pos; i++)
    {
        printf ("%d ", assembled.chars[i]);
    }*/

    open_file (out, outName, "wb", "#Output error");
    fwrite (assembled.chars, sizeof (char), writing_pos, out);
    buffer_destruct(&assembled);
    close_file (out);
    printf("\n#Programm successfully written to %s.\n", outName);
    //^^^^^^^^^^^^^^^^^^^^^^^^^
    //Output END
    //^^^^^^^^^^^^^^^^^^^^^^^^^
    return NO_ERROR;
}

size_t assemble(const Buffer* source, Buffer* assembled, label_t* labels, bool show_warn)
{
    Buffer buffer;
    buffer_construct_copy (&buffer, source);

    char* nextLinePtr = buffer.chars;
    // Current line number and writing byte
    size_t lineN = 0, writing_pos = 0;

    //^^^^^^^^^^^^^^^^^^^^^^^^^
    //Buffer END
    //Parse BEGIN
    //^^^^^^^^^^^^^^^^^^^^^^^^^

    //Reading state
    char state = DONE;
    bool labels_success = true;
    while (nextLinePtr != NULL && state == DONE)
    {
        //^^^^^^^^^^^^^^^^^^^^^^^^^
        //Lines marking BEGIN
        //^^^^^^^^^^^^^^^^^^^^^^^^^

        //printf ("#%lu# ", lineN);
        lineN++;

        while (strchr(" \t", *nextLinePtr))
            nextLinePtr++;
        if (strchr("\r\n", *nextLinePtr))
        {
            //printf ("(skipped)\n");
            nextLinePtr++;
            continue;
        }

        char* line = nextLinePtr;
        nextLinePtr = strpbrk (nextLinePtr, "\n\r");
        if (nextLinePtr)
        {
            // Need to put \0 for strtok to work correctly
            *nextLinePtr = '\0';
            nextLinePtr++;
        }

        //^^^^^^^^^^^^^^^^^^^^^^^^^
        //Lines marking END
        //Words parsing BEGIN
        //^^^^^^^^^^^^^^^^^^^^^^^^^

        char* word = strtok (line, " ");
        // State is what we are currently expecting to read
        int arg_type = -1;
        state = CMD;

        while (word != NULL){
            switch (state){
            case DONE:
                if (sscanf(word, ";%*s")){
                    //printf (" $comment$ ");
                    state = DONE;
                    word = NULL;
                    continue;
                }
                else{
                    if (show_warn) printf ("Parsing error\n");
                    return 0;
                }
                break;
            case CMD:
                if (state != DONE && sscanf(word, ";%*s")){
                    //printf (" $comment$ ");
                    state = DONE;
                    word = NULL;
                    continue;
                }
                if ((state != DONE) && (!strcmp (word, "end"))){
                    //printf ("end\n");
                    assembled->chars[writing_pos] = (char)cmd_end;
                    writing_pos ++;
                    assembled->chars[writing_pos] = 0;
                    buffer_destruct(&buffer);

                    return writing_pos;
                }
                #define CMD(name, key, shift, arguments) \
                if (!(arguments & ARG_OVL)) \
                {\
                    if ((state == CMD) && (!strcmp (word, #name)))\
                    {\
                        /*printf ("%s ", #name);*/\
                        assembled->chars[writing_pos] = (char)key;\
                        writing_pos ++;\
                        if (arguments & ARG_NO)\
                        {\
                            state = DONE;\
                        }\
                        else \
                        {\
                            state = ARG;\
                            arg_type = arguments;\
                        }\
                    }\
                }
                #include "commands.h"
                #undef CMD

                #define VAR(_name, _size) \
                if ((state == CMD) && (!strcmp (word, #_name))){\
                    /*printf ("%s ", #name);*/\
                    writing_pos += _size;\
                    state = DONE;\
                }
                #include "var_sizes.h"
                #undef VAR
                if (state == CMD){
                    // If decimal point is detected we'll treat it as float value
                    int word_len = strlen(word);
                    if (strchr(word, '.')){
                        float float_num = 0;
                        //printf ("(%f)", num);
                        if (sscanf (word, "%f", &float_num) == word_len){
                            *(float*)(assembled->chars+writing_pos) = float_num;
                            writing_pos += sizeof(float);
                            state = DONE;
                        }
                    }
                    else{
                        int int_num = 0;
                        char char_num = 0;
                        //printf ("(%f)", num);
                        if (sscanf (word, "%d", &int_num) == word_len){
                            *(int*)(assembled->chars+writing_pos) = int_num;
                            writing_pos += sizeof(int);
                            state = DONE;
                        }
                        else if (sscanf (word, "'%c'", &char_num) == word_len){
                            *(char*)(assembled->chars+writing_pos) = char_num;
                            writing_pos += sizeof(char);
                            state = DONE;
                        }
                    }
                }
                char* dots_ptr = strchr (word, ':');
                if (dots_ptr){
                    *dots_ptr = '\0';
                    state = DONE;
                    int i = 0;
                    for (; labels[i].position != SIZE_MAX; i++){
                        if (!strcmp (labels[i].name, word)){
                            if (labels[i].counter < 2){
                                labels[i].counter ++;
                                break;
                            }
                            else{
                                if (show_warn) printf ("Multiple label definition: %s\n", word);
                                buffer_destruct(&buffer);
                                return 0;
                            }
                        }
                    }
                    if (labels[i].position == SIZE_MAX){
                        labels[i].name = strdup (word);
                        labels[i].counter ++;
                        labels[i].position = writing_pos;
                        //printf (" ::%s:: (%lu)", word, writing_pos);
                    }
                    state = DONE;
                }
                if (state == CMD){
                    if (show_warn) printf ("\nUnknown command in line #%lu: %s\n", lineN, word);
                    buffer_destruct(&buffer);
                    assembled->chars[writing_pos] = (char)cmd_err;
                    return 0;
                }

                break;
            case ARG:
                ;
                int word_len = strlen(word);
                /////////////////////////////////////////////
                //// SIZE OPERAND /////////////////////////
                /////////////////////////////////////////////
                if ((state != DONE) && (arg_type & ARG_SIZ)){
                    bool size_done = false;
                    #define CHECK_SIZE(_name, _offset)\
                    if (!size_done && !strcmp(word, #_name)){\
                        assembled->chars[writing_pos - 1] += _offset;\
                        size_done = true;\
                    }
                    // Changing command identifier to the momory's one
                    // They follow each other, so cmd+2 will give us the mem variant
                    CHECK_SIZE(byte, 1)
                    CHECK_SIZE(word,  2)
                    CHECK_SIZE(dword, 3)
                    if (size_done != true){
                        if (show_warn) printf ("\nCan't find size specifier in line #%lu: %s\n", lineN, word);
                        buffer_destruct(&buffer);
                        assembled->chars[writing_pos] = (char)cmd_err;
                        return 0;
                    }
                }
                //printf ("Parsing: <%s> ", word);
                /////////////////////////////////////////////
                //// POSITION OPERAND /////////////////////////
                /////////////////////////////////////////////
                if ((state != DONE) && (arg_type & ARG_POS)){
                    size_t pos = 0;
                    if (sscanf (word, "%lu", &pos) == word_len){
                        //printf ("{%d}", pos);
                        *(size_t*)(assembled->chars+writing_pos) = pos;
                        writing_pos += sizeof(size_t);
                        state = DONE;
                    }
                }
                /////////////////////////////////////////////
                //// NUMBER OPERAND /////////////////////////
                /////////////////////////////////////////////
                if ((state != DONE) && (arg_type & ARG_NUM)){
                    // If decimal point is detected we'll treat it as float value
                    if (strchr(word, '.')){
                        float float_num = 0;
                        //printf ("(%f)", num);
                        if (sscanf (word, "%f", &float_num) == word_len){
                            *(float*)(assembled->chars+writing_pos) = float_num;
                            assembled->chars[writing_pos - 1] += 8;
                            writing_pos += sizeof(float);
                            state = DONE;
                        }
                    }
                    else{
                        int int_num = 0;
                        char char_num = 0;
                        //printf ("(%f)", num);
                        if (sscanf (word, "%d", &int_num) == word_len){
                            *(int*)(assembled->chars+writing_pos) = int_num;
                            assembled->chars[writing_pos - 1] += 7;
                            writing_pos += sizeof(int);
                            state = DONE;
                        }
                        else if (sscanf (word, "'%c'", &char_num) == word_len){
                            *(char*)(assembled->chars+writing_pos) = char_num;
                            assembled->chars[writing_pos - 1] += 9;
                            writing_pos += sizeof(char);
                            state = DONE;
                        }
                    }
                }
                /////////////////////////////////////////////
                //// MEMORY OPERAND /////////////////////////
                /////////////////////////////////////////////
                if ((state != DONE) && (arg_type & ARG_MEM)){
                    if (*word == '['){
                        word ++;
                        char* p = word;
                        for (; *p != ']' && *p; p++);
                        if (*p != ']'){
                            if (show_warn) printf ("\nWrong address (']' is missed?) in line #%lu: %s\n", lineN, word);
                            labels_success = false;
                            state = DONE;
                        }
                        else{
                            *p = 0;
                            size_t mem = 0;
                            for (size_t i = 0; labels[i].position != SIZE_MAX && state != DONE; i++){
                                if (!strcmp (labels[i].name, word)){
                                    //printf ("[%s]{%d}", word, labels[i].position);
                                    *(size_t*)(assembled->chars+writing_pos) = labels[i].position;
                                    writing_pos += sizeof(size_t);
                                    state = DONE;
                                }
                            }
                            if (state != DONE && sscanf (word, "%lu", &mem) == word_len){
                                //printf ("[%u]", mem);
                                *(size_t*)(assembled->chars + writing_pos) = mem;
                                writing_pos += sizeof(size_t);
                                state = DONE;
                            }
                            if (state != DONE){
                                if (show_warn) {
                                    printf ("\nWrong address (format problem?) in line #%lu: %s\n", lineN, word);
                                    buffer_destruct(&buffer);
                                    assembled->chars[writing_pos] = (char)cmd_err;
                                    return 0;
                                }
                                *(size_t*)(assembled->chars+writing_pos) = SIZE_MAX;
                                writing_pos += sizeof(size_t);
                                labels_success = false;
                                state = DONE;// Just to be sure
                            }
                        }
                    }
                }
                /////////////////////////////////////////////
                //// REGISTER OPERAND ///////////////////////
                /////////////////////////////////////////////
                if ((state != DONE) && (arg_type & ARG_REG)){
                    //printf ("[Wr_pos: %s]", word);
                    #define ADDRESS(name, address, size, offset) \
                    if ((state != DONE) && (!strcmp (word, #name)))\
                    {\
                        /* Changing command identifier to reg*/\
                        /*printf ("[%s]|%d|", word, address);*/\
                        assembled->chars[writing_pos - 1] += offset ;\
                        assembled->chars[writing_pos] = (char)address;\
                        writing_pos ++;\
                        state = DONE;\
                    }
                    #include "reg_address.h"
                    #undef ADDRESS
                }
                /////////////////////////////////////////////
                //// LABEL OPERAND //////////////////////////
                /////////////////////////////////////////////
                if ((state != DONE) && (arg_type & ARG_LBL)){
                    for (size_t i = 0; labels[i].position != SIZE_MAX && state != DONE; i++){
                        if (!strcmp (labels[i].name, word)){
                            //printf ("[%s]{%d}", word, labels[i].position);
                            *(size_t*)(assembled->chars+writing_pos) = labels[i].position;
                            writing_pos += sizeof(size_t);
                            state = DONE;
                        }
                    }
                    if (state != DONE)
                    {
                        *(int*)(assembled->chars+writing_pos) = -1;
                        writing_pos += sizeof(int);
                    }
                    labels_success = false;
                    state = DONE;// Just to be sure
                }
                if (state != DONE)
                {
                    printf ("\nUnknown command in line #%lu: %s\n", lineN, word);
                    buffer_destruct(&buffer);
                    assembled->chars[writing_pos] = (char)cmd_err;
                    return 0;
                }
                break;
            }

            // Take next word
            word = strtok (NULL, " ");
        }

        //^^^^^^^^^^^^^^^^^^^^^^^^^
        //Words parsing END
        //^^^^^^^^^^^^^^^^^^^^^^^^^
        //printf ("\n");
    }
    buffer_destruct(&buffer);
    return labels_success && writing_pos;
}
