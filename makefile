.PHONY: sloc

test: lex.h lex.c test.c
	$(CC) -std=c99 -Os lex.c test.c -o test

run: test
	./test < lex.c

verify: lex.h lex.c
	$(CC) -std=c99 -O3 lex.c -S -o .asm.s
	cat .asm.s | grep call | wc -l
	cat .asm.s | grep j | wc -l
	rm .asm.s

sloc:
	cat lex.c | grep . | grep -v "//" | wc -l

docs: lex.c
	docco lex.c
