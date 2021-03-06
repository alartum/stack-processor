//#define DEBUG
/// Author name
#define AUTHOR "Alartum"
/// Project name
#define PROJECT "Assembler"
/// Version
#define VERSION "1"

#include <stdio.h>
#include "mylib.h"
#include "buffer_t.h"
#include "commands_enum.h"
#include <string.h>
#include <limits.h>
#include <stdint.h>

#define DEFINES_ONLY
#include "commands.h"
#undef DEFINES_ONLY

// For jump address
#include "commands_enum.h"

#define NO_DEBUG_INFO
//#undef NO_DEBUG_INFO

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
    unsigned position; //Position in bytes
    short counter;
};

unsigned assemble (const buffer_t* source, buffer_t* assembled, label_t* labels, bool show_warn, bool is_verbose);

int main (int argc, char* argv[])
{
    // Not using it
    (void)DUMP_INDENT;
    bool is_verbose = false;
    //^^^^^^^^^^^^^^^^^^^^^^^^^
    //Default part BEGIN
    //^^^^^^^^^^^^^^^^^^^^^^^^^
    CHECK_DEFAULT_ARGS();
    char inName[NAME_MAX] = {}, outName[NAME_MAX] = {};
    switch (argc)
    {
    case 4:
        strcpy (inName, argv[1]);
        strcpy (outName, argv[2]);
        if (!strcmp (argv[3], "--verbose"))
            is_verbose = true;
        break;
    case 3:
        strcpy (inName, argv[1]);
        if (!strcmp (argv[2], "--verbose")){
            is_verbose = true;
            strcpy (outName, "program.bin");
        }
        else
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
    //buffer_t BEGIN
    //^^^^^^^^^^^^^^^^^^^^^^^^^
    buffer_t buffer;
    if (!buffer_t_construct_filename (&buffer, inName))
    {
        perror ("#Input error");
        return WRONG_RESULT;
    }
    buffer_t assembled;
    buffer_t_construct (&assembled, buffer.size, false);


    int charcount = 1; // 1 because we need a stop label, so we need one more of them
    for(int m=0; buffer.data[m]; m++)
        if(buffer.data[m] == ':')
            charcount ++;
    label_t* labels = (label_t*) calloc (charcount, sizeof (*labels));
    for (int i = 0; i < charcount; i ++){
        labels[i].counter = 0;
        labels[i].position = UINT_MAX;
        labels[i].name = NULL;
    }
    #ifndef NO_DEBUG_INFO
    printf ("\nFIRST RUN:\n");
    #endif // NO_DEBUG_INFO
    assemble(&buffer, &assembled, labels, false, false);
    #ifndef NO_DEBUG_INFO
    printf ("\n\nSECOND RUN:\n");
    #endif // NO_DEBUG_INFO
    unsigned writing_pos = assemble(&buffer, &assembled, labels, true, is_verbose);
    if (!writing_pos)
    {
        printf ("Assemble error\n");
        open_file (out, outName, "wb", "#Output error");
        fwrite (assembled.data, sizeof (char), assembled.size, out);
        buffer_t_destruct(&assembled);
        close_file (out);
        return WRONG_RESULT;
    }
    if (is_verbose){
        printf ("Labels:\n");
        for (unsigned i = 0; labels[i].position != UINT_MAX; i ++){
            printf ("[%s] = %u;\n", labels[i].name, labels[i].position);
            free(labels[i].name);
        }
    }
    //^^^^^^^^^^^^^^^^^^^^^^^^^
    //Parse END
    //Output BEGIN
    //^^^^^^^^^^^^^^^^^^^^^^^^^
    free (labels);
    buffer_t_destruct(&buffer);
    //printf ("AMOUNT: %u\n", writing_pos);
    /*for (unsigned i = 0; i < writing_pos; i++)
    {
        printf ("%d ", assembled.data[i]);
    }*/

    open_file (out, outName, "wb", "#Output error");
    fwrite (assembled.data, sizeof (char), writing_pos, out);
    buffer_t_destruct(&assembled);
    close_file (out);
    printf("#Programm successfully written to %s.\n", outName);
    //^^^^^^^^^^^^^^^^^^^^^^^^^
    //Output END
    //^^^^^^^^^^^^^^^^^^^^^^^^^
    return NO_ERROR;
}

unsigned assemble(const buffer_t* source, buffer_t* assembled, label_t* labels, bool show_warn, bool is_verbose)
{
    buffer_t buffer;
    buffer_t_construct_copy (&buffer, source);

    char* nextLinePtr = buffer.data;
    // Current line number and writing byte
    unsigned lineN = 0, writing_pos = 0;

    //^^^^^^^^^^^^^^^^^^^^^^^^^
    //buffer_t END
    //Parse BEGIN
    //^^^^^^^^^^^^^^^^^^^^^^^^^

    //Reading state
    char state = DONE;
    bool labels_success = true;
    unsigned entry_point = 0;
    bool in_data = false, end_data = false, in_code = false;
    // Reserve space for enter point
    writing_pos += sizeof(unsigned) + 1;
    while (nextLinePtr != NULL && state == DONE)
    {
        //^^^^^^^^^^^^^^^^^^^^^^^^^
        //Lines marking BEGIN
        //^^^^^^^^^^^^^^^^^^^^^^^^^

        if (is_verbose) printf ("#%u# ", lineN);
        lineN++;

        while (strchr(" \t", *nextLinePtr))
            nextLinePtr++;
        if (strchr("\r\n", *nextLinePtr)){
            if (is_verbose) printf ("(skipped)\n");
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
            if (in_data || end_data)
                if (show_warn) printf ("Multiple .data section (there must be only one!) in line #%u: %s\n", lineN, word);
            #ifndef NO_DEBUG_INFO
            printf ("Data section detected!\n");
            #endif // NO_DEBUG_INFO
            in_data = true;
            continue;
        }
        if (!strcmp(word, ".code")){
            #ifndef NO_DEBUG_INFO
            if (is_verbose) printf ("Data section ended! Code section starts.\n");
            #endif // NO_DEBUG_INFO
            if (show_warn && in_code) printf ("Multiple .code section (there must be only one!) in line #%u: %s\n", lineN, word);
            in_code = true;
            end_data = true;
            entry_point = writing_pos;
            continue;
        }
        if (in_data && !end_data)
            state = DATA;
        else
            state = CMD;

        for (;word != NULL; word = (word)? strtok (NULL, " ") : word){
            switch (state){
            case DATA:
                //printf ("Data check: [%s]\n", word);
                // If in the end of the section. change state
                // Reading variable in format: var_name: var_size var_value (example: a: byte raw)
                if (state == DATA){
                    char* dots_ptr = strchr (word, ':');
                    if (dots_ptr){
                        *dots_ptr = '\0';
                        state = DONE;
                        int i = 0;
                        for (; labels[i].position != UINT_MAX; i++){
                            if (!strcmp (labels[i].name, word)){
                                if (labels[i].counter < 2){
                                    labels[i].counter ++;
                                    break;
                                }
                                else{
                                    if (show_warn) printf ("Multiple label definition in line #%u: %s\n", lineN, word);
                                    buffer_t_destruct(&buffer);
                                    return 0;
                                }
                            }
                        }
                        if (labels[i].position == UINT_MAX){
                            labels[i].name = strdup (word);
                            labels[i].counter ++;
                            labels[i].position = writing_pos;
                            if (is_verbose) printf (" ::%s:: (%u)", word, writing_pos);
                        }
                        state = DATA;
                    }
                }
                word = strtok (NULL, " ");
                //printf (".data offset: [%s]\n", word);
                if (state == DATA){
                    unsigned offset = 0;
                    //printf ("Checking [" #_name "]\n");
                    #define VAR(_name,_size) \
                    if ((state == DATA) && (!strcmp (word, #_name))){\
                        if (is_verbose) printf ("%s ", #_name); \
                        offset = _size;\
                        state = ARG;\
                    }
                    #include "var_sizes.h"
                    #undef VAR
                    word = strtok (NULL, " ");
                    if (state == ARG){
                        //printf (".data arg: [%s]\n", word);
                        // If decimal point is detected we'll treat it as float value
                        if (!strcmp(word, "raw"))
                            state = DONE;
                        #define READ_CONST(_type, _spec) \
                        _type _type##_num = 0;\
                        if (state == ARG && sscanf (word, _spec, &(_type##_num))){\
                            if (offset < sizeof(_type) && show_warn){\
                                printf ("\nWrong size (%u bytes reserved, but " #_type " takes %lu bytes) in line #%u: %s\n", offset, sizeof(_type), lineN, word);\
                                buffer_t_destruct(&buffer);\
                                assembled->data[writing_pos] = (char)cmd_err;\
                                return 0;\
                            }\
                            if (is_verbose) printf ("[" #_type "]" _spec, _type##_num);\
                            *(_type*)(assembled->data+writing_pos) = _type##_num;\
                            word = strtok (NULL, " ");\
                            state = DONE;\
                        }
                        //printf ("word=[%s]\n", word);
                        if (strchr(word, '.')){
                            READ_CONST(float, "%f")
                        }
                        READ_CONST(int, "%d")
                        READ_CONST(char, "'%c'")
                        #undef READ_CONST
                        writing_pos += offset;
                    }
                }
                if (state == DATA || state == ARG){
                    if (show_warn) printf ("\nUnknown command in line #%u: %s\n", lineN, word);
                    buffer_t_destruct(&buffer);
                    assembled->data[writing_pos] = (char)cmd_err;
                    return 0;
                }
                break;
            case DONE:
                if (sscanf(word, ";%*s")){
                    if (is_verbose) printf (" $comment$ ");
                    state = DONE;
                    word = NULL;
                    continue;
                }
                else{
                    if (show_warn) printf ("\n Unknown sequence [operand expected?] in line #%u: %s\n", lineN, word);
                    return 0;
                }
                break;
            case CMD:
                if (state != DONE && sscanf(word, ";%*s")){
                    if (is_verbose) printf (" $comment$ ");
                    state = DONE;
                    word = NULL;
                    continue;
                }
                if ((state != DONE) && (!strcmp (word, "stop"))){
                    if (is_verbose) printf ("stop\n");
                    assembled->data[writing_pos] = (char)cmd_stop;
                    writing_pos ++;
                    assembled->data[writing_pos] = 0;
                    buffer_t_destruct(&buffer);
                    // Writing entry info
                    if (!in_code && show_warn) printf ("Can't find entry point (.code section)!\n");
                    assembled->data[0] = cmd_jmp;
                    *(unsigned*)(assembled->data + 1) = entry_point;

                    return writing_pos;
                }
                #define CMD(name, key, shift, arguments) \
                if (!(arguments & ARG_OVL)) \
                {\
                    if ((state == CMD) && (!strcmp (word, #name)))\
                    {\
                        if (is_verbose) printf ("%s ", #name);\
                        assembled->data[writing_pos] = (char)key;\
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
                    for (; labels[i].position != UINT_MAX; i++){
                        if (!strcmp (labels[i].name, word)){
                            if (labels[i].counter < 2){
                                labels[i].counter ++;
                                break;
                            }
                            else{
                                if (show_warn) printf ("Multiple label definition in line #%u: %s\n", lineN, word);
                                buffer_t_destruct(&buffer);
                                return 0;
                            }
                        }
                    }
                    if (labels[i].position == UINT_MAX){
                        labels[i].name = strdup (word);
                        labels[i].counter ++;
                        labels[i].position = writing_pos;
                        if (is_verbose) printf (" ::%s:: (%u)", word, writing_pos);
                    }
                    state = DONE;
                }
                if (state == CMD){
                    if (show_warn) printf ("\nUnknown command in line #%u: %s\n", lineN, word);
                    buffer_t_destruct(&buffer);
                    assembled->data[writing_pos] = (char)cmd_err;
                    return 0;
                }
                break;
            case ARG:
                ;
                /////////////////////////////////////////////
                //// NUMBER OPERAND /////////////////////////
                /////////////////////////////////////////////
                if ((state != DONE) && (arg_type & ARG_NUM)){
                    for (unsigned i = 0; labels[i].position != UINT_MAX && state != DONE; i++){
                        if (!strcmp (labels[i].name, word)){
                            //printf ("[%s]{%d}", word, labels[i].position);
                            // If the command is pop
                            if (!(arg_type^ARG_SIZ))
                                assembled->data[writing_pos - 1] += 7;
                            *(unsigned*)(assembled->data+writing_pos) = labels[i].position;
                            writing_pos += sizeof(unsigned);
                            state = DONE;
                        }
                    }
                    #define READ_CONST(_type, _spec, _offset) \
                    _type _type##_num = 0;\
                    if (state != DONE && sscanf (word, _spec, &(_type##_num))){\
                        if (is_verbose) printf ("[" #_type "]" _spec, _type##_num);\
                        *(_type*)(assembled->data+writing_pos) = _type##_num;\
                        assembled->data[writing_pos - 1] += _offset;\
                        writing_pos += sizeof(_type);\
                        state = DONE;\
                    }
                    if (strchr(word, '.')){
                        READ_CONST(float, "%f", 8)
                    }
                    READ_CONST(int, "%d", 7)
                    READ_CONST(char, "'%c'", 9)
                    #undef READ_CONST
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
                        if (is_verbose) printf ("[%s]|%d|", word, address);\
                        assembled->data[writing_pos - 1] += offset ;\
                        assembled->data[writing_pos] = (char)address;\
                        writing_pos ++;\
                        state = DONE;\
                    }
                    #include "reg_address.h"
                    #undef ADDRESS
                }
                /////////////////////////////////////////////
                //// SIZE OPERAND /////////////////////////
                /////////////////////////////////////////////
                if ((state != DONE) && (arg_type & ARG_SIZ)){
                    bool size_done = false;
                    //printf ("Size check: [%s]\n", word);
                    #define CHECK_SIZE(_name, _offset)\
                    if (!size_done && !strcmp(word, #_name)){\
                        assembled->data[writing_pos - 1] += _offset;\
                        size_done = true;\
                    }
                    // Changing command identifier to the momory's one
                    // They follow each other, so cmd+2 will give us the mem variant
                    CHECK_SIZE(byte, 1)
                    CHECK_SIZE(word,  2)
                    CHECK_SIZE(dword, 3)
                    #undef CHECK_SIZE
                    if (size_done != true){
                        if (show_warn) printf ("\nCan't find size specifier [byte, word, dword] in line #%u: %s\n", lineN, word);
                        buffer_t_destruct(&buffer);
                        assembled->data[writing_pos] = (char)cmd_err;
                        return 0;
                    }
                    if (!(arg_type^ARG_SIZ))
                        state = DONE;
                    else
                        word = strtok (NULL, "");
                }
                //printf ("Parsing: <%s> ", word);
                /////////////////////////////////////////////
                //// POSITION OPERAND /////////////////////////
                /////////////////////////////////////////////
                if ((state != DONE) && (arg_type & ARG_POS)){
                    unsigned pos = 0;
                    if (sscanf (word, "%u", &pos)){
                        //printf ("{%d}", pos);
                        *(unsigned*)(assembled->data+writing_pos) = pos;
                        writing_pos += sizeof(unsigned);
                        state = DONE;
                    }
                }
                /////////////////////////////////////////////
                //// MEMORY OPERAND /////////////////////////
                /////////////////////////////////////////////
                if ((state != DONE) && (arg_type & ARG_MEM)){
                    //printf ("Mem check: [%s]\n", word);
                    if (*word == '['){
                        word ++;
                        char* p = word;
                        for (; *p != ']' && *p; p++);
                        if (*p != ']'){
                            if (show_warn) printf ("\nWrong address (']' is missed?) in line #%u: %s\n", lineN, word);
                            labels_success = false;
                            state = DONE;
                        }
                        else{
                            *p = 0;
                            unsigned mem = 0;
                            for (unsigned i = 0; labels[i].position != UINT_MAX && state != DONE; i++){
                                if (!strcmp (labels[i].name, word)){
                                    //printf ("[%s]{%d}", word, labels[i].position);
                                    *(unsigned*)(assembled->data+writing_pos) = labels[i].position;
                                    writing_pos += sizeof(unsigned);
                                    state = DONE;
                                }
                            }
                            if (state != DONE && sscanf (word, "%u", &mem)){
                                //printf ("[%u]", mem);
                                *(unsigned*)(assembled->data + writing_pos) = mem;
                                writing_pos += sizeof(unsigned);
                                state = DONE;
                            }
                            if (state != DONE){
                                if (show_warn) {
                                    printf ("\nWrong address (format problem?) in line #%u: %s\n", lineN, word);
                                    buffer_t_destruct(&buffer);
                                    assembled->data[writing_pos] = (char)cmd_err;
                                    return 0;
                                }
                                *(unsigned*)(assembled->data+writing_pos) = UINT_MAX;
                                writing_pos += sizeof(unsigned);
                                labels_success = false;
                                state = DONE;// Just to be sure
                            }
                        }
                    }
                }
                /////////////////////////////////////////////
                //// LABEL OPERAND //////////////////////////
                /////////////////////////////////////////////
                if ((state != DONE) && (arg_type & ARG_LBL)){
                    for (unsigned i = 0; labels[i].position != UINT_MAX && state != DONE; i++){
                        if (!strcmp (labels[i].name, word)){
                            //printf ("[%s]{%d}", word, labels[i].position);
                            // If the command is pop
                            if (!(arg_type^ARG_SIZ))
                                assembled->data[writing_pos - 1] += 7;
                            *(unsigned*)(assembled->data+writing_pos) = labels[i].position;
                            writing_pos += sizeof(unsigned);
                            state = DONE;
                        }
                    }
                    if (state != DONE)
                    {
                        *(int*)(assembled->data+writing_pos) = -1;
                        writing_pos += sizeof(int);
                    }
                    labels_success = false;
                    state = DONE;// Just to be sure
                }
                if (state != DONE)
                {
                    printf ("\nUnknown command in line #%u: %s\n", lineN, word);
                    buffer_t_destruct(&buffer);
                    assembled->data[writing_pos] = (char)cmd_err;
                    return 0;
                }
                break;
            }
        }

        //^^^^^^^^^^^^^^^^^^^^^^^^^
        //Words parsing END
        //^^^^^^^^^^^^^^^^^^^^^^^^^
        if (is_verbose) printf ("\n");
    }
    buffer_t_destruct(&buffer);
    return labels_success && writing_pos;
}
