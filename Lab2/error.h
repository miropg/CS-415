// header file for errors
#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

// Prints an error for an unrecognized command
void err_unrecognized(const char *cmd);

// Prints an error for incorrect parameter count
void err_params(const char *cmd);

// Checks if the command is one of the known valid commands
int valid_command(const char *cmd);

// Checks if the command has the correct number of parameters
int param_count_valid(const char *cmd, int arg_count);

#endif
