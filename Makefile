.PHONY: test all clean

CFLAGS += \
		  -I include \
		  -I deps/bk/include \
		  -I deps \
		  -g \
		  -O3 \
		  -fsanitize=undefined \
		  -fsanitize=address \
		  -std=c99 \
		  -pedantic \
		  -Wall \
		  -Werror \
		  -Wextra \
		  -Wno-missing-field-initializers
TEST_SOURCES = $(shell find test -name '*.c')
FORT_SOURCES = $(shell find src -name '*.c')
BK_SOURCES = \
		deps/bk/src/array.c \
		deps/bk/src/default_allocator.c \
		deps/bk/src/stdstream.c \
		deps/bk/src/fs/mem.c \


all: test bin/fort

clean:
	rm -rf bin

test: bin/test
	bin/test

bin/test: $(FORT_SOURCES) $(TEST_SOURCES) $(BK_SOURCES) deps/munit/munit.c | bin
	$(CC) $(CFLAGS) $^ -o $@

bin/fort: $(SOURCES) $(BK_SOURCES)
	$(CC) $(CFLAGS) $^ -o $@

bin:
	mkdir -p bin
