# Define the compiler and the flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Define the executable names
MYNC = mync
TTT = ttt

# Define the source files
MYNC_SRC = mync.c
TTT_SRC = ttt.c

# Define the object files
MYNC_OBJ = $(MYNC_SRC:.c=.o)
TTT_OBJ = $(TTT_SRC:.c=.o)

# Default target
all: $(MYNC) $(TTT)

# Compile mync executable
$(MYNC): $(MYNC_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compile ttt executable
$(TTT): $(TTT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Rule for cleaning up the build
clean:
	rm -f $(MYNC) $(MYNC_OBJ) $(TTT) $(TTT_OBJ)

.PHONY: all clean
