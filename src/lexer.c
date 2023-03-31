#include "compiler.h"
#include <string.h>
#include <assert.h>

// define list
#define LEX_GETC_IF(buffer, c, exp)     \
    for (c = peekc(); exp; c = peekc()) \
    {                                   \
        buffer_write(buffer, c);        \
        nextc();                        \
    }

// lexer.c prototypes
struct token *read_next_token();

// static values and methods

static struct lex_process *lex_process;
static struct token tmp_token;

static char peekc()
{
    return lex_process->function->peek_char(lex_process);
}

static char nextc()
{
    char c = lex_process->function->next_char(lex_process);
    lex_process->pos.col += 1;
    if (c == '\n')
    {
        lex_process->pos.line += 1;
        lex_process->pos.col = 1;
    }

    return c;
}

static void pushc(char c)
{
    lex_process->function->push_char(lex_process, c);
}

static char assert_next_char(char c)
{
    char next_c = nextc();
    assert(c == next_c);
    return next_c;
}

static struct pos lex_file_position()
{
    return lex_process->pos;
}

struct token *token_create(struct token *token)
{
    memcpy(&tmp_token, token, sizeof(struct token));

    // fill up token pos from lex process pos
    tmp_token.pos = lex_file_position();

    return &tmp_token;
}

static struct token *lexer_last_token()
{
    return vector_back_or_null(lex_process->token_vec);
}

static struct token *handle_whitespace()
{
    struct token *last_token = lexer_last_token();
    if (last_token)
    {
        last_token->whitespace = true;
    }

    nextc();
    return read_next_token();
}

const char *read_number_str()
{
    // HTC:
    // const char *num = NULL;
    struct buffer *buffer = buffer_create();
    char c = peekc();
    LEX_GETC_IF(buffer, c, (c >= '0' && c <= '9'));

    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}

unsigned long long read_number()
{
    const char *s = read_number_str();
    return atoll(s);
}

struct token *token_make_number_for_value(unsigned long number)
{
    return token_create(&(struct token){.type = TOKEN_TYPE_NUMBER, .llnum = number});
}

struct token *token_make_number()
{
    return token_make_number_for_value(read_number());
}

struct token *token_make_string(char start_delim, char end_delim)
{
    struct buffer *buf = buffer_create();
    assert(nextc() == start_delim);
    char c = nextc();

    for (; c != end_delim && c != EOF; c = nextc())
    {
        if (c == '\\')
        {
            continue;
        }

        buffer_write(buf, c);
    }

    buffer_write(buf, 0x00);
    return token_create(&(struct token){.type = TOKEN_TYPE_STRING, .sval = buffer_ptr(buf)});
}

// only one character
static bool op_treated_as_one(char op)
{
    return op == '(' || op == '[' || op == ',' || op == '.' || op == '*' || op == '?';
}

static bool is_single_operator(char op)
{
    return op == '+' ||
           op == '-' ||
           op == '/' ||
           op == '*' ||
           op == '=' ||
           op == '>' ||
           op == '<' ||
           op == '|' ||
           op == '&' ||
           op == '^' ||
           op == '%' ||
           op == '!' ||
           op == '(' ||
           op == '[' ||
           op == ',' ||
           op == '.' ||
           op == '~' ||
           op == '?';
}

bool op_valid(const char *op)
{
    return S_EQ(op, "+") ||
           S_EQ(op, "-") ||
           S_EQ(op, "*") ||
           S_EQ(op, "/") ||
           S_EQ(op, "!") ||
           S_EQ(op, "^") ||
           S_EQ(op, "+=") ||
           S_EQ(op, "-=") ||
           S_EQ(op, "*=") ||
           S_EQ(op, "/=") ||
           S_EQ(op, ">>") ||
           S_EQ(op, "<<") ||
           S_EQ(op, ">=") ||
           S_EQ(op, "<=") ||
           S_EQ(op, ">") ||
           S_EQ(op, "<") ||
           S_EQ(op, "||") ||
           S_EQ(op, "&&") ||
           S_EQ(op, "|") ||
           S_EQ(op, "&") ||
           S_EQ(op, "++") ||
           S_EQ(op, "--") ||
           S_EQ(op, "=") ||
           S_EQ(op, "!=") ||
           S_EQ(op, "==") ||
           S_EQ(op, "->") ||
           S_EQ(op, "(") ||
           S_EQ(op, "[") ||
           S_EQ(op, ",") ||
           S_EQ(op, ".") ||
           S_EQ(op, "...") ||
           S_EQ(op, "~") ||
           S_EQ(op, "?") ||
           S_EQ(op, "%");
}

void read_op_flush_back_keep_first(struct buffer *buffer)
{
    const char *data = buffer_ptr(buffer);
    int len = buffer->len;
    for (size_t i = len - 1; i >= 1; i--)
    {
        if (data[i] == 0x00)
        {
            continue;
        }

        pushc(data[i]);
    }
}

// read operators and check if they contain one or two char (+= or =)
const char *read_op()
{
    bool single_operator = true;
    char op = nextc();
    struct buffer *buffer = buffer_create();
    buffer_write(buffer, op);

    if (!op_treated_as_one(op))
    {
        op = peekc();
        if (is_single_operator(op))
        {
            buffer_write(buffer, op);
            nextc();

            single_operator = false;
        }
    }

    buffer_write(buffer, 0x00);
    char *ptr = buffer_ptr(buffer);
    if (!single_operator)
    {
        if (!op_valid(ptr))
        {
            read_op_flush_back_keep_first(buffer);
            ptr[1] = 0x00;
        }
    }
    else if (!op_valid(ptr))
    {
        compiler_error(lex_process->compiler, "The operator %s is not valid\n", ptr);
    }

    return ptr;
}

static void lex_new_expression()
{
    lex_process->current_expression_count++;
    if (lex_process->current_expression_count == 1)
    {
        lex_process->parantheses_buffer = buffer_create();
    }
}

static void lex_finish_expression()
{
    lex_process->current_expression_count--;
    if (lex_process->current_expression_count < 0)
    {
        compiler_error(lex_process->compiler, "You closed an expression that you never opened\n");
    }
}

bool lex_is_in_expression()
{
    return lex_process->current_expression_count > 0;
}

bool is_keyword(const char *str)
{
    return S_EQ(str, "unsigned") ||
           S_EQ(str, "signed") ||
           S_EQ(str, "char") ||
           S_EQ(str, "short") ||
           S_EQ(str, "int") ||
           S_EQ(str, "long") ||
           S_EQ(str, "float") ||
           S_EQ(str, "double") ||
           S_EQ(str, "void") ||
           S_EQ(str, "struct") ||
           S_EQ(str, "union") ||
           S_EQ(str, "static") ||
           S_EQ(str, "__ignore_typecheck") ||
           S_EQ(str, "return") ||
           S_EQ(str, "include") ||
           S_EQ(str, "sizeof") ||
           S_EQ(str, "if") ||
           S_EQ(str, "else") ||
           S_EQ(str, "while") ||
           S_EQ(str, "for") ||
           S_EQ(str, "do") ||
           S_EQ(str, "break") ||
           S_EQ(str, "switch") ||
           S_EQ(str, "case") ||
           S_EQ(str, "default") ||
           S_EQ(str, "goto") ||
           S_EQ(str, "typedef") ||
           S_EQ(str, "const") ||
           S_EQ(str, "extern") ||
           S_EQ(str, "restrict") ||
           S_EQ(str, "volatile");
}

static struct token *token_make_operator_or_string()
{
    char op = peekc();
    if (op == '<')
    {
        struct token *last_token = lexer_last_token();
        if (token_is_keyword(last_token, "include"))
        {
            return token_make_string('<', '>');
        }
    }
    struct token *token = token_create(&(struct token){.type = TOKEN_TYPE_OPERATOR, .sval = read_op()});
    if (op == '(')
    {
        lex_new_expression();
    }
    return token;
}

struct token *token_make_one_line_comment()
{
    struct buffer *buffer = buffer_create();
    char c = 0;
    LEX_GETC_IF(buffer, c, c != '\n' && c != EOF);

    return token_create(&(struct token){.type = TOKEN_TYPE_COMMENT, .sval = buffer_ptr(buffer)});
}

struct token *token_make_multiple_line_comment()
{
    struct buffer *buffer = buffer_create();
    char c = 0;

    while (1)
    {
        LEX_GETC_IF(buffer, c, c != '*' && c != EOF);
        if (c == EOF)
        {
            compiler_error(lex_process->compiler, "You did not close this multiline comment\n");
        }
        else if (c == '*')
        {
            nextc();

            if (peekc() == '/')
            {
                nextc();
                break;
            }
        }
    }

    return token_create(&(struct token){.type = TOKEN_TYPE_COMMENT, .sval = buffer_ptr(buffer)});
}

struct token *handle_comment()
{
    char c = peekc();
    if (c == '/')
    {
        nextc();

        if (peekc() == '/')
        {
            nextc();
            return token_make_one_line_comment();
        }
        else if (peekc() == '*')
        {
            nextc();
            return token_make_multiple_line_comment();
        }

        pushc('/');
        return token_make_operator_or_string();
    }
    else
    {
        return NULL;
    }
}

static struct token *token_make_symbol()
{
    char c = nextc();
    if (c == ')')
    {
        lex_finish_expression();
    }

    return token_create(&(struct token){.type = TOKEN_TYPE_SYMBOL, .cval = c});
}

static struct token *token_make_identifier_or_keyword()
{
    struct buffer *buffer = buffer_create();
    char c = 0;
    LEX_GETC_IF(buffer, c, (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_');

    // add null terminate
    buffer_write(buffer, 0x00);

    // string of keyword or identifier
    char *s = buffer_ptr(buffer);

    // check  if this is keyword
    if (is_keyword(s))
    {
        return token_create(&(struct token){.type = TOKEN_TYPE_KEYWORD, .sval = s});
    }

    return token_create(&(struct token){.type = TOKEN_TYPE_IDENTIFIER, .sval = s});
}

struct token *read_special_token()
{
    char c = peekc();
    if (isalpha(c) || c == '_')
    {
        return token_make_identifier_or_keyword();
    }

    return NULL;
}

struct token *token_make_new_line()
{
    nextc();
    return token_create(&(struct token){.type = TOKEN_TYPE_IDENTIFIER});
}

char lex_get_escaped_char(char c)
{
    char co = 0;
    switch (c)
    {
    case 'n':
        co = '\n';
        break;
    case '\\':
        co = '\\';
        break;
    case 't':
        co = '\t';
        break;
    case '\'':
        co = '\'';
        break;
    }

    return co;
}

struct token *token_make_quote()
{
    assert_next_char('\'');
    char c = nextc();
    if (c == '\\')
    {
        c = nextc();
        c = lex_get_escaped_char(c);
    }

    if (nextc() != '\'')
    {
        compiler_error(lex_process->compiler, "You opened quote(') but did not close it\n");
    }

    return token_create(&(struct token){.type = TOKEN_TYPE_NUMBER, .cval = c});
}

struct token *read_next_token()
{
    struct token *token = NULL;
    char c = peekc();

    token = handle_comment();
    if (token)
    {
        return token;
    }

    switch (c)
    {
    NUMERIC_CASE:
        token = token_make_number();
        break;

    OPERATOR_CASE_EXCLUDING_DIVISION:
        token = token_make_operator_or_string();
        break;

    SYMBOL_CASE:
        token = token_make_symbol();
        break;

    case '\'':
        token = token_make_quote();
        break;

    case '"':
        token = token_make_string('"', '"');
        break;

    case ' ':
    case '\t':
        token = handle_whitespace();
        break;

    case '\n':
    case '\r':
        token = token_make_new_line();
        break;

    case EOF:
        // end of lexial analysis
        break;

    default:
        token = read_special_token();
        if (!token)
        {
            compiler_error(lex_process->compiler, "Unexpected Token\n", c);
        }
        break;
    }

    return token;
}

static void print_tokens(struct vector *token_vec)
{
    for (size_t i = 0; i < token_vec->count; i++)
    {
        // struct token token = ((struct token*)(token_vec->data[i]));
        // printf("<value = %s , type = %d>",(struct token*)token_vec->data[i]    );
    }
}

int lex(struct lex_process *process)
{
    process->current_expression_count = 0;
    process->parantheses_buffer = NULL;
    lex_process = process;
    process->pos.filename = process->compiler->cfile.abs_path;
    struct token *token = read_next_token();
    while (token)
    {
        vector_push(process->token_vec, token);
        token = read_next_token();
    }

    return LEXICAL_ANALYSIS_ALL_OK;
}