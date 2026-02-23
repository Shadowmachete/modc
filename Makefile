CC := gcc
CFLAGS := -g -Wall -Werror -Wextra -pedantic

# headers
HDRS := utils.h lexer/lexer.h parser/parser.h symtab/symtab.h

# source files
SRCS := main.c utils.c lexer/lexer.c parser/parser.c symtab/symtab.c

# object files
OBJS := $(SRCS:.c=.o)

# executable
EXEC := main

all: $(EXEC)

$(EXEC): $(OBJS) $(HDRS) Makefile
	$(CC) -o $@ $(OBJS) $(CFLAGS)

clean:
	rm -f $(EXEC) $(OBJS)
