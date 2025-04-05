/*
 * string_parser.c
 *
 *  Created on: Nov 25, 2020
 *      Author: gguan, Monil
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_parser.h"

#define _GUN_SOURCE
//char* buf: ptr to str you want to break into tokens
//const char* delim: a C str conatining the characters that act as separators (delimiters)
// -> think " " for spaces, "; " for semicolens
// both of these count as a delimeter
int count_token (char* buf, const char* delim)
{
	//TODO：
	/*
	*	#1.	Check for NULL string
	*	#2.	iterate through string counting tokens
	*		Cases to watchout for
	*			a.	string start with delimeter
	*			b. 	string end with delimeter
	*			c.	account NULL for the last token
	*	#3. return the number of token (note not number of delimeter)
	*/
			//ls -l; cd folder; echo Hello
			//Line segment 1: ls -l
			//Line segment 2: cd folder
			//Line segment 3: echo Hello
			//For cd folder:
			//Token 1: cd
			//Token 2: folder
	if (buf == NULL){
		return -1; //indicate error
	}
	int num_tokens = 0;
	char* copy = strdup(buf); //copy the buffer since strtok_r modifies it
	char* saveptr;// Declare a pointer to store the current position during tokenization
	//strtok_r parameters: 
	//copy: str to tokenize (it gets modified), set to NULL after first call to continue tokenizing str
	//delim: a str containing all delimeter characters, " " & "; "
	//&saveptr: a ptr to char* that keeps track of where strtok-r left off between teh calls(used in loop)
	char* token = strtok_r(copy, delim, &saveptr);//tokenize 'copy' using 'delim', store pos in 'saveptr'
	//check if first token is not NULL (leading delimenter case)
	if (token != NULL){
		//if token exists (ex. ls does exist), we count it
		num_tokens++; 
	}
	//loop for finding all tokens besides the 1st
	while (token != NULL){
		token = strtok_r(NULL, delim, &saveptr);
		if (token != NULL) {
			num_tokens++;
		}
	}
	free(copy); //free copied buffer
	return num_tokens;
}

command_line str_filler (char* buf, const char* delim)
{
	//TODO：
	/*
	*	#1.	create command_line variable to be filled and returned
	*	#2.	count the number of tokens with count_token function, set num_token. 
    *           one can use strtok_r to remove the \n at the end of the line.
	*	#3. malloc memory for token array inside command_line variable
	*			based on the number of tokens.
	*	#4.	use function strtok_r to find out the tokens 
    *   #5. malloc each index of the array with the length of tokens,
	*			fill command_list array with tokens, and fill last spot with NULL.
	*	#6. return the variable.
	*/
	command_line cmd;
	if (buf == NULL) {
		cmd.num_token = 0;
		cmd.command_list = NULL;
		return cmd;
	}
	// step 2
	int num_tokens = count_token(buf, delim);
	if (num_tokens <= 0){
		cmd.num_token = 0;
		cmd.command_list = NULL;
		return cmd;
	}

	cmd.num_token = num_tokens;
	char* saveptr;
	char* line = strtok_r(buf, "\n", &saveptr); // remove \n by tokenizing at '\n'
	buf = line; // update tbuf to cleaned, no \n, version

	// step 3
	cmd.command_list = malloc(sizeof(char*) * (num_tokens + 1));
	if (cmd.command_list == NULL){
		perror("failed to allocate memory for command_list");
		exit(EXIT_FAILURE);
	}
	// step 4
	char* token = strtok_r(buf, delim, &saveptr);
	int i = 0;
	// step 5
	while (token != NULL){
		cmd.command_list[i] = malloc(strlen(token) + 1);
		if (cmd.command_list[i] == NULL){
			perror("Failed to allocate memory for token");
			exit(EXIT_FAILURE);
		}
		strcpy(cmd.command_list[i], token); //copy token into the allocated space
		// get next token, size of token may be different so we adjust which token
		// the next malloc call will taken in
		token = strtok_r(NULL, delim, &saveptr);
		i++;
	}
	cmd.command_list[i] = NULL; //add NULL character at end
	// return command_line variable
	return cmd;
}


void free_command_line(command_line* command)
{
	//TODO：
	/*
	*	#1.	free the array base num_token
	*/
	for (int i = 0; i < command->num_token; i++){
		free(command->command_list[i]);
	}
	free(command->command_list);
}
  
 