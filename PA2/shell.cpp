#include <Command.h>
#include <Tokenizer.h>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

// All the basic colors for a shell prompt
#define RED    "\033[1;31m"
#define GREEN  "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE   "\033[1;34m"
#define WHITE  "\033[1;37m"
#define NC     "\033[0m"

// Signal handler for SIGCHLD
void sigchld_handler(int child) {
    (void) child;
    wait(0);
    
//     while (waitpid(-1, nullptr, WNOHANG) > 0);
//     errno = saved_errno;
}

/**
 * Returns the shell prompt to display before every command as a string.
 *
 * Prompt format is: {MMM DD hh:mm:ss} {user}:{current_working_directory}$
 */
void print_prompt() {
    time_t timer;
    time(&timer);
    struct tm *localTime = localtime(&timer);

    // Date-Time format (MMM DD hh:mm:ss), with a buffer of 32 bytes to be safe
    char buf[32];
    strftime(buf, sizeof(buf), "%b %d %H:%M:%S", localTime);

    // Obtain current user
    char *user = getenv("USER");

    // Get current working directory
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        exit(1);
    }

    // Output the shell prompt with colors
    std::cout << BLUE << buf << " " << user << ":" << YELLOW << cwd << NC << "$ ";
}

/**
 * Processes shell command line
 */
void process_commands(Tokenizer &tokens) {
    int fdsBack[2] = {-1, -1}; // File descriptors for the previous command

    for (unsigned int i = 0; i < tokens.commands.size(); i++) {
        Command *command = tokens.commands[i];
        std::vector<std::string>& args = command->args;

        // Handle `cd` command separately in the parent process
        if (args[0] == "cd") {
            if (args.size() > 2) {
                std::cerr << "cd: too many arguments" << std::endl;
                continue;
            }

            const char *new_dir = nullptr;
            if (args.size() == 1) {
                new_dir = getenv("HOME"); // Go to home directory
            } else if (args[1] == "-") {
                new_dir = getenv("OLDPWD"); // Go to previous directory
                if (!new_dir) {
                    std::cerr << "cd: OLDPWD not set" << std::endl;
                    continue;
                }
            } else {
                new_dir = args[1].c_str(); // Go to specified directory
            }

            // Before changing directory, save current PWD
            char *prevPWD = getenv("PWD");
            if (prevPWD != nullptr) {
                setenv("OLDPWD", prevPWD, 1);
            }

            // Attempt to change directory
            if (chdir(new_dir) == 0) {
                // Update PWD
                char cwd[PATH_MAX];
                if (getcwd(cwd, sizeof(cwd)) != NULL) {
                    setenv("PWD", cwd, 1);
                } else {
                    perror("getcwd");
                }
            } else {
                perror("chdir");
            }
            continue;
        }

        int fdsFor[2] = {-1, -1}; // File descriptors for the next command

        // If not the last command, set up a pipe for the next command
        if (i < tokens.commands.size() - 1) {
            if (pipe(fdsFor) < 0) {
                perror("pipe error");
                exit(1);
            }
        }

        // Fork to create a child process
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork error");
            exit(2);
        }

        if (pid == 0) { // In the child process
            // Reset signal handlers to default
            struct sigaction sa_default;
            sa_default.sa_handler = SIG_DFL;
            sigemptyset(&sa_default.sa_mask);
            sa_default.sa_flags = 0;

            sigaction(SIGCHLD, &sa_default, nullptr);
            sigaction(SIGINT, &sa_default, nullptr);
            sigaction(SIGQUIT, &sa_default, nullptr);

            // Reset signal mask
            sigset_t set;
            sigemptyset(&set);
            if (sigprocmask(SIG_SETMASK, &set, nullptr) == -1) {
                perror("sigprocmask");
                exit(1);
            }

            // Input redirection or pipe setup
            if (command->hasInput()) {
                int inputFd = open(command->in_file.c_str(), O_RDONLY);
                if (inputFd < 0) {
                    perror("open input file");
                    exit(2);
                }
                dup2(inputFd, STDIN_FILENO);
                close(inputFd);
            } else if (fdsBack[0] != -1) {
                dup2(fdsBack[0], STDIN_FILENO);
            }

            // Output redirection or pipe setup
            if (command->hasOutput()) {
                int outputFd = open(command->out_file.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
                if (outputFd < 0) {
                    perror("open output file");
                    exit(2);
                }
                dup2(outputFd, STDOUT_FILENO);
                close(outputFd);
            } else if (fdsFor[1] != -1) {
                dup2(fdsFor[1], STDOUT_FILENO);
            }

            // Close all pipe file descriptors in the child process
            if (fdsBack[0] != -1) close(fdsBack[0]);
            if (fdsBack[1] != -1) close(fdsBack[1]);
            if (fdsFor[0] != -1) close(fdsFor[0]);
            if (fdsFor[1] != -1) close(fdsFor[1]);

            // Build arguments for execvp
            std::vector<char*> execArgs;
            for (auto &arg : command->args) {
                execArgs.push_back(const_cast<char*>(arg.c_str()));
            }

            // Disable color when running the `ls` command
            if (execArgs[0] == std::string("ls")) {
                execArgs.push_back(const_cast<char*>("--color=never"));
            }

            execArgs.push_back(nullptr); // Null-terminate the argument list

            if (execvp(execArgs[0], execArgs.data()) < 0) {
                std::cerr << "Command not found: " << execArgs[0] << std::endl;
                perror("execvp");
                exit(1);
            }
        } else { // In the parent process
            // Parent doesn't need the write end of the previous pipe
            if (fdsBack[1] != -1) {
                close(fdsBack[1]);
            }

            // Parent doesn't need the write end of the current pipe
            if (fdsFor[1] != -1) {
                close(fdsFor[1]);
            }

            // Only wait for foreground processes
            if (!command->isBackground()) {
                int status = 0;
                waitpid(pid, &status, 0);
                if (WIFEXITED(status)) {
                    int exit_status = WEXITSTATUS(status);
                    if (exit_status != 0) {
                        std::cerr << "Command exited with status: " << exit_status << std::endl;
                    }
                } else if (WIFSIGNALED(status)) {
                    int term_sig = WTERMSIG(status);
                    std::cerr << "Command terminated by signal: " << term_sig << std::endl;
                }
            } else {
                // Background process; do not wait here
                // The SIGCHLD handler will reap the process
            }

            // Close the read end of the previous pipe after it's no longer needed
            if (fdsBack[0] != -1) {
                close(fdsBack[0]);
            }

            // Prepare for the next command
            fdsBack[0] = fdsFor[0];
            fdsBack[1] = -1; // fdsFor[1] has been closed in the parent
        }
    }

    // After the loop, close any remaining file descriptors
    if (fdsBack[0] != -1) close(fdsBack[0]);
    if (fdsBack[1] != -1) close(fdsBack[1]);
}

int main() {
    // Set up the SIGCHLD handler
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
        perror("sigaction");
        exit(1);
    }

    for (;;) {
        print_prompt();

        std::string input;
        if (!std::getline(std::cin, input)) {
            // Handle EOF (Ctrl+D)
            std::cout << std::endl;
            break;
        }

        if (input.empty()) { continue; }

        if (input == "exit") {
            std::cout << RED << "Now exiting shell..." << std::endl << "Goodbye" << NC << std::endl;
            break;
        }

        Tokenizer tknr(input);
        if (tknr.hasError()) {
            continue;
        }

        process_commands(tknr);

        // Fallback reaping of zombie processes
        int status;
        while (waitpid(-1, &status, WNOHANG) > 0) {
            // Optionally handle exit status
        }
    }

    return 0;
}
