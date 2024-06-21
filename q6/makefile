# Define the compiler and the flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -fprofile-arcs -ftest-coverage
LDFLAGS = -lgcov

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
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile ttt executable
$(TTT): $(TTT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Rule for cleaning up the build
clean:
	rm -f $(MYNC) $(MYNC_OBJ) $(TTT) $(TTT_OBJ) *.gcno *.gcda *.gcov

# Rule for generating code coverage report
coverage: $(MYNC) $(TTT)
	./$(MYNC) # Run tests for mync
	./$(TTT) # Run tests for ttt
	gcov $(MYNC_SRC)
	gcov $(TTT_SRC)

.PHONY: all clean coverage
