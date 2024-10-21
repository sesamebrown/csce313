#include <Command.h>
#include <Tokenizer.h>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

// all the basic colours for a shell prompt
#define RED    "\033[1;31m"
#define GREEN  "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE   "\033[1;34m"
#define WHITE  "\033[1;37m"
#define NC     "\033[0m"

/**
 * Return Shell prompt to display before every command as a string
 * 
 * Prompt format is: `{MMM DD hh:mm:ss} {user}:{current_working_directory}$`
 */
void print_prompt() {
        // Use time() to obtain the current system time, and localtime() to parse the raw time data
        time_t timer;
        time(&timer);
        struct tm *localTime = localtime(&timer);

        // Parse date time string from raw time data using strftime()
        // Date-Time format will be in the format (MMM DD hh:mm:ss), with a constant size of 15 characters requiring 16 bytes to output
        char buf[16];
        strftime(buf, sizeof(buf), "%b %d %H:%M:%S", localTime);

        // Obtain current user and current working directory by calling getenv() and obtaining the "USER" and "PWD" environment variables
        char *user = getenv("USER");
        char *dir = getenv("PWD");

        // Can use colored #define's to change output color by inserting into output stream
        // MODIFY to output a prompt format that has at least the USER and CURRENT WORKING DIRECTORY
        std::cout << BLUE << buf << " " << user << ":" << YELLOW << dir << NC << "$ ";
}

/**
 * Process shell command line
 * 
 * Each command line from the shell is parsed by '|' pipe symbols, so command line must be iterated to execute each command
 * Example: `ls -l / | grep etc`    ::  This command will list (in detailed list form `-l`) the root directory and then pipe into a filter for `etc`
 * When parsed into the Tokenizer, this will split into two separate commands, `ls -l /` and `grep etc`.
 */
void process_commands(Tokenizer &tokens) {
        // Declare file descriptor variables for storing unnamed pipe fd's
        // Maintain both a FORWARD and BACKWARDS pipe in the parent for command redirection
        int fdsBack[2];
        int fdsFor[2];

        pipe(fdsFor);
        pipe(fdsBack);

        // LOOP THROUGH COMMANDS FROM SHELL INPUT
        // std::string shellInput;
        // std::getline(std::cin, shellInput);
        // Tokenizer tokens(shellInput);

        for (unsigned int i = 0; i < tokens.commands.size(); i++) {
        // Check if the command is 'cd' first
        // This will not have any pipe logic and needs to be done manually since 'cd' is not an executable command
        // Use getenv(), setenv(), and chdir() here to implement this command
                Command *command = tokens.commands[i];
                std::vector<std::string>& args = command->args;

                if (args[0] == "cd") {
                        // There are two tested inputs for 'cd':
                        // (1) User provides "cd -", which directs to the previous directory that was opened

                        if (args[1] == "-") {
                                char* oldPwd = getenv("OLDPWD");
                                if (oldPwd) {
                                        chdir(oldPwd);
                                }
                        }
                        // (2) User provides a directory to open
                        else {
                                chdir(args[1].c_str());
                        }
                        continue;
                }

                // Initialize backwards pipe to forward pipe
                if (i > 0) { // if not first command, set up backward pipe
                        dup2(fdsBack[0], STDIN_FILENO);
                        close(fdsBack[0]);
                }

                // If any other command, set up forward pipe IF the current command is not the last command to be executed
                if (i < tokens.commands.size() - 1) { // if not last command, set up forward pipe
                        pipe(fdsFor);
                        dup2(fdsFor[1], STDOUT_FILENO);
                        close(fdsFor[1]);
                }

                // fork to create child
                pid_t pid = fork();
                if (pid < 0) {  // error check
                        perror("fork error");
                        exit(2);
                }

                if (pid == 0) {  // if child, exec to run command
                        if (i < tokens.commands.size() - 1) {  // i.e. {current command} | {next command} ...
                                dup2(fdsFor[1], STDOUT_FILENO);     // Reidrect STDOUT to forward pipe
                                close(fdsFor[1]);    // Close respective pipe end
                                close(fdsFor[0]);
                        }

                        if (i > 0) {    // i.e. {first command} | {current command} ...
                                dup2(fdsBack[0], STDIN_FILENO);     // Redirect STDIN to backward pipe
                                close(fdsBack[0]);    // Close respective pipe end
                                close(fdsBack[1]);
                        }
                        
                        if (command->hasInput()) {    // i.e. {command} < {input file}
                                int inputFd = open(command->in_file.c_str(), O_RDONLY); // Open input file
                                if (inputFd < 0) {
                                        perror("open input file");
                                        exit(2);
                                }
                                dup2(inputFd, STDIN_FILENO); // Redirect STDIN from file
                                close(inputFd);
                        }

                        if (command->hasOutput()) {   // i.e. {command} > {output file}
                                int outputFd = open(command->out_file.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644); // Open output file
                                if (outputFd < 0) {
                                        perror("open output file");
                                        exit(2);
                                }
                                dup2(outputFd, STDOUT_FILENO); // Redirect STDOUT to file
                                close(outputFd);
                        }

                        if (fdsBack[0] != -1) close(fdsBack[0]);
                        if (fdsBack[1] != -1) close(fdsBack[1]);
                        if (fdsFor[0] != -1) close(fdsFor[0]);
                        if (fdsFor[1] != -1) close(fdsFor[1]);

                        // Execute command after any redirection
                        // char* args[] = {(char*) tokens.commands.at(0)->args.at(0).c_str(), nullptr};
                        std::vector<char*> execArgs;
                        for (auto &arg : command->args) {
                            execArgs.push_back((char*) arg.c_str());
                        }
                        execArgs.push_back(nullptr);

                        if (execvp(execArgs[0], execArgs.data()) < 0) {  // error check
                                perror("execvp");
                                exit(2);
                        }
                }
                else {  // if parent, wait for child to finish    
                        // Close pipes from parent so pipes receive `EOF`
                        // Pipes will otherwise get stuck indefinitely waiting for parent input/output that will never occur                        
                        if (i > 0) {
                                close(fdsBack[0]);
                                close(fdsBack[1]);
                        }

                        if (i < tokens.commands.size() - 1) {
                                close(fdsFor[0]);
                                close(fdsFor[1]);
                        }

                        // If command is indicated as a background process, set up to ignore child signal to prevent zombie processes
                        if (command->isBackground()) {
                                pid_t bg_pid = waitpid(pid, NULL, WNOHANG);
                                        if (bg_pid == -1) {
                                        perror("waitpid");
                                }
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

int main()
{
        // Use setenv() and getenv() to set an environment variable for the previous PWD from 'PWD'
        char *prevPWD = getenv("PWD");
        if (prevPWD != nullptr) {
                setenv("OLDPWD", prevPWD, 1);
        }

        for(;;)
        {
                // need to print date/time, username, and absolute path to current dir into 'shell'
                print_prompt();

                // get user inputted command
                std::string input;
                getline(std::cin, input);

                if (input.empty()) { continue; }

                if (input == "exit")
                {
                        // print exit message and break out of infinite loop
                        std::cout << RED << "Now exiting shell..." << std::endl << "Goodbye" << NC << std::endl;
                        break;
                }

                // get tokenized commands from user input
                Tokenizer tknr(input);
                if(tknr.hasError())
                {
                        // continue to next prompt if input had an error
                        continue;
                }
                // print out every command token-by-token on individual lines
                // prints to cerr to avoid influencing autograder
                for (auto cmd : tknr.commands) {
                    for (auto str : cmd->args) {
                        std::cerr << "|" << str << "| ";
                    }
                    if (cmd->hasInput()) {
                        std::cerr << "in< " << cmd->in_file << " ";
                    }
                    if (cmd->hasOutput()) {
                        std::cerr << "out> " << cmd->out_file << " ";
                    }
                    std::cerr << std::endl;
                }

                process_commands(tknr);

                // // fork to create child
                // pid_t pid = fork();
                // if(pid < 0)
                // { // error check
                //         perror("fork");
                //         exit(2);
                // }

                // if(pid == 0)
                // { // if child, exec to run command
                //         // run single commands with no arguments
                //         char* args[] = {(char*)tknr.commands.at(0)->args.at(0).c_str(), nullptr};

                //         if(execvp(args[0], args) < 0)
                //         { // error check
                //                 perror("execvp");
                //                 exit(2);
                //         }
                // }
                // else
                // { // if parent, wait for child to finish
                //         int status = 0;
                //         waitpid(pid, &status, 0);
                //         if(status > 1)
                //         { // exit if child didn't exec properly
                //                 exit(status);
                //         }
                // }
        }
}