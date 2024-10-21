/*
    Programming Assignment 2: Aggie Shell

    ==================================================
    Original assignment code written and provided by:
        Professor Kumar
        Department of Computer Science and Engineering
        Texas A&M University
        Date: 10/10/2024
    ==================================================

    ==================================================
    Pseudo-code and commented edits authored by:
        Kyle Lang
        Department of Computer Science and Engineering
        Texas A&M University
        Date: 10/10/2024
    ==================================================
*/


#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <termios.h>


#include <vector>
#include <string>
#include <ctime>

#include "Tokenizer.h"


// all the basic colours for a shell prompt
#define RED     "\033[1;31m"
#define GREEN	"\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define WHITE	"\033[1;37m"
#define NC      "\033[0m"
#define PATH_MAX 500


using namespace std;


/**
 * Return Shell prompt to display before every command as a string
 * 
 * Prompt format is: `{MMM DD hh:mm:ss} {user}:{current_working_directory}$`
 */
void print_prompt() {
    // Use time() to obtain the current system time, and localtime() to parse the raw time data
    time();

    // Parse date time string from raw time data using strftime()
    // Date-Time format will be in the format (MMM DD hh:mm:ss), with a constant size of 15 characters requiring 16 bytes to output
    strftime();

    // Obtain current user and current working directory by calling getenv() and obtaining the "USER" and "PWD" environment variables
    getenv();

    // Can use colored #define's to change output color by inserting into output stream
    // MODIFY to output a prompt format that has at least the USER and CURRENT WORKING DIRECTORY
    std::cout << YELLOW << "shell" << NC << " ";
}


/**
 * Process shell command line
 * 
 * Each command line from the shell is parsed by '|' pipe symbols, so command line must be iterated to execute each command
 * Example: `ls -l / | grep etc`    ::  This command will list (in detailed list form `-l`) the root directory and then pipe into a filter for `etc`
 * When parsed into the Tokenizer, this will split into two separate commands, `ls -l /` and `grep etc`.
 */
void process_commands() {
    // Declare file descriptor variables for storing unnamed pipe fd's
    // Maintain both a FORWARD and BACKWARDS pipe in the parent for command redirection
    backwards_fds;
    forwards_fds;

    // LOOP THROUGH COMMANDS FROM SHELL INPUT
    for (command in tknr) {
        // Check if the command is 'cd' first
        // This will not have any pipe logic and needs to be done manually since 'cd' is not an executable command
        // Use getenv(), setenv(), and chdir() here to implement this command
        if (command == "cd") {
            // There are two tested inputs for 'cd':
            // (1) User provides "cd -", which directs to the previous directory that was opened
            // (2) User provides a directory to open

            continue;
        }
        
        // Initialize backwards pipe to forward pipe
        backwards_fds = forwards_fds;
        // If any other command, set up forward pipe IF the current command is not the last command to be executed
        if (command is NOT last command) {
            pipe();
        }

        // fork to create child
        pid_t pid = fork();
        if (pid < 0) {  // error check
            perror("fork error");
            exit(2);
        }
        if (pid == 0) {  // if child, exec to run command
            if (command is NOT last command) {  // i.e. {current command} | {next command} ...
                dup2();     // Reidrect STDOUT to forward pipe
                close();    // Close respective pipe end
            }

            if (command NOT first command) {    // i.e. {first command} | {current command} ...
                dup2();     // Redirect STDIN to backward pipe
                close();    // Close respective pipe end
            }
            
            if (command has INPUT) {    // i.e. {command} < {input file}
                open(); // Open input file
                dup2(); // Redirect STDIN from file
            }

            if (command has OUTPUT) {   // i.e. {command} > {output file}
                open(); // Open output file
                dup2(); // Redirect STDOUT to file
            }

            // Execute command after any redirection
            char* args[] = {(char*) tknr.commands.at(0)->args.at(0).c_str(), nullptr};
            if (execvp(args[0], args) < 0) {  // error check
                perror("execvp");
                exit(2);
            }
        }
        else {  // if parent, wait for child to finish    
            // Close pipes from parent so pipes receive `EOF`
            // Pipes will otherwise get stuck indefinitely waiting for parent input/output that will never occur
            close();

            // If command is indicated as a background process, set up to ignore child signal to prevent zombie processes
            if (cmd is BACKGROUND) {
                signal();
            }
            else {
                int status = 0;
                waitpid(pid, &status, 0);
                
                if (status > 1) {  // exit if child didn't exec properly
                    exit(status);
                }
            }
        }
    }
}


int main () {
    // Use setenv() and getenv() to set an environment variable for the previous PWD from 'PWD'
    getenv();
    setenv();

    
    // This is your process loop; Will run infinitely until given defined break case
    for (;;) {
        // need to print date/time, username, and absolute path to current dir into 'shell'
        // Might want to use a helper function like this here for code clarity
        print_prompt();
                
        // get user inputted command
        string input;
        getline(cin, input);
        
        if (input == "exit") {  // print exit message and break out of infinite loop
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }
        // Might want to catch empty input; Tokenizer will error and not recover on empty input

        // get tokenized commands from user input
        // This will parse your user input for you; Please refer the header files to understand how to use this class
        Tokenizer tknr(input);
        if (tknr.hasError()) {  // continue to next prompt if input had an error
            continue;
        }


        /*
        // print out every command token-by-token on individual lines
        // prints to cerr to avoid influencing autograder
        for (auto cmd : tknr.commands) {
            for (auto str : cmd->args) {
                cerr << "|" << str << "| ";
            }

            if (cmd->hasInput()) {
                cerr << "in< " << cmd->in_file << " ";
            }

            if (cmd->hasOutput()) {
                cerr << "out> " << cmd->out_file << " ";
            }
            cerr << endl;
        }
        */
       
        // Might want to use a helper function like this for processing the shell's command line 
        process_commands();
    }
}
