CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11 -g -O0 -I src
LDFLAGS =

SRC     = src/malloc.c
OBJ     = $(SRC:.c=.o)
TEST    = tests/test

.PHONY: all clean test asan valgrind

all: $(TEST)

$(TEST): tests/test.c $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.c src/%.h
	$(CC) $(CFLAGS) -c $< -o $@

asan: CFLAGS += -fsanitize=address,undefined
asan: clean $(TEST)
	$(TEST)

test: $(TEST)
	./$(TEST)

valgrind: $(TEST)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TEST)

clean:
	rm -f $(OBJ) $(TEST)
