#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h> //for O_WRONLY | O_CREAT ...
#include "string_parser.h"
#include "command.h"

#define _GNU_SOURCE
#define MAX_PATH_LEN 1024
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
int compare(const void *a, const void *b) {
    char * const *pa = a;
    char * const *pb = b;
    return strcmp(*pa, *pb);
}
// SIDE NOTE
// Return vs. COntinue
// continue when a single file fails.
// return when a critical system call like getcwd() or opendir() fails.
void lfcat() {
/* High level functionality you need to implement: */
/* Get the current directory with getcwd() */
/* Open the dir using opendir() */
/* use a while loop to read the dir with readdir()*/
/* You can debug by printing out the filenames here */
/* Option: use an if statement to skip any names that are not readable
files (e.g. ".", "..", "main.c", "lab2.exe", "output.txt" */
/* Open the file */
/* Read in each line using getline() */
/* Write the line to stdout */
/* write 80 "-" characters to stdout */
/* close the read file and free/null assign your line buffer */
/*close the directory you were reading from using closedir() */

    //program will behave like a cat-all-files-in-this-folder tool
    // 1: getcwd()	Get the name of the current directory
    char *cwd = malloc(sizeof(char) * MAX_PATH_LEN);
    if (cwd == NULL) {
        const char *err = "Memory allocation failed for current directory.\n";
        write(2, err, strlen(err));
        return;
    }
    if (getcwd(cwd, MAX_PATH_LEN) == NULL) {
        const char *err = "Failed to get current diretory.\n";
        write(2, err, strlen(err));
        free(cwd);
        return;
        }
    // 2: opendir()	Open that directory
    DIR *dir = opendir("files");
    if (!dir) {
        const char *err = "Failed to open directory.\n";
        write(2, err, strlen(err));
        free(cwd);
        return;
    }
    // 3: readdir() in a loop	Go through every file in the directory
    /*The readdir() function returns a pointer to a dirent structure
        representing the next directory entry in the directory stream
        pointed to by dirp.  It returns NULL on reaching the end of the
        directory stream or if an error occurred.
    //essentially reads one item from the directory at a time
    */
    //sets up ptr entry to store info about each file or directory
    //that we find in the current folder
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL){
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        } 
        // Build full path to file: "files/filename"
        char *filepath = malloc(sizeof(char) * MAX_PATH_LEN);
        if (!filepath) {
            const char *err = "Memory allocation failed.\n";
            write(2, err, strlen(err));
            continue;
        }
        //snprintf assembles string to look like filepath
        /*takes:
        "files/%s" → a string format
        entry->d_name → like "1_poem.txt"
        Produces:
        filepath = "files/1_poem.txt";*/
        snprintf(filepath, MAX_PATH_LEN, "files/%s", entry->d_name);
        FILE *fp = fopen(filepath, "r");
        if (!fp) {
            const char *err = "Failed to oepn file for reading\n";
            write(2, err, strlen(err));
            free(filepath); //we must free if the if statement activates
            continue;
        }
        char *line_buf = NULL;
        size_t len = 0;
        // 4: getline() Read each file line-by-line
        //getline reads a line from batch file, stores the line in a buffer
        //pointed to by line_buf and updates len w/ current allocated 
        //size of buffer
        while (getline(&line_buf, &len, fp) != -1) {
            // 5: write file contents to terminal, like cat
            write(1, line_buf, strlen(line_buf));
        }
        // 6: Print 80 "-" characters	To separate each file's contents
        for (int i = 0; i < 80; i++) {
            write(1, "-", 1);
        }
        write(1, "\n", 1);

        free(line_buf);
        free(filepath);
        fclose(fp); //must use fclose because we used fopen
    }
    closedir(dir);
    free(cwd);
}

void listDir() {
    DIR *dir = opendir(".");
    if (!dir) {
        const char *err = "Failed to open directory.\n";
        write(2, err, strlen(err));
        return;
    }

    struct dirent *entry;
    char **entries = NULL;
    int count = 0;
    size_t capacity = 10;

    entries = malloc(capacity * sizeof(char *));
    if (!entries) {
        const char *err = "Memory allocation failed.\n";
        write(2, err, strlen(err));
        closedir(dir);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (count >= capacity) {
            capacity *= 2;
            char **temp = realloc(entries, capacity * sizeof(char *));
            if (!temp) {
                const char *err = "Reallocation failed.\n";
                write(2, err, strlen(err));
                closedir(dir);
                for (int i = 0; i < count; ++i) free(entries[i]);
                free(entries);
                return;
            }
            entries = temp;
        }

        entries[count] = malloc(strlen(entry->d_name) + 1);
        if (!entries[count]) {
            const char *err = "Memory allocation failed.\n";
            write(2, err, strlen(err));
            closedir(dir);
            for (int i = 0; i < count; ++i) free(entries[i]);
            free(entries);
            return;
        }
        strcpy(entries[count], entry->d_name);
        count++;
    }

    closedir(dir);

    qsort(entries, count, sizeof(char *), compare);

    for (int i = 0; i < count; ++i) {
        write(1, entries[i], strlen(entries[i]));
        write(1, "\n", 1);
        free(entries[i]);
    }
    free(entries);
}

void showCurrentDir(){ /*for the pwd command*/
    //cwd = current working directory
    char *cwd = malloc(sizeof(char) * MAX_PATH_LEN);
    if (cwd == NULL) {
        const char *err = "Memory allocation failed for current directory.\n";
        write(2, err, strlen(err));
        return;
    }
    //getcwd: get current working directory
    if (getcwd(cwd, MAX_PATH_LEN) != NULL) {
        //write the full path to std output
        write(1, cwd, strlen(cwd));
        //write new line after the path
        write(1, "\n", 1);
    } else {
        //prins to std error
        const char *err = "Failed to get current directory.\n";
        write(2, err, strlen(err));
    }
    free(cwd);
    // const char *msg = "DEBUG: Executing pwd command\n";
    // write(1, msg, strlen(msg));
}

void makeDir(char *dirName){ /*for the mkdir command*/
    if (mkdir(dirName, 0755) == -1) {
        if (errno == EEXIST) {
            const char *msg = "Directory already exists!\n";
            write(1, msg, strlen(msg));
        } else {
            const char *err = "Failed to create directory.\n";
            write(2, err, strlen(err));
        }
    }      
//  const char *msg = "DEBUG: Executing mkdir command\n";
//  write(1, msg, strlen(msg));
}

void changeDir(char *dirName){ /*for the cd command*/
    if (chdir(dirName) == -1) {
        const char *err = "Failed to change directory.\n";
        write(2, err, strlen(err));
    }
    // const char *msg = "DEBUG: Executing cd command\n";
    // write(1, msg, strlen(msg));
}

//Instructions copyfile:
//check if destination path
//open file with source path
//otherwerise copy contents of destination file to source path

//sourcePath: file you want to copy
//destinationPath: either a new filename, a dif directory, or a combination
void copyFile(char *sourcePath, char *destinationPath){ /*for the cp command*/
    //open source file
    int srcFD = open(sourcePath, O_RDONLY);
    if (srcFD == -1) {
        const char *err = "Failed to open source file.\n";
        write(2, err, strlen(err));
        return;
    }
    //check if destinationPath is a directory
    struct stat destStat;
    int isDir = 0;
//stat(path, &statStruct) checks if a file or directory exists, and fills in info
    //destStat is a structure that stores that info.
    //if path exists it returns 0, otherwise -1
    //S_ISDIR(mode): checks if path refers to a directory
    //destStat.st_mode contains the "type" of file, regular file, dir...
    if (stat(destinationPath, &destStat) == 0 && S_ISDIR(destStat.st_mode)){
        isDir = 1;
    }
    //if it's a directory, append the filename to the path
    //allocate buffer to build the full destination path
    //previously: char finalDest[1024];
    char *finalDest = malloc(sizeof(char) * MAX_PATH_LEN);
    if (!finalDest) {
        const char *err = "Memory allocation failed for destination path.\n";
        write(2, err, strlen(err));
        close(srcFD);
        return;
    }
    if (isDir) {
    //strchr: returns ptr to 1st occurrence of the char c in string s
    // if / is found, we just want file name, so we take one chart past it
        char *fileName = strchr(sourcePath, '/');
        // if file name is not null, then set filename to pt 1 char after 
        // the slas, otherwise set filename to og sourcePath
        fileName = fileName ? fileName + 1 : sourcePath;  
        //ex. strcpy(dest, "hello"), dest now holds "hello"
        strcpy(finalDest, destinationPath); // start with the directory
        //concatenate
        strcat(finalDest, "/");             // add the slash
        strcat(finalDest, fileName);        // add the file name
    } else { //in case destination path not a directory
    // so we want to use dest path as-is, & safely cpy it to finaldest
    //strncpy(ptr to destination buffer, ptr to source str, max char to copy)
        // -1 for the nullptr
        strncpy(finalDest, destinationPath, MAX_PATH_LEN - 1);
        // after copying manually add null ptr
        finalDest[MAX_PATH_LEN - 1] = '\0';
    }
    // open dest file for writing
    //O_WRONLY  = open file for writing
    // O_CREAT  = create the file if it doesn't exist
    // O_TRUNC = truncate the file to 0 length if it already exists
    // 0644 = file permission
    int destFD = open(finalDest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (destFD == -1) {
        const char *err = "Failed to open destination file.\n";
        write(2, err, strlen(err));
        close(srcFD);
        return;
    }
    // previously: char buffer[1024];
    char *buffer = malloc(sizeof(char) * MAX_PATH_LEN);
    if (!finalDest) {
        const char *err = "Memory allocation failed for destination path.\n";
        write(2, err, strlen(err));
        close(srcFD);
        return;
    }
    //ssize_t: signed size type, the type that read and write return
    ssize_t bytesRead, bytesWritten;
    // while there are still bytes being read from the source file into buf
    while ((bytesRead = read(srcFD, buffer, MAX_PATH_LEN)) > 0) {
        //read and write return how many bytes were read/written
        //so they can be assigned to 
        bytesWritten = write(destFD, buffer, bytesRead);
        //bytesRead/bytesWritten variables created just for error below 
        if (bytesWritten != bytesRead) {
            const char *err = "Error writing to destination file.\n";
            write(2, err, strlen(err));
            break;
        }
    }
    close(srcFD);
    close(destFD);
    free(buffer);
    free(finalDest);
    // const char *msg = "DEBUG: Executing cp command\n";
    // write(1, msg, strlen(msg));
}

 // copy and delete basically
void moveFile(char *sourcePath, char *destinationPath){ /*for the mv command*/
    // const char *msg = "DEBUG: Executing mv command\n";
    // write(1, msg, strlen(msg));
    copyFile(sourcePath, destinationPath);
    deleteFile(sourcePath);
}

void deleteFile(char *filename){ /*for the rm command*/
    /*unlink() deletes a name from the filesystem.  If that name was the
       last link to a file and no processes have the file open, the file
       is deleted and the space it was using is made available for reuse.
       */
    if (unlink(filename) == -1){
        const char *err = "Error deleting file.\n";
        write(2, err, strlen(err));
    }
    // const char *msg = "DEBUG: Executing rm command\n";
    // write(1, msg, strlen(msg));
}

void displayFile(char *filename){ /*for the cat command*/
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        const char *err = "failier opening input file\n";
        write(2, err, strlen(err)); //does this need write?
        return;
    }
    char *buffer = malloc(MAX_PATH_LEN);
    if (!buffer) {
        const char *err = "memory allocation failed\n";
        write(2, err, strlen(err));
        close(fd);
        return;
    }
    ssize_t bytesRead;
    while ((bytesRead = read(fd, buffer, MAX_PATH_LEN)) > 0) {
        write(1, buffer, bytesRead);
    }
    free(buffer);
    close(fd);
    // const char *msg = "DEBUG: Executing cat command\n";
    // write(1, msg, strlen(msg));
}