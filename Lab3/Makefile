EXEC=pseudoshell
FLAGS=-g -o
CC=gcc

all: $(EXEC)

$(EXEC): main.o string_parser.o command.o
	$(CC) $(FLAGS) $@ $^

main.o: main.c
	$(CC) -c $^

%.o %.c %.h
	$(CC) -c $< 

command.o: command.c command.h
	gcc -c command.c


clean:
	rm -f *.o $(EXEC)