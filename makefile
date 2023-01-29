all: lex.yy.c lex.out
.PHONY: clean

lex.yy.c: lexer/lexer.l
	flex lexer/lexer.l

lex.out: lex.yy.c
	gcc lex.yy.c -o lex.out

clean:
	rm -rf lex.yy.c lex.out

