all: gip
.PHONY: clean

FLAGS = -Wall -Wextra

parser.tab.c parser.tab.h: parser/parser.y
	bison -v -k -d parser/parser.y

parser.tab.o: parser.tab.c
	gcc $(FLAGS) -c parser.tab.c -o parser.tab.o

lex.yy.c: lexer/lexer.l parser.tab.h
	flex lexer/lexer.l

lexer_util.o: lexer/lexer_util.c lexer/lexer_util.h
	gcc $(FLAGS) -c lexer/lexer_util.c -o lexer_util.o

ast.o: parser/ast.c parser/ast.h
	gcc $(FLAGS) -c parser/ast.c -o ast.o

lex.o: lex.yy.c
	gcc $(FLAGS) lex.yy.c -c -o lex.o
lex_test.o: lexer/lexer_test.c
	gcc $(FLAGS) -c lexer/lexer_test.c -o lex_test.o

main.o: main.c
	gcc $(FLAGS) -c main.c -o main.o

gip: lex.o ast.o lexer_util.o parser.tab.o main.o
	gcc $(FLAGS) main.o lex.o lexer_util.o parser.tab.o ast.o -o gip

lex_test.out: lex.o lexer_util.o lex_test.o
	gcc $(FLAGS) lex.o lexer_util.o lex_test.o -o lex_test.out

clean:
	rm -f lex.yy.c lex.h parser.tab.* *.o *.out parser.output gip

