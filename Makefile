all : lab1.exe
	
	
lab1.exe: lab1_skeleton.o string_parser.o
	gcc -o lab1.exe lab1_skeleton.o string_parser.o
	
	
lab1_skeleton.o: lab1_skeleton.c
	gcc -c lab1_skeleton.c
	
string_parser.o: string_parser.c string_parser.h
	gcc -c string_parser.c
	
clean:
	rm -f core *.o lab1.exe
