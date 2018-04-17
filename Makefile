.PHONY: test all clean

CFLAGS += \
		  -I include \
		  -I deps/bk/include \
		  -I deps \
		  -g \
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
REPL_SOURCES = fort.c

all: test bin/fort

clean:
	rm -rf bin

test: bin/test
	bin/test

bin/test: bin/fort.so $(TEST_SOURCES) deps/munit/munit.c | bin
	$(CC) $(CFLAGS) $^ -o $@

bin/fort: bin/fort.so $(REPL_SOURCES) | bin
	$(CC) $(CFLAGS) $^ -o $@

bin/fort.so: $(FORT_SOURCES) | bin
	$(CC) \
		$(CFLAGS) \
		-DFORT_DYNAMIC=1 \
		-DFORT_BUILD=1 \
		-fPIC \
		-shared \
		-fvisibility=hidden \
		-o $@ \
		$^

bin:
	mkdir -p bin
