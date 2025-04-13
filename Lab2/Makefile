all : lab2

lab2: main.o string_parser.o command.o error.o
	gcc -g -o pseudo-shell main.o string_parser.o command.o error.o

main.o: main.c
	gcc -g -c main.c

string_parser.o: string_parser.c string_parser.h
	gcc -g -c string_parser.c

command.o: command.c command.h
	gcc -g -c command.c
	
error.o: error.c error.h
	gcc -g -c error.c

clean:
	rm -f pseudo-shell *.o
