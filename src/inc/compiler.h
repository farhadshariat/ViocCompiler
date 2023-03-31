#ifndef VIOCCOMPILER_H
#define VIOCCOMPILER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "vector.h"
#include "buffer.h"

#define S_EQ(str1,str2) \
        (str1 && str2 && (strcmp(str1,str2) == 0)) 

#define NUMERIC_CASE \
    case '0':        \    
    case '1':        \    
    case '2':        \    
    case '3':        \    
    case '4':        \    
    case '5':        \    
    case '6':        \    
    case '7':        \    
    case '8':        \    
    case '9'        

#define OPERATOR_CASE_EXCLUDING_DIVISION \
    case '+':                            \    
    case '-':                            \    
    case '*':                            \    
    case '>':                            \    
    case '<':                            \    
    case '^':                            \    
    case '%':                            \    
    case '!':                            \    
    case '=':                            \    
    case '~':                            \
    case '|':                            \
    case '&':                            \
    case '(':                            \
    case '[':                            \
    case ',':                            \
    case '.':                            \
    case '?'                             \


#define SYMBOL_CASE \
    case '{':       \
    case '}':       \
    case ':':       \
    case ';':       \
    case '#':       \
    case '\\':      \
    case ')':       \
    case ']'       

struct pos
{
    int line;
    int col;
    const char *filename;
};

enum
{
    LEXICAL_ANALYSIS_ALL_OK,
    LEXICAL_ANALYSIS_INPUT_ERROR,
};

enum
{
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_KEYWORD,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_SYMBOL,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_COMMENT,
    TOKEN_TYPE_NEWLINE,
};

enum
{
    COMPILER_FILE_COMPILED_OK,
    COMPILER_FAILED_WITH_ERRORS
};

struct token
{
    int type;
    int flags;
    struct pos pos;

    union
    {
        char cval;                //'A'
        char *sval;               //"Hello"
        unsigned int inum;        // 4343
        unsigned long lnum;       // 4343234554534
        unsigned long long llnum; // 3463645645634535
    };

    // * a != *a
    bool whitespace;

    //(10+20+30)
    const char *between_brackets;
};

struct lex_process;
typedef char (*LEX_PROCESS_NEXT_CHAR)(struct lex_process *process);
typedef char (*LEX_PROCESS_PEEK_CHAR)(struct lex_process *process);
typedef void (*LEX_PROCESS_PUSH_CHAR)(struct lex_process *process, char c);

struct lex_process_functions
{
    LEX_PROCESS_NEXT_CHAR next_char;
    LEX_PROCESS_PEEK_CHAR peek_char;
    LEX_PROCESS_PUSH_CHAR push_char;
};

struct lex_process
{
    struct pos pos;
    struct vector *token_vec;
    struct compiler_process *compiler;

    /*
        ((20)) => 2
    */
    int current_expression_count;
    struct buffer *parantheses_buffer;
    struct lex_process_functions *function;

    // priavte data for lexer
    void *private;
};

struct compiler_process
{
    int flags;

    struct pos pos;
    struct compiler_process_input_file
    {
        FILE *fp;
        const char *abs_path;

    } cfile;

    FILE *ofile;
};

// compiler prototype
int compile_file(const char *filename, const char *out_fiolename, int flags);
struct compiler_process *compile_process_create(const char *filename, const char *out_fiolename, int flags);
// compiler warning, error handling
 void compiler_error(struct compiler_process *compiler, const char *msg, ...);
 void compiler_warning(struct compiler_process *compiler, const char *msg, ...);

// cprocess prototype
char compile_process_next_char(struct lex_process *lex_process);
char compile_process_peek_char(struct lex_process *lex_process);
void compile_process_push_char(struct lex_process *lex_process, char c);

// lex_process proto type
struct lex_process *lex_process_create(struct compiler_process *compiler, struct lex_process_functions *functions, void *private);
void lex_process_free(struct lex_process *process);
void *lex_process_private(struct lex_process *process);
struct vector *lex_process_tokens(struct lex_process *process);

// lex proto type
int lex(struct lex_process *process);

//token functions
bool token_is_keyword(struct token* token, const char* value);

#endif