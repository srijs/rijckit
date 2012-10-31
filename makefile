.PHONY: sloc

test: lex.h lex.c test.c
	$(CC) -std=c99 -Os lex.c test.c -o test

sloc:
	cat lex.c | grep . | grep -v "//" | wc -l

docs: lex.c
	docco lex.c
