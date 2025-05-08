CC = gcc
CFLAGS = -g -Wall

# Default target: just build everything
all: part1 part2 cpubound iobound

# Part1 build
part1: Part1.o string_parser.o
	$(CC) $(CFLAGS) Part1.o string_parser.o -o part1

Part1.o: Part1.c
	$(CC) $(CFLAGS) -c Part1.c

# Part2 build
part2: Part2.o string_parser.o
	$(CC) $(CFLAGS) Part2.o string_parser.o -o part2

Part2.o: Part2.c
	$(CC) $(CFLAGS) -c Part2.c

# Shared object
string_parser.o: string_parser.c string_parser.h
	$(CC) $(CFLAGS) -c string_parser.c

# Workload binaries
cpubound: cpubound.c
	$(CC) $(CFLAGS) cpubound.c -o cpubound

iobound: iobound.c
	$(CC) $(CFLAGS) iobound.c -o iobound

# Optional run targets (you can choose to use or ignore)
run1: part1
	./part1 input.txt

run2: part2
	./part2 input.txt

# Clean everything
clean:
	rm -f *.o part1 part2 cpubound iobound
