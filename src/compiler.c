#include "compiler.h"
#include <stdarg.h>
#include <stdlib.h>

struct lex_process_functions compiler_lex_function = {
    .next_char = compile_process_next_char,
    .peek_char = compile_process_peek_char,
    .push_char = compile_process_push_char,
};

void compiler_error(struct compiler_process* compiler , const char* msg, ...)
{
    va_list args;
    va_start(args,msg);
    vfprintf(stderr,msg,args);
    va_end(args);

    fprintf(stderr," on line %i, col %i in file %s\n",compiler->pos.line,compiler->pos.col,compiler->pos.filename);

    exit(-1);
}

void compiler_warning(struct compiler_process* compiler , const char* msg, ...)
{
    va_list args;
    va_start(args,msg);
    vfprintf(stderr,msg,args);
    va_end(args);

    fprintf(stderr," on line %i, col %i in file %s\n",compiler->pos.line,compiler->pos.col,compiler->pos.filename);

    exit(-1);
}

int compile_file(const char* filename, const char* out_fiolename, int flags)
{
    struct compiler_process* process = compile_process_create(filename, out_fiolename, flags);
    if(!process)
        return COMPILER_FAILED_WITH_ERRORS;

    //lexial 
    struct lex_process* lex_process = lex_process_create(process, &compiler_lex_function,NULL);
    if(!lex_process)
        return COMPILER_FAILED_WITH_ERRORS;
    
    if (lex(lex_process) != LEXICAL_ANALYSIS_ALL_OK)
    {
        return COMPILER_FAILED_WITH_ERRORS;
    }
    

    //parsing

    //code generation


    return COMPILER_FILE_COMPILED_OK;
}