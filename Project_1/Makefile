all : pseudo-shell

pseudo-shell: main.o string_parser.o command.o error.o
	gcc -o pseudo-shell main.o string_parser.o command.o error.o

main.o: main.c
	gcc -c main.c

string_parser.o: string_parser.c string_parser.h
	gcc -c string_parser.c

command.o: command.c command.h
	gcc -c command.c
	
error.o: error.c error.h
	gcc -c error.c

clean:
	rm -f pseudo-shell *.o
