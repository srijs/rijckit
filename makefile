.PHONY: bench sloc lex

test: lex.h lex.c test.c
	clang -O3 lex.c test.c -o test

sloc:
	cat lex.c | grep . | grep -v "//" | wc -l

docs: lex.c
	docco lex.c
