all: lex_test.out
.PHONY: clean

parser.tab.c parser.tab.h: parser/parser.y
	bison -k -d parser/parser.y

parser.tab.o: parser.tab.c
	gcc -c parser.tab.c -o parser.tab.o

lex.yy.c: lexer/lexer.l parser.tab.h
	flex lexer/lexer.l

lexer_util.o: lexer/lexer_util.c lexer/lexer_util.h
	gcc -c lexer/lexer_util.c -o lexer_util.o

lex.o: lex.yy.c
	gcc lex.yy.c -c -o lex.o
lex_test.o: lexer/lexer_test.c
	gcc -c lexer/lexer_test.c -o lex_test.o

lex_test.out: lex.o lexer_util.o lex_test.o
	gcc lex.o lexer_util.o lex_test.o -o lex_test.out

clean:
	rm -rf lex.yy.c parser.tab.* *.o *.out

