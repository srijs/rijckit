.PHONY: sloc

lib: lex.h lex.c
	$(CC) -c -fPIC -std=c99 -Os lex.c -o lex.o
	$(CC) -s -nostdlib -shared lex.o -o liblex.so

bench: lex.h lex.c
	$(CC) -c -fPIC -std=c99 -Os lex.c -o lex.o -DBENCH
	$(CC) -nostdlib -shared lex.o -o liblex.so

run: lib
	cpp < lex.h | grep -v "#" > lex.hi
	luajit test.lua < lex.c
	rm lex.hi

verify: lex.h lex.c
	$(CC) -std=c99 -Os lex.c -S -o .asm.s
	cat .asm.s | grep call | wc -l
	cat .asm.s | grep j | wc -l
	rm .asm.s

sloc:
	cat lex.c | grep . | grep -v "//" | wc -l

docs: lex.c
	docco lex.c
