#note: you can aliasing mingw32-make to make(go to c:\\MinGW\bin and rename mingw32-make to make)
CC=gcc
CClang=clang
CPP=g++
CFLAGS=-g -Wall

TARGETNAME=main

OBJSdir = obj
SRCdir = src
INCdir = ./src/inc/

ASMFLAGS=-fverbose-asm #-fverbose-asm comment all the c code next to assembly code (very useful)
#gcc -S -masm=intel -Og -fverbose-asm main.c (intel assembly syntax)
#clang -S -mllvm --x86-asm-syntax=intel main.c (clang tool intel assembly syntax)
# -Woverloaded-virtual is Working for C++		 
# dependency file maker "-MD -MP"
#..... LIST
# -Wextra -Wfloat-equal -Wundef -Wcast-align -Wwrite-strings -Wlogical-op -Wmissing-declarations -Wredundant-decls -Wshadow
#..... LIST

# wildcard finds all file which is finiesh by certain postfix (.s .c .h .d)
ASM_SRCS = $(wildcard *.s)

SRCS = $(wildcard $(SRCdir)/*.c)
OBJS := $(SRCS:$(SRCdir)/%.c=$(OBJSdir)/%.o)
INCS = $(wildcard $(SRCdir)/$(INCdir)/*.h)


#tell the shell that it does not need to worry about making file they are just commands(.PHONY all)

all:$(OBJSdir) $(TARGETNAME)

#region Assembly Configuration 

#region assembly code generator				#.......... if you want assembly script for your code
# main: $(SRCS)								
# 	$(CC) $(CFLAGS) -S $(ASMFLAGS) $^
#end region

#region assembly exe builder				#.......... if you want to ru your assembly
# asmEXE: $(ASM_SRCS)
# 	$(CC) $(CFLAGS) -o $@ $^
#end region

#end region 

$(OBJSdir)/%.o: $(SRCdir)/%.c
	$(CC) $(CFLAGS) -I $(INCdir) -c $^ -o $@

main: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^



# -include $(SRCS:%.c=%.d)

clean:
	rm -rf $(OBJSdir)/*
	rm ./$(TARGETNAME)