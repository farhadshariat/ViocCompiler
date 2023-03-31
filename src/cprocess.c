#include "compiler.h"

struct compiler_process *compile_process_create(const char *filename, const char *out_fiolename, int flags)
{
    FILE* file = fopen(filename, "r");
    if (!file)
    {
        return NULL;
    }

    FILE* out_file = NULL;
    if (out_fiolename)
    {
        out_file = fopen(out_fiolename, "w");
        if(!out_file)
        {
            return NULL;
        }
    }

    //set up compiler process I/O
    struct compiler_process* compiler = calloc(1,sizeof(struct compiler_process));
    compiler->flags= flags;
    compiler->cfile.fp = file;
    compiler->ofile = out_file;

    return compiler;
}

char compile_process_next_char(struct lex_process* lex_process)
{
    struct compiler_process* compiler =lex_process->compiler;
    compiler->pos.col += 1;
    char c = getc(compiler->cfile.fp);
    if (c == '\n')
    {
        compiler->pos.line += 1; 
        compiler->pos.col = 0; 
    }

    return c;
}

char compile_process_peek_char(struct lex_process* lex_process)
{
    struct compiler_process* compiler =lex_process->compiler;
    char c = getc(compiler->cfile.fp);
    ungetc(c,compiler->cfile.fp);
    return c;
}

void compile_process_push_char(struct lex_process* lex_process, char c)
{
    struct compiler_process* compiler =lex_process->compiler;
    ungetc(c,compiler->cfile.fp);
}