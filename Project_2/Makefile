CC = gcc
CFLAGS = -g -Wall

# Default target
all: part1 cpubound iobound

# Part1 executable
part1: Part1.o string_parser.o
	$(CC) $(CFLAGS) Part1.o string_parser.o -o part1

Part1.o: Part1.c
	$(CC) $(CFLAGS) -c Part1.c

string_parser.o: string_parser.c string_parser.h
	$(CC) $(CFLAGS) -c string_parser.c

# Test workload executables
cpubound: cpubound.c
	$(CC) $(CFLAGS) cpubound.c -o cpubound

iobound: iobound.c
	$(CC) $(CFLAGS) iobound.c -o iobound

# Clean rule
clean:
	rm -f *.o part1 cpubound iobound
