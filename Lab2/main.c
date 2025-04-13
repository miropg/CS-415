#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>   // Defines O_WRONLY, O_CREAT, O_TRUNC
#include <unistd.h>  // Defines dup2, STDOUT_FILENO, write, close
#include "string_parser.h"
#include "command.h"
#include "error.h"

#define _GNU_SOURCE

// NOTE
// open insead of fopen
// perror might be ok, if not, made your own error function
// close instead fclose
// write instead of fprintf

void execute_command(command_line s_tok_buf){
    if (s_tok_buf.command_list[0] != NULL) {
        // double check user wrote valid command before checking which command it is
        if (strcmp(s_tok_buf.command_list[0], "lfcat") == 0) {
            if (param_count_valid("lfcat", s_tok_buf.num_token))
                lfcat();
            else
                err_params("lfcat");
        } else {
            err_unrecognized(s_tok_buf.command_list[0]);
        }
    }
    free_command_line(&s_tok_buf);
}

// trimming white space function
void trim(char *str) {
    if (str == NULL){
        return;
    }
    //trim leading space
    while (isspace((unsigned char)*str)){
        str++;
    }
    //if all spaces
    if (*str == 0){
        return;
    }
    //now trim trailing space
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    //write new null terminator
    *(end + 1) = '\0';
}
/*
// helper function for allocating memory
char* allocate_buffer(size_t size) {
    char *buf = malloc(size);
    if (buf == NULL) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    return buf;
}
// helper function to allocate memory when there are also files to close

char* allocate_buffer_w_files(size_t size, FILE *inFPtr, FILE *outFPtr) {
    char *buf = malloc(size);
    if (buf == NULL) {
        perror("malloc failed");
        if (inFPtr) fclose(inFPtr);
        if (outFPtr) fclose(outFPtr);
        exit(EXIT_FAILURE);
    }
    return buf;
}
*/
void file_mode(const char *filename){
    
    FILE *in_fd = fopen(filename, "r");
    if (!in_fd) {
        const char *err = "failier opening input file\n";
        write(2, err, strlen(err)); //does this need write?
        return;
    }
    
    int out_fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd == -1) {
        const char *err = "failier opening output file\n";
        write(2, err, strlen(err));
        fclose(in_fd); //if fopen used, you need fclose
        return;
    }

    // Redirect stdout (file descriptor 1) to the output file descriptor.
    // essentially duplicates file descriptor out_fd into std output file
    // descriptor (STDOUT_FILENO, which is 1)
    //after this call, anything printed to stdout will go to "output.txt"(out_fd)
    if (dup2(out_fd, STDOUT_FILENO) == -1) {
        const char *err = "Error redirecting stdout\n";
        write(2, err, strlen(err));
        fclose(in_fd);
        close(out_fd);
        return;
    }
    //so that errors print out to file as well
    if (dup2(out_fd, STDERR_FILENO) == -1) {
        const char *err = "Error redirecting stderr\n";
        write(2, err, strlen(err));
        fclose(in_fd);
        close(out_fd);
        return;
    }
    //now we close out_fd becayse stdout now points to output.txt
    close(out_fd);
    // Allocate a buffer for reading lines
    // allocate memory on the heap
    size_t len = 0;
    char *line_buf = NULL;
    // declare variables to store tokens
    /*
    Token Type   Split By	     Represents	             Example
    Large Token	  ;	    Full individual command	  "ls -l" or "cd folder"
    Small Token	 " "	  Command + each argument	 "cd", "folder"
    */
    command_line l_tok_buf;
    command_line s_tok_buf;
    //count the lines processed
    //int line_num = 0; //for debugging

    //getline reads a line from batch file, stores the line in a buffer
    //pointed to by line_buf and updates len w/ current allocated size of buffer
    while (getline(&line_buf, &len, in_fd) != -1) {
        if (strncmp(line_buf, "exit", 4) == 0)
            break; //exit psudo-shell if exit typed
        //prints command line (e.g., pwd ; mkdir test ; cd test) into output.txt.
        //write(1, line_buf, strlen(line_buf)); //for ex.output.txt
        trim(line_buf); //trim any white space away

    // parse the input line: fir
        l_tok_buf = str_filler(line_buf, ";");
        for (int i = 0; l_tok_buf.command_list[i] != NULL; i++){
            trim(l_tok_buf.command_list[i]); //trim white space
            s_tok_buf = str_filler(l_tok_buf.command_list[i], " ");
            //if the user types something
            execute_command(s_tok_buf);
            // s_tok_buf freed in execute_command
        }
        free_command_line(&l_tok_buf);
    }
    free(line_buf);
    fclose(in_fd);
}

void interactive_mode(void){
     // INTERACTIVE MODE
    /*
    i. If no command line argument is given the project will start in 
    interactive mode.
    ii. The program accepts input from the console and writes output 
    from the console.
    */
    //user types: ./pseudo-shell
    // Allocate a buffer for reading lines
    // allocate memory on the heap
    size_t len = 0;
    char *line_buf = NULL;
    // declare variables to store tokens
    /*
    Token Type   Split By	     Represents	             Example
    Large Token	  ;	    Full individual command	  "ls -l" or "cd folder"
    Small Token	 " "	  Command + each argument	 "cd", "folder"
    */
    command_line l_tok_buf;
    command_line s_tok_buf;
    //count the lines processed
    // int line_num = 0; //possible for debugging later

    // Interactive Loop
    while (1) {
        write(1, ">>> \n", 4);
        // printf(">>> "); //display prompt that shows up for user

    //'line_buf' is a pointer to a buffer (which getline() may allocate/reallocate).
    //'len' is the current size of the buffer.
    //'stdin' is the input stream (the keyboard, in interactive mode).
    //getline reads a full line from consol, stores the line in a buffer
    //pointed to by line_buf and updates len w/ curr allocated size of buffer
        if (getline(&line_buf, &len, stdin) == -1)
    //a -1 would appear only if there is an error or user typed an
    //end of input command (Ctrl+D)
            break; //will Exit psudo-shell program
        trim(line_buf); //trim any white space away
        //strncmp compares 1st 4 char of line_buf with "exit"
        //if match, exit shell
        if (strncmp(line_buf, "exit", 4) == 0)
            break; //exit psudo-shell if exit typed

    // parse the input line into sepparate command segments
    // each command segmenet sepparated by semicolen (;)
    //ex. ls -l; cd folder" is split into "ls -l" and " cd folder".
        l_tok_buf = str_filler(line_buf, ";");
        //now process each command segments obtained from splitting by ;
        for (int i = 0; l_tok_buf.command_list[i] != NULL; i++){
            trim(l_tok_buf.command_list[i]); //trim white space
            // further split command segment into tokens by spaces
            // the first token is command (ls)
            s_tok_buf = str_filler(l_tok_buf.command_list[i], " ");
        // now execute the command, execute_command determines which commnd
            execute_command(s_tok_buf);
        }
        free_command_line(&l_tok_buf);
    }
    free(line_buf);
}

int main(int argc, char const *argv[]){
    //redirect std output
    //freopen(str w/ name of file to open, mode, file stream to redirect)
    freopen("output.txt", "w", stdout);
    // start interactive mode immediately 
    if (argc == 1) {
        interactive_mode();
    }
    return 0;
}
