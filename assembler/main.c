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

#define NO_DEBUG_INFO
#undef NO_DEBUG_INFO

enum PARSING_STATE
{
    CMD, // Normal command
    ARG, // Command's argument
    DATA, // Labels and sizes in .data section
    DONE // Command parsing is finished
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
    #ifndef NO_DEBUG_INFO
    printf ("\nFIRST RUN:\n");
    #endif // NO_DEBUG_INFO
    assemble(&buffer, &assembled, labels, false);
    #ifndef NO_DEBUG_INFO
    printf ("\n\nSECOND RUN:\n");
    #endif // NO_DEBUG_INFO
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
    bool in_data = false, end_data = false;
    while (nextLinePtr != NULL && state == DONE)
    {
        //^^^^^^^^^^^^^^^^^^^^^^^^^
        //Lines marking BEGIN
        //^^^^^^^^^^^^^^^^^^^^^^^^^

        #ifndef NO_DEBUG_INFO
        printf ("#%lu# ", lineN);
        #endif // NO_DEBUG_INFO
        lineN++;

        while (strchr(" \t", *nextLinePtr))
            nextLinePtr++;
        if (strchr("\r\n", *nextLinePtr)){
            #ifndef NO_DEBUG_INFO
            printf ("(skipped)\n");
            #endif // NO_DEBUG_INFO
            nextLinePtr++;
            continue;
        }

        char* line = nextLinePtr;
        nextLinePtr = strpbrk (nextLinePtr, "\n\r");
        if (nextLinePtr){
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
        // The .data (if provided) must be in the very beginning
        if (!strcmp(word, ".data")){
            #ifndef NO_DEBUG_INFO
            printf ("Data section detected!\n");
            #endif // NO_DEBUG_INFO
            in_data = true;
        }
        if (in_data && !end_data)
            state = DATA;
        else
            state = CMD;

        for (;word != NULL; word = strtok (NULL, " ")){
            switch (state){
            case DATA:
                // If in the end of the section. change state
                if (!strcmp(word, ".code")){
                    #ifndef NO_DEBUG_INFO
                    printf ("Data section ended! Code section starts.\n");
                    #endif // NO_DEBUG_INFO
                    end_data = true;
                    state = DONE;
                }
                // Reading variable in format: var_name: var_size var_value (example: a: byte raw)

                if (state == DATA){
                    #define VAR(_name,_size) \
                    if ((state == DATA) && (!strcmp (word, #_name))){\
                        printf ("%s ", #_name); \
                        writing_pos += _size;\
                        strtok (NULL, " ");\
                        state = ARG;\
                    }
                    #include "var_sizes.h"
                    #undef VAR

                    if (state == ARG){
                        // If decimal point is detected we'll treat it as float value
                        int word_len = strlen(word);
                        if (!strcmp(word, "raw"))
                            state = DONE;
                        #define READ_CONST(_type, _spec) \
                        _type _type##_num = 0;\
                        if (state == ARG && sscanf (word, _spec, &(_type##_num)) == word_len){\
                            printf ("[" #_type "]" _spec, _type##_num);\
                            *(_type*)(assembled->chars+writing_pos - sizeof(_type)) = _type##_num;\
                            state = DONE;\
                        }
                        READ_CONST(int, "%d")
                        READ_CONST(float, "%f")
                        READ_CONST(char, "'%c'")
                        #undef READ_CONST
                    }
                }
                if (state == DATA){
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
                                    if (show_warn) printf ("Multiple label definition in line #%lu: %s\n", lineN, word);
                                    buffer_destruct(&buffer);
                                    return 0;
                                }
                            }
                        }
                        if (labels[i].position == SIZE_MAX){
                            labels[i].name = strdup (word);
                            labels[i].counter ++;
                            labels[i].position = writing_pos;
                            #ifndef NO_DEBUG_INFO
                            printf (" ::%s:: (%lu)", word, writing_pos);
                            #endif // NO_DEBUG_INFO
                        }
                        state = DONE;
                    }
                }
                if (state == DATA || state == ARG){
                    if (show_warn) printf ("\nUnknown command in line #%lu: %s\n", lineN, word);
                    buffer_destruct(&buffer);
                    assembled->chars[writing_pos] = (char)cmd_err;
                    return 0;
                }
                break;
            case DONE:
                if (sscanf(word, ";%*s")){
                    #ifndef NO_DEBUG_INFO
                    printf (" $comment$ ");
                    #endif // NO_DEBUG_INFO
                    state = DONE;
                    word = NULL;
                    continue;
                }
                else{
                    if (show_warn) printf ("\n Unknown sequence [operand expected?] #%lu: %s\n", lineN, word);
                    return 0;
                }
                break;
            case CMD:
                if (state != DONE && sscanf(word, ";%*s")){
                    #ifndef NO_DEBUG_INFO
                    printf (" $comment$ ");
                    #endif // NO_DEBUG_INFO
                    state = DONE;
                    word = NULL;
                    continue;
                }
                if ((state != DONE) && (!strcmp (word, "end"))){
                    #ifndef NO_DEBUG_INFO
                    printf ("end\n");
                    #endif // NO_DEBUG_INFO
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
                        printf ("%s ", #name);\
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
                                if (show_warn) printf ("Multiple label definition in line #%lu: %s\n", lineN, word);
                                buffer_destruct(&buffer);
                                return 0;
                            }
                        }
                    }
                    if (labels[i].position == SIZE_MAX){
                        labels[i].name = strdup (word);
                        labels[i].counter ++;
                        labels[i].position = writing_pos;
                        #ifndef NO_DEBUG_INFO
                        printf (" ::%s:: (%lu)", word, writing_pos);
                        #endif // NO_DEBUG_INFO
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
                //// NUMBER OPERAND /////////////////////////
                /////////////////////////////////////////////
                if ((state != DONE) && (arg_type & ARG_NUM)){
                    #define READ_CONST(_type, _spec, _offset) \
                    _type _type##_num = 0;\
                    if (state != DONE && sscanf (word, _spec, &(_type##_num)) == word_len){\
                        printf ("[" #_type "]" _spec, _type##_num);\
                        *(_type*)(assembled->chars+writing_pos) = _type##_num;\
                        assembled->chars[writing_pos - 1] += _offset;\
                        writing_pos += sizeof(_type);\
                        state = DONE;\
                    }
                    READ_CONST(int, "%d", 7)
                    if (state != DONE && int_num < 0 && *word == '-'){
                        printf ("[" "int" "]" "%d", int_num);
                        *(int*)(assembled->chars+writing_pos) = int_num;
                        assembled->chars[writing_pos - 1] += 7;
                        writing_pos += sizeof(int);
                        state = DONE;
                    }
                    READ_CONST(float, "%f", 8)
                    READ_CONST(char, "'%c'", 9)
                    #undef READ_CONST
                }
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
                    #undef CHECK_SIZE
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
        }

        //^^^^^^^^^^^^^^^^^^^^^^^^^
        //Words parsing END
        //^^^^^^^^^^^^^^^^^^^^^^^^^
        #ifndef NO_DEBUG_INFO
        printf ("\n");
        #endif // NO_DEBUG_INFO
    }
    buffer_destruct(&buffer);
    return labels_success && writing_pos;
}
