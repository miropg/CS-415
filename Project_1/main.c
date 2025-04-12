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
    if(s_tok_buf.command_list[0] != NULL) {
        // double check user wrote valid command before checking which command it is
        if (!valid_command(s_tok_buf.command_list[0])) {
            err_unrecognized(s_tok_buf.command_list[0]);
            free_command_line(&s_tok_buf);
            return;
        } 
        if (strcmp(s_tok_buf.command_list[0], "ls") == 0) {
            if (param_count_valid("ls", s_tok_buf.num_token))
                listDir();
            else
                err_params("ls");
        } else if (strcmp(s_tok_buf.command_list[0], "pwd") == 0) {
            if (param_count_valid("pwd", s_tok_buf.num_token))
                showCurrentDir();
            else
                err_params("pwd");
        } else if (strcmp(s_tok_buf.command_list[0], "mkdir") == 0) {
            if (param_count_valid("mkdir", s_tok_buf.num_token))
                makeDir(s_tok_buf.command_list[1]);
            else
                err_params("mkdir");
        } else if (strcmp(s_tok_buf.command_list[0], "cd") == 0) {
            if (param_count_valid("cd", s_tok_buf.num_token))
                changeDir(s_tok_buf.command_list[1]);
            else
                err_params("cd");
        } else if (strcmp(s_tok_buf.command_list[0], "cp") == 0) {
            if (param_count_valid("cp", s_tok_buf.num_token))
                copyFile(s_tok_buf.command_list[1], s_tok_buf.command_list[2]);
            else
                err_params("cp");
        } else if (strcmp(s_tok_buf.command_list[0], "mv") == 0) {
            if (param_count_valid("mv", s_tok_buf.num_token))
                moveFile(s_tok_buf.command_list[1], s_tok_buf.command_list[2]);
            else
                err_params("mv");
        } else if (strcmp(s_tok_buf.command_list[0], "rm") == 0) {
            if (param_count_valid("rm", s_tok_buf.num_token))
                deleteFile(s_tok_buf.command_list[1]);
            else
                err_params("rm");
        } else if (strcmp(s_tok_buf.command_list[0], "cat") == 0) {
            if (param_count_valid("cat", s_tok_buf.num_token))
                displayFile(s_tok_buf.command_list[1]);
            else
                err_params("cat");
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

        write(1, line_buf, strlen(line_buf));
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
    // IF NEITHER MODE / INCORRECT INPUT
    // TEST if wrong commands show the following instructions

    // Check for valid argument counts.
    // Interactive mode: no arguments (argc == 1)
    // File mode: exactly 3 arguments: executable, "-f", and filename (argc == 3)
    if (argc != 1 && (argc != 3 || strcmp(argv[1], "-f") != 0)) {
        write(2, "Usage:\n", strlen("Usage:\n"));

        // Write file mode usage message.
        write(2, "  For File mode: ", strlen("  For File mode: "));
        write(2, argv[0], strlen(argv[0]));
        write(2, " -f <filename>\n", strlen(" -f <filename>\n"));

        //write interactive mode message
        write(2, "  For Interactive mode: ", strlen("  For Interactive mode: "));
        write(2, argv[0], strlen(argv[0]));
        write(2, "\n", 1);

        //fprintf(stderr, "Usage:\n");
        //fprintf(stderr, "  For File mode: %s -f <filename>\n", argv[0]);
        //fprintf(stderr, "  For Interactive mode: %s\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if (argc == 3 && strcmp(argv[1], "-f") == 0) {
    // Run in file mode
    file_mode(argv[2]);
    } else if (argc == 1) {
        interactive_mode();
    }
    return 0;
}
/*
OLD FILE MODE
void file_mode(const char *filename){
 // FILE MODE
    
    i. File mode is called by using the -f flag and passing it a filename
    (e.g. pseudo-shell -f <filename>)
    ii. The shell only accepts filenames in file mode.
    iii. All input is taken from the file whose name is specified by <filename>
    and all output is written to the file “output.txt”. There should be no
    console output in this mode. Place all output in the file.
    
   FILE *inFPtr = fopen(filename, "r");
   FILE *outFPtr = fopen("output.txt", "w");

   if (!inFPtr) {
       perror("Error opening input file");
       return 1;
   }
   if (!outFPtr) {
       perror("Error opening output file");
       fclose(inFPtr);
       return 1;
   } 
   // Allocate a buffer for reading lines
   // allocate memory on the heap
   size_t len = 128;  
   // call helper function
   char *line_buf = allocate_buffer_w_files(len, inFPtr, outFPtr);

   // declare variables to store tokens
   command_line l_tok_buf;
   command_line s_tok_buf;
   //count the lines processed
   int line_num = 0;

   // Process file line-by-line
   while (getline(&line_buf, &len, inFPtr) != -1) {
       fprintf(outFPtr, "Line %d:\n", ++line_num);
       l_tok_buf = str_filler(line_buf, ";");  // split line into commands
       for (int i = 0; l_tok_buf.command_list[i] != NULL; i++) {
           fprintf(outFPtr, "\tLine segment %d:\n", i + 1);
           s_tok_buf = str_filler(l_tok_buf.command_list[i], " "); // further split command into tokens
           for (int j = 0; s_tok_buf.command_list[j] != NULL; j++) {
               fprintf(outFPtr, "\t\tToken %d: %s\n", j + 1, s_tok_buf.command_list[j]);
           }
           free_command_line(&s_tok_buf);
       }
       free_command_line(&l_tok_buf);
   }
   
   // Clean up
   fclose(inFPtr);
   fclose(outFPtr);
   free(line_buf);
   return 0;
}
*/
