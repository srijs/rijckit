.PHONY: bench sloc lex

lex:
	clang -O3 lex.c

sloc:
	cat lex.c | grep . | grep -v "//" | wc -l

docs: lexlex.c
	docco lex.c
