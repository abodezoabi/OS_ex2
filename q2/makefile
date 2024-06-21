# Define the compiler and the flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Define the executable name
MYNC = mync

# Define the source file
MYNC_SRC = mync.c

# Define the object file
MYNC_OBJ = $(MYNC_SRC:.c=.o)

# Default target
all: $(MYNC)

# Compile mync executable
$(MYNC): $(MYNC_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Rule for cleaning up the build
clean:
	rm -f $(MYNC) $(MYNC_OBJ)

.PHONY: all clean
