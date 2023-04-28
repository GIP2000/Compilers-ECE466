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

types.o: parser/types.c parser/types.h
	gcc $(FLAGS) -c parser/types.c -o types.o

symbol_table.o: parser/symbol_table.c parser/symbol_table.h
	gcc $(FLAGS) -c parser/symbol_table.c -o symbol_table.o

ast.o: parser/ast.c parser/ast.h
	gcc $(FLAGS) -c parser/ast.c -o ast.o

lex.o: lex.yy.c
	gcc lex.yy.c -c -o lex.o

lex_test.o: lexer/lexer_test.c
	gcc $(FLAGS) -c lexer/lexer_test.c -o lex_test.o

quad.o: quads/quad.c quads/quad.h
	gcc $(FLAGS) -c quads/quad.c -o quad.o

ast_parser.o: quads/ast_parser.h quads/ast_parser.c
	gcc $(FLAGS) -c quads/ast_parser.c -o ast_parser.o

target.o: target_code/target.h target_code/target.c
	gcc $(FLAGS) -c target_code/target.c -o target.o

main.o: main.c
	gcc $(FLAGS) -c main.c -o main.o

gip: lex.o ast.o lexer_util.o parser.tab.o main.o symbol_table.o types.o quad.o ast_parser.o target.o
	gcc $(FLAGS) main.o lex.o lexer_util.o parser.tab.o ast.o symbol_table.o types.o quad.o ast_parser.o target.o -o gip

lex_test.out: lex.o lexer_util.o lex_test.o
	gcc $(FLAGS) lex.o lexer_util.o lex_test.o -o lex_test.out

clean:
	rm -f lex.yy.c lex.h parser.tab.* *.o *.out parser.output gip

