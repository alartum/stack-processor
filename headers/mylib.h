/** @file */

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>

#ifndef MYLIB_H_INCLUDED
#define MYLIB_H_INCLUDED

static int DUMP_INDENT = 0;
const int INDENT_VALUE = 4;

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
/// Indent of dump for pretty output

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
enum MAIN_ERRORS
{
    NO_ERROR,
    WRONG_RESULT,
    INFO_CALL,
    WRONG_USE
};

#define BOOM() printf(ANSI_COLOR_RED "BOOM!"ANSI_COLOR_RESET"\n\t"ANSI_COLOR_RED \
                    "Function:"ANSI_COLOR_RESET" %s\n\t"ANSI_COLOR_RED \
                    "File:"ANSI_COLOR_RESET" %s\n\t"ANSI_COLOR_RED \
                    "Line:"ANSI_COLOR_RESET" %d\n\n", __FUNCTION__, __FILE__, __LINE__)
#define BADABOOM(code) printf("BADABOOM " code "\n")
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define open_file(file, name, mode, error_message) \
    FILE *(file) = fopen (name, mode);\
        if (!(file))\
        {\
            perror (error_message);\
            return WRONG_RESULT;\
        }

#define close_file(file) \
    if (fclose (file))\
    {\
        perror ("#Can't close the file");\
        return WRONG_RESULT;\
    }

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Default info message
#if !defined(AUTHOR) || !defined (PROJECT) || !defined (VERSION)
#error Please, define AUTHOR, PROJECT and VERSION.
#else
#if defined(DEBUG)
/// Standard info message (debug mode)
#define INFO() ( printf ("#\t\t[" PROJECT " v." VERSION "] by " AUTHOR "\n#\t\t   ("__TIMESTAMP__ ")\n#\t\t\t    [DEBUG]\n") )
#else
/// Standard info message (default mode)
#define INFO() ( printf ("#\t\t[" PROJECT " v." VERSION "] by " AUTHOR "\n#\t\t   ("__TIMESTAMP__ ")\n") )
#endif
int print_help ()
{
    INFO();
    FILE *f;
    f = fopen("help", "r");
    if (!f){
        perror ("print_help: (can't open file)");
        return WRONG_RESULT;
    }
    char c = fgetc(f);
    while (c != EOF){
        printf ("%c", c);
        c = fgetc(f);
    }
    fclose(f);
    return NO_ERROR;
}

#define CHECK_DEFAULT_ARGS()                \
    if (argc == 2)                          \
    {                                       \
        if (!strcmp (argv[1], "--help"))    \
        {                                   \
            print_help ();                  \
            return INFO_CALL;               \
        }                                   \
        if (!strcmp (argv[1], "--version")) \
        {                                   \
            COMMENT("Version: " VERSION);   \
            return INFO_CALL;               \
        }                                   \
    }                                       \

#define WRITE_WRONG_USE() \
    printf ("#Wrong use. Use --help specifier to get help.\n");\
    return WRONG_USE;\

#endif

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Puts a comment in pretty way
#define COMMENT(x) ( printf ("#" x "\n") )
/// Useful check for number is it 0
#define IS_ZERO(x) ( fabs (x) < DBL_EPSILON )
/// Returns the minimum of two numbers
#define MIN(x, y) (((x) < (y))? (x) : (y))
/// Returns the maximum of two numbers
#define MAX(x, y) (((x) < (y))? (y) : (x))

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Amount of tries are given to input a value
#define TRY_AMOUNT 3
/// Block for input generating
#define TYPE_INFO(TYPE, SPEC) int DoInput_ ## TYPE (TYPE *var, const char name[])\
    {\
        assert (var);\
        printf ("#%s>", name);\
        \
        if (scanf (SPEC, var) == 1)\
            return 1;\
        else\
        {\
            printf ("#Invalid input\n");\
            errno = EIO;\
        }\
        \
        int i = 0;\
        for (; i < TRY_AMOUNT - 1; ++i)\
        {\
            printf ("#%s>", name);\
            if (scanf ("%*s" SPEC, var) == 1)\
                return 1;\
            else\
            {\
                printf ("#Invalid input\n");\
                errno = EIO;\
            }\
        }\
        \
        perror ("#Input error");\
        return 0;\
    }
#include "type_info.h"
#undef TYPE_INFO
#define INPUT (TYPE, VAR) DoInput_ ## TYPE (&(VAR), #VAR)

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

///Block for debug output generating
#define TYPE_INFO(TYPE, SPEC) void PrintDebug_ ## TYPE (TYPE var, const char name[])\
    {\
        printf ("(?)%s = " SPEC "\n", name, var);\
        getchar();\
    }
#include "type_info.h"
#undef TYPE_INFO

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Debug mode handler.
#if defined(DEBUG)
    /**
    *@brief Assertion of structures of different types.
    *
    *Let "type" be the structure name. For correct work of this macro you need:
    *
    *(1) void type_dump (type* what) | provides standard output for the structure;
    *
    *(2) bool type_OK (type* what) | provides validation of structure, returns true if the structure is OK, false otherwise.
    */
    #define ASSERT_OK(type, what)\
        assert (what);\
        if (!type ## _OK(what))\
        {\
            printf ("(!)ERROR: " #type " is broken!\nFunction: %s\nFile: %s\nLine: %d\n\n[LOG]\n\n", __FUNCTION__, __FILE__, __LINE__);\
            type ## _dump(what);\
            abort();\
        }

    #define DPRINT(TYPE, VAR) PrintDebug_ ## TYPE (VAR, #VAR)
#else
    #define DPRINT(TYPE, VAR) ;
    #define ASSERT_OK(type, what) ;
#endif // DEBUG

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#endif // MYLIB_H_INCLUDED
