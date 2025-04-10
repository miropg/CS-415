#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "string_parser.h"
#include "command.h"

/*
* Description: Project 1 C file.
* Author: Miro Garcia
*
* Date: 04/07/25
*/

/*
ssize_t write
(File descriptor (where to write data), ptr to buf, num bytes to write);
FD	            Meaning
0	    stdin (standard input)
1	    stdout (standard output)
2	    stderr (standard error)
*/

void listDir(){ /*for the ls command*/
    /*
    // opens currend directory, because "." refers to current directory
    DIR *dir = opendir(".");
    if (dir == NULL) {
        const char *err = "Failed to open directory.\n";
        //prints to standard error
        write(2, err, strlen(err));
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".")  == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }
        //both prints to std output 
        write(1, entry->d_name, strlen(entry->d_name));
        write(1, "\n", 1);
    }
    closedir(dir);
    */
    const char *msg = "DEBUG: Executing ls command\n";
    write(1, msg, strlen(msg));
}

void showCurrentDir(){ /*for the pwd command*/
    /*
    char cwd[1024];
    //getcwd: get current working directory
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        //write the full path to std output
        write(1, cwd, strlen(cwd));
        //write new line after the path
        write(1, "\n", 1);
    } else {
        //prins to std error
        const char *err = "Failed to get current directory.\n";
        write(2, err, strlen(err));
    }
        */
    const char *msg = "DEBUG: Executing pwd command\n";
    write(1, msg, strlen(msg));
}

void makeDir(char *dirName){ /*for the mkdir command*/
    /*
    if (mkdir(dirName, 0755) == -1) {
        const char *err = "Failed to create directory.\n";
        write(2, err, strlen(err));
    }
        */
       const char *msg = "DEBUG: Executing mkdir command\n";
       write(1, msg, strlen(msg));
}

void changeDir(char *dirName){ /*for the cd command*/
    /*
    if (chdir(dirName) == -1) {
        const char *err = "Failed to chnage directory.\n";
        write(2, err, strlen(err));
    }
        */
       const char *msg = "DEBUG: Executing cd command\n";
    write(1, msg, strlen(msg));
}
//sourcePath: file you want to copy
//destinationPath: either a new filename, a dif directory, or a combination
void copyFile(char *sourcePath, char *destinationPath){ /*for the cp command*/
    /*
    char buffer[1024];
    ssize_t bytesRead, bytesWritten;

    int srcFD = open(sourcePath, O_RDONLY)
    DIR *dir = open(sourcePath);
    char cwd[1024];
    write(1, cwd, strlen(cwd));
    DIR *otherdir = open(destinationPath);
    */
   const char *msg = "DEBUG: Executing cp command\n";
    write(1, msg, strlen(msg));
    //check if destination path
    //open file with source path
    //otherwerise copy contents of destination file to source path
}
void moveFile(char *sourcePath, char *destinationPath){ /*for the mv command*/
    const char *msg = "DEBUG: Executing mv command\n";
    write(1, msg, strlen(msg));
    // copy and delete basically
}
void deleteFile(char *filename){ /*for the rm command*/
    const char *msg = "DEBUG: Executing rm command\n";
    write(1, msg, strlen(msg));
}
void displayFile(char *filename){ /*for the cat command*/
    const char *msg = "DEBUG: Executing cat command\n";
    write(1, msg, strlen(msg));
}