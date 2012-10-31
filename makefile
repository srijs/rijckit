.PHONY: sloc

test: lex.h lex.c test.c
	$(CC) -std=c99 -Os lex.c test.c -o test
	
verify: lex.h lex.c
	$(CC) -std=c99 -Os lex.c -S -o .asm.s
	cat .asm.s | grep call

sloc:
	cat lex.c | grep . | grep -v "//" | wc -l

docs: lex.c
	docco lex.c
