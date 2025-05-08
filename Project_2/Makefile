CC = gcc
CFLAGS = -g -Wall

# Default target: build part1 and part2 only
all: part1 part2

# Part1 build
part1: part1.o string_parser.o
	$(CC) $(CFLAGS) part1.o string_parser.o -o part1

part1.o: part1.c
	$(CC) $(CFLAGS) -c part1.c

# Part2 build
part2: part2.o string_parser.o
	$(CC) $(CFLAGS) part2.o string_parser.o -o part2

part2.o: part2.c
	$(CC) $(CFLAGS) -c part2.c

# Shared object
string_parser.o: string_parser.c string_parser.h
	$(CC) $(CFLAGS) -c string_parser.c

# Optional run targets
run1: part1
	./part1 input.txt

run2: part2
	./part2 input.txt

# Clean everything
clean:
	rm -f *.o part1 part2