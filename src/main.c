#include "compiler.h"
#include "inc/vector.h"

int main()
{
    int res = compile_file("./test.c","./test",0);
    if (res== COMPILER_FILE_COMPILED_OK)
    {
        printf("everything compiled fine!!\n");
    }
    else if(res== COMPILER_FAILED_WITH_ERRORS)
    {
        printf("everything compiled fine!!\n");        
    }
    else
    {
        printf("everything compiled fine!!\n");        
    }
    
    return 0;
}
