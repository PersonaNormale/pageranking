# Compiler
CC = gcc

# Flags del compiler
CFLAGS = -Wall -g -O3

# Librerie
LIBS = -lpthread

# Source
SRCS = main.c utils/graph.c utils/nodebuffer.c utils/pagerank.c utils/threadpool.c

# File .o
OBJS = $(SRCS:.c=.o)

# Eseguibile
EXEC = pagerank

# Default target
all: $(EXEC)

# Linker
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Compilazione
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Pulizia
clean:
	rm -f $(OBJS) $(EXEC)

.PHONY: all clean
