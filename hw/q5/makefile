# Makefile for printing directory tree structure

# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Output executable
TARGET = tree

# Source files
SRC = tree.c

# Object files
OBJ = $(SRC:.c=.o)

# Default target to build the executable
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

# Rule to compile the source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up object files and executable
clean:
	rm -f $(OBJ) $(TARGET)

# Optional: Run the program after building
run: $(TARGET)
	./$(TARGET)
