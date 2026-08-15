CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -O2 -I../../include
.PHONY: test clean
test: test_keyboard
	./test_keyboard
test_keyboard: test_keyboard.c ../../hw/dev_keyboard.c
	$(CC) $(CFLAGS) $^ -o $@
clean:
	@rm -f $(wildcard test_keyboard test_keyboard.exe *.o)
