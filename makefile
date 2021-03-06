.PHONY: sloc docs

lib: lex.h lex.c
	$(CC) -Wall -Wextra -c -fPIC -std=c99 -Os lex.c -o lex.o
	$(CC) -s -nostdlib -shared lex.o -o liblex.so

bench: lex.h lex.c
	$(CC) -Wall -Wextra -c -fPIC -std=c99 -Os lex.c -o lex.o -DBENCH
	$(CC) -shared lex.o -o liblex.so

run: lib
	cpp < lex.h | grep -v "#" > scripts/lex.h
	cd scripts && luajit run.lua < ../lex.c

verify: lex.h lex.c
	$(CC) -std=c99 -Os lex.c -S -o .asm.s
	cat .asm.s | grep call | wc -l
	cat .asm.s | grep j | wc -l
	rm .asm.s

sloc:
	cat lex.c | grep . | grep -v "//" | wc -l

docs: lex.c
	docco lex.c
