/*
 * lab1_skeleton.c
 *
 *  Created on: Nov 25, 2020
 *      Author: Guan, Xin, Monil
 */
/*
When you type something into the terminal (like a command), the computer has to
figure out what you're saying and what to do with it. That’s what this lab is practicing.
You're learning how to:
Take complex input from the user (like full commands typed into the console),
Break it down into smaller pieces (called tokens, like the command name and its options or arguments),
And get it ready for your program (or the computer) to understand and do something with it.
This is how real command-line interfaces (CLIs) work! For example, when you type:
cp file1.txt folder/
The shell:
Reads the whole line.
Breaks it up into:

"cp" → the command
"file1.txt" → the file to copy
"folder/" → where to put it

Sends that info to a program that actually does the copying.
You're building a tool (a helper function or structure) to do that kind of breaking-down for strings typed in.

What You'll Do
Read full lines of text typed into your program.
Split them into chunks (tokens).
Use those chunks in your code (maybe to simulate commands, or to process them in other labs).
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_parser.h"

#define _GNU_SOURCE

int main(int argc, char const *argv[])
{
	//checking for command line argument (exactly 1 [input file])
	if (argc != 2)
	{
		printf ("Usage ./lab1.exe intput.txt\n");
	}
	//opening file passed in the command line for reading
	FILE *inFPtr;
	inFPtr = fopen (argv[1], "r");

	//declare a buffer for reading each line from the file
	size_t len = 128; //initial buffer size
	char* line_buf = malloc (len); //dynamincally allocate space for the buffer

	//structures to store tokens
	command_line large_token_buffer; //will hold tokens split by ';'
	command_line small_token_buffer; //will hold tokens split by ' ' (space)

	int line_num = 0; //keep track of line number we are on in the file

	//loop until the file is over, line by line
	while (getline(&line_buf, &len, inFPtr) != -1) 
	{
		printf ("Line %d:\n", ++line_num); //print current line number

		//tokenize line buffer
		//large token is seperated by ";"
		//split the current line using ";" as the delimeter (e.x. multiple commands on one line)
		large_token_buffer = str_filler (line_buf, ";");
		//iterate through each large token
		//loop through each of the ";" -separated tokens
		for (int i = 0; large_token_buffer.command_list[i] != NULL; i++)
		{
			printf ("\tLine segment %d:\n", i + 1); //print the sub-segment #
			//ex.
			//ls -l; cd folder; echo Hello
			//Line segment 1: ls -l
			//Line segment 2: cd folder
			//Line segment 3: echo Hello
			//For cd folder:
			//Token 1: cd
			//Token 2: folder
			//Further split each command into space-separated tokens (like command name and arguments)
			small_token_buffer = str_filler (large_token_buffer.command_list[i], " ");

			//iterate through each smaller token to print (space-separated tokens)
			for (int j = 0; small_token_buffer.command_list[j] != NULL; j++)
			{
				printf ("\t\tToken %d: %s\n", j + 1, small_token_buffer.command_list[j]);
			}

			//free smaller tokens and reset variable
			free_command_line(&small_token_buffer);
			//reset small_token_buffer to 0 (clears it just in case)
			memset (&small_token_buffer, 0, 0);
		}

		//free smaller tokens and reset variable
		free_command_line (&large_token_buffer);
		memset (&large_token_buffer, 0, 0);
	}
	//close file
	fclose(inFPtr);
	//free line buffer to avoide memory leaks
	free (line_buf);
}
