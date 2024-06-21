# Define the compiler and the flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Define the executable names
TTT = ttt
MYNC = mync

# Define the source files
TTT_SRC = ttt.c
MYNC_SRC = mync.c

# Define the object files
TTT_OBJ = $(TTT_SRC:.c=.o)
MYNC_OBJ = $(MYNC_SRC:.c=.o)

# Default target
all: $(TTT) $(MYNC)

# Compile ttt executable
$(TTT): $(TTT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compile mync executable
$(MYNC): $(MYNC_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Rule for cleaning up the build
clean:
	rm -f $(TTT) $(MYNC) $(TTT_OBJ) $(MYNC_OBJ)


.PHONY: all clean
