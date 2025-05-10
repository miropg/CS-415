CC = gcc
CFLAGS = -g -Wall

# Default target: build all parts
all: part1 part2 part3 part4

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

# Part3 build
part3: part3.o string_parser.o
	$(CC) $(CFLAGS) part3.o string_parser.o -o part3

part3.o: part3.c
	$(CC) $(CFLAGS) -c part3.c

# Part4 build (same deps as part3, but includes /proc enhancements)
part4: part4.o string_parser.o
	$(CC) $(CFLAGS) part4.o string_parser.o -o part4

part4.o: part4.c
	$(CC) $(CFLAGS) -c part4.c

# Shared object
string_parser.o: string_parser.c string_parser.h
	$(CC) $(CFLAGS) -c string_parser.c

# Optional run targets
run1: part1
	./part1 input.txt

run2: part2
	./part2 input.txt

run3: part3
	./part3 input.txt

run4: part4
	./part4 input.txt

# Clean everything
clean:
	rm -f *.o part1 part2 part3 part4
