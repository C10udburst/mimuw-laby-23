.PHONY: test

rw: rw.c err.c
	gcc -o rw rw.c err.c -lpthread

test: rw check.py
	./rw | python3 check.py