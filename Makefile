.PHONY: test all clean src/builtins_fs.h

ifeq ($(NDEBUG), 1)
EXTRA_FLAGS= -O3 -DNDEBUG=1
else
EXTRA_FLAGS= -fsanitize=undefined -fsanitize=address
endif

CFLAGS += \
		  -I include \
		  -I deps/bk/include \
		  -I deps \
		  -g \
		  $(EXTRA_FLAGS) \
		  -std=c99 \
		  -pedantic \
		  -Wall \
		  -Werror \
		  -Wextra \
		  -Wno-missing-field-initializers

TEST_SOURCES = $(shell find test -name '*.c')
FORT_SOURCES = $(shell find src -name '*.c') src/builtins_fs.h
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
		$(filter %.c %.s,$^) \
		-o $@

src/builtins_fs.h: src/builtins.fs
	tools/incbin.bat src/builtins.fs src/builtins_fs.h fort_builtins_fs

bin:
	mkdir -p bin
