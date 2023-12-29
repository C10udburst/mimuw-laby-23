.PHONY: all clean

EXAMPLES := $(addprefix examples_build/,$(notdir $(basename $(wildcard examples/*.c))))
FILES_ALLOWED_FOR_CHANGE := $(shell cat files_allowed_for_change)
CHANGED_FILES := $(wildcard $(FILES_ALLOWED_FOR_CHANGE))
TEMPLATE_HASH := $(shell cat template_hash)
CFLAGS := --std=gnu11 -Wall -DDEBUG -lpthread -g
TESTS := $(wildcard tests/*.self)

CHANNEL_SRC := channel.c channel.h
MIMPI_COMMON_SRC := $(CHANNEL_SRC) mimpi_common.c mimpi_common.h
MIMPIRUN_SRC := $(MIMPI_COMMON_SRC) mimpirun.c
MIMPI_SRC := $(MIMPI_COMMON_SRC) mimpi.c mimpi.h

VALGRIND := valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes

all: mimpirun $(EXAMPLES) $(TESTS) pipecleaner repl

mimpirun: $(MIMPIRUN_SRC)
	gcc $(CFLAGS) -o $@ $(filter %.c,$^)

examples_build/%: examples/%.c $(MIMPI_SRC)
	mkdir -p examples_build
	gcc $(CFLAGS) -o $@ $(filter %.c,$^)

assignment.zip: $(CHANGED_FILES)
	zip assignment.zip $(CHANGED_FILES)

pipecleaner: pipecleaner.c
	gcc $(CFLAGS) -o $@ $(filter %.c,$^)

clean:
	rm -rf mimpirun assignment.zip examples_build repl pipecleaner

repl: repl.c $(MIMPI_SRC)
	gcc $(CFLAGS) -o $@ $(filter %.c,$^)

r: mimpirun repl pipecleaner
	rm -rf ./repl.sock.d || true
	./pipecleaner 20 1023 ./mimpirun 4 ./repl konsole

rgdb: mimpirun repl pipecleaner
	rm -rf ./repl.sock.d || true
	GDB_RANK=0 ./pipecleaner 20 1023 ./mimpirun 3 ./gdbone ./repl konsole

rv: mimpirun repl pipecleaner
	rm -rf ./repl.sock.d || true
	./pipecleaner 20 1023 $(VALGRIND) ./pipecleaner 8 13 ./mimpirun 3 $(VALGRIND) ./repl konsole

