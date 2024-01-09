#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>

#define DELIMITER " \t\n"
pid_t child_pid;

struct JobInfo 
{
    int job_id;
    pid_t pid;
    char command_line[1024];
    char status[12];
    int state; // -1: unintialized, 0: stopped, 1: fg, 2:bg
};

struct JobInfo jobArray[10];

// helper functions
void perform_redirection(char *args[], int num_args, char cmd_copy[]);
void ctrl_c_handler(int signum);
void sigchld_handler(int signum);
void ctrl_z_handler(int signum);
void cd(char *path, int max_size);
void move_to_background(pid_t pid);
void move_to_foreground(pid_t pid);
void quit();
void pwd();
void run_foreground_command(char *args[], int max_size, char cmd[]);
void run_background_command(char *args[], int max_size, char cmd[]);
void jobs();
void fg_by_pid(pid_t pid_target);
void fg_by_job_id(int job_id_target);
void bg_by_pid(pid_t pid_target);
void bg_by_job_id(int job_id_target);
void kill_by_job_id(int job_id_target);
void kill_by_pid(pid_t pid);

void perform_redirection(char *args[], int num_args, char cmd_copy[]) 
{
    //num args = max index, args -> args[0] = hello args[1] = world ... args[num args], cmd copy[] = cmd[1024] = copied command
    char *input_file = NULL;
    char *output_file = NULL;
    char *append_file = NULL;

    int input_fd, output_fd;
    int arg_num = num_args + 1; // add one to num_args to to for loop
    char copied_command[1024];
    char *token;
    char *copied_args [100];
    char *redirectino_command;
    for (int i = 0; i < arg_num; i++) 
    {
        if (strcmp(args[i], ">") == 0) 
        {
            if (i + 1 < arg_num) 
            {
                output_file = args[i + 1]; // next of ">" is output file
            }
        } 
        else if (strcmp(args[i], "<") == 0) 
        {
            if (i + 1 < arg_num) 
            {
                input_file = args[i + 1]; // next of "<" is input file
            }
        } 
        else if (strcmp(args[i], ">>") == 0) 
        {
            if (i + 1 < arg_num) 
            {
                append_file = args[i + 1]; // next of ">>" append (type of output) file
            }
        }
    } // get input, output, append file name, and also get the position of &.

    for(int i = 0; i < 1024; i ++)
    {
        copied_command[i] = '\0';
    }

    int test = 0;
    // copy to array before &, after that, tokenize.
    for(int i = 0; i < 1024; i++)
    {
        if(cmd_copy[i] == '&' || cmd_copy[i] == '<' || cmd_copy[i] == '>' || cmd_copy[i] == '\n')
        {
            break;
        }
        else
        {
            copied_command[i] = cmd_copy[i];
            test ++;
        }
    } // until here, copid_command has command line before '&'. now tokenize it.
    
    token = strtok(copied_command, DELIMITER);

    int index = 0;
    while (token != NULL) 
    {
        copied_args[index] = token;
        index++;
        token = strtok(NULL, DELIMITER);
        // printf ("token = %s\n", token);

    } // now copied_command has command args wihout &.

    copied_args[index] = NULL;

    //now set the file's mode and dup2
    mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO; 
    if (input_file != NULL) 
    {
        input_fd = open(input_file, O_RDONLY, mode);
        if (input_fd == -1) 
        {
            perror("open input file");
            exit(EXIT_FAILURE);
        }
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
    }

    if (output_file != NULL) 
    {
        output_fd = open(output_file, O_CREAT | O_WRONLY | O_TRUNC, mode);
        if (output_fd == -1) 
        {
            perror("open output file");
            exit(EXIT_FAILURE);
        }
        dup2(output_fd, STDOUT_FILENO);
        close(output_fd);
    }

    if (append_file != NULL) 
    {
        output_fd = open(append_file, O_CREAT | O_APPEND | O_WRONLY, mode);
        if (output_fd == -1) 
        {
            perror("open append file");
            exit(EXIT_FAILURE);
        }
        dup2(output_fd, STDOUT_FILENO);
        close(output_fd);
    }
    
    if ((execvp(copied_args[0], copied_args) < 0))
    {
        if (execv(copied_args[0], copied_args) < 0)
        {
            exit(0);
        }
    } 
    
    // close file 
    close(input_fd);
    close(output_fd);
}


void ctrl_c_handler(int signum) 
{
    int status;
    if (child_pid > 0) 
    {
        for(int i = 0; i < 10; i++)
        {
            if(jobArray[i].pid == child_pid) 
            {
                if (jobArray[i].state != 1) 
                {
                    // Exit if not a foreground process
                    return;
                } else
                { // kill if state is foreground..
                    // printf("Killing .. state: %d\n", jobArray[i].state);
                    // fflush(stdout);
                    kill(child_pid, SIGINT);

                    jobArray[i].job_id = i;
                    jobArray[i].pid = -1;
                    strcpy(jobArray[i].status, "NONE");
                    strcpy(jobArray[i].command_line, "\0");
                    break;
                }
            }
        }
    }
    // printf("return");
    // fflush(stdout);
}


void sigchld_handler(int signum) 
{
    int status;
    pid_t terminated_pid;

    // Reap all terminated child processes
    while ((terminated_pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        for(int i = 0; i < 10; i++)
        {
            if(jobArray[i].pid == terminated_pid)
            {
                jobArray[i].job_id = i;
                jobArray[i].pid = -1;
                strcpy(jobArray[i].status, "NONE");
                break;
            }
        }
    }
}

void ctrl_z_handler(int signum)
{
    // printf("ENTERED ctrl_z_handler\n");
    if (child_pid > 0) 
    {
        for(int i = 0; i < 10; i++)
        {
            if(jobArray[i].pid == child_pid) 
            {
                if (jobArray[i].state == 1) 
                {
                    strcpy(jobArray[i].status, "Stopped");
                    jobArray[i].state = 0;
                    // stop if state is foreground..
                    kill(child_pid, SIGTSTP);
                    break;
                } else
                { // exit b/c it's not foreground
                    return;
                }
            }
        }
    }
}

void cd(char *path, int max_size) 
{
    if (chdir(path) != 0) 
    {
        perror("Error");
    }
}

void move_to_background(pid_t pid) 
{
    for (int i = 0; i<10; i++) {
        if (jobArray[i].pid == pid) {
            jobArray[i].state = 2;
            strcpy(jobArray[i].status, "Running");
        }
    }
    if (setpgid(pid, pid) == -1)
    {
        perror("setpgid");
    }
    if (kill(pid, SIGCONT) == -1) 
    {
        perror("kill");
    }
}

void move_to_foreground(pid_t pid) 
{
    for(int i = 0; i < 10; i++)
    {
        if(jobArray[i].pid == pid)
        {
            strcpy(jobArray[i].status, "Running");
            jobArray[i].state = 1; // 1=fg
            break;
        }
    }
    kill(pid, SIGCONT);
    
    int status;
    while (waitpid(pid, &status, WUNTRACED) == -1) 
    {
        perror("waitpid");
    }

    if (WIFEXITED(status)) 
        {
            for(int i = 0; i < 10; i++)
            {
                if(jobArray[i].pid == child_pid)
                {
                    jobArray[i].job_id = i;
                    jobArray[i].pid = -1;
                    strcpy(jobArray[i].status, "NONE");
                    strcpy(jobArray[i].command_line, "\0");
                    jobArray[i].state = -1; // 
                    break;
                }
            }
        } 
        else if (WIFSTOPPED(status)) 
        {
            for(int i = 0; i < 10; i++)
            {
                if(jobArray[i].pid == child_pid)
                {
                    strcpy(jobArray[i].status, "Stopped");
                    jobArray[i].state = 0; // 0=stopped
                    break;
                }
            }
        }
}

void quit() 
{
    for (int i = 0; i < 10; i++) 
    {
        if (jobArray[i].pid != -1)
        {
            kill(jobArray[i].pid, SIGKILL); // Send SIGKILL signal to each child process to terminate immediately
        }
    }

    int status;
    pid_t wpid;
    do {
        wpid = waitpid(-1, &status, WNOHANG); // Reap any terminated child processes
        if (wpid > 0) {
        }
    } while (wpid > 0);

    exit(0);
}

void pwd() 
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) 
    {
        printf("%s\n", cwd);
    } 
    else 
    {
        perror("Error");
    }
}

void run_foreground_command(char *args[], int max_size, char cmd[]) 
{
    int status;
    child_pid = fork();
    
    if (child_pid == 0) 
    {
        perform_redirection(args, max_size, cmd);

        perror("execlp");
        exit(EXIT_FAILURE);
    } 
    else if (child_pid > 0) 
    {   
        for(int i = 0; i < 10; i++)
        {
            if(jobArray[i].pid == -1)
            {
                jobArray[i].job_id = i;
                jobArray[i].pid = child_pid;
                strcpy(jobArray[i].status, "Foreground");
                strcpy(jobArray[i].command_line, cmd);
                jobArray[i].state = 1; //1=fg
                break;
            }
        }

        if (waitpid(child_pid, &status, WUNTRACED) == -1)
        {
            perror("waitpid");
        }

        if (WIFEXITED(status)) 
        {
        // The child process terminated
            for(int i = 0; i < 10; i++)
            {
                if(jobArray[i].pid == child_pid)
                {
                    jobArray[i].job_id = i;
                    jobArray[i].pid = -1;
                    strcpy(jobArray[i].status, "NONE");
                    strcpy(jobArray[i].command_line, "\0");
                    jobArray[i].state = -1; //-1=dummy val
                    break;
                }
            }
        } 
        else if (WIFSTOPPED(status)) 
        {
        // The child process was stopped
            for(int i = 0; i < 10; i++)
            {
                if(jobArray[i].pid == child_pid)
                {
                    strcpy(jobArray[i].status, "Stopped");
                    jobArray[i].state = 0; //1=stopped
                    break;
                }
            }
        }
    } 
    else 
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
}

void run_background_command(char *args[], int max_size, char cmd[])
{
    // parent group
    //  bg process
    int status;
    child_pid = fork();
    if (child_pid < 0) 
    {
        perror("fork");
        exit(EXIT_FAILURE);
    } 
    else if (child_pid == 0) 
    {
        setpgid(0, 0); 
        perform_redirection(args, max_size, cmd);

        perror("execlp");
        exit(EXIT_FAILURE);
    } 
    else 
    {
        // do nothign just return main shell
        for(int i = 0; i < 10; i++)
        {
            if(jobArray[i].pid == -1)
            {
                jobArray[i].job_id = i;
                jobArray[i].pid = child_pid;
                strcpy(jobArray[i].status, "Running");
                strcpy(jobArray[i].command_line, cmd);
                jobArray[i].state = 2; //2=bg
                break;
            }
        }
    }
}

void jobs()
{
    for(int i = 0; i < 10; i++)
    {
        if(jobArray[i].pid != -1)
        {
            printf("[%d] (%d) %s %s", jobArray[i].job_id, jobArray[i].pid, jobArray[i].status, jobArray[i].command_line);
        }
    }
}

void fg_by_pid(pid_t pid_target)
{
    for(int i = 0; i < 10; i++)
    {
        if(jobArray[i].pid == pid_target)
        {
            strcpy(jobArray[i].status, "Foreground");
            jobArray[i].state = 1; //1=fg
            break;
        }
    }
    move_to_foreground(pid_target);
}

void fg_by_job_id(int job_id_target)
{
    pid_t pid = jobArray[job_id_target].pid;

    for(int i = 0; i < 10; i++)
    {
        if(jobArray[i].job_id == job_id_target)
        {
            strcpy(jobArray[i].status, "Foreground");
            jobArray[i].state = 1; //1=fg
            break;
        }
    }

    move_to_foreground(pid);
}

void bg_by_pid(pid_t pid_target)
{
    for(int i = 0; i < 10; i++)
    {
        if(jobArray[i].pid == pid_target)
        {
            strcpy(jobArray[i].status, "Running");
            jobArray[i].job_id = i;
            jobArray[i].state = 2; //2=bg
            break;
        }
    }
    move_to_background(pid_target);
}

void bg_by_job_id(int job_id_target)
{
    pid_t pid = jobArray[job_id_target].pid;
    for(int i = 0; i < 10; i++)
    {
        if(jobArray[i].job_id == job_id_target)
        {
            strcpy(jobArray[i].status, "Running");
            jobArray[i].job_id = i;
            jobArray[i].state = 2; //2=bg
            pid = jobArray[i].pid;
            break;
        }
    }

    move_to_background(pid);
}

void kill_by_job_id(int job_id_target)
{
    int status;
    pid_t pid = jobArray[job_id_target].pid;

    if(strcmp(jobArray[job_id_target].status, "Stopped") == 0)
    {
        kill(pid, SIGCONT);
        kill(pid, SIGINT);
        waitpid(pid, &status, 0);

        jobArray[job_id_target].pid = -1;
        strcpy(jobArray[job_id_target].status, "NONE");
        strcpy(jobArray[job_id_target].command_line, "");
        jobArray[job_id_target].state = -1; //-1=dummy
    }
    else
    {
        kill(pid, SIGINT);
        waitpid(pid, &status, 0);

        jobArray[job_id_target].pid = -1;
        strcpy(jobArray[job_id_target].status, "NONE");
        strcpy(jobArray[job_id_target].command_line, "");
        jobArray[job_id_target].state = -1; //-1=dummy
    }
}

void kill_by_pid(pid_t pid)
{
    int status;

    for(int i = 0; i < 10; i++)
    {
        if(jobArray[i].pid == pid)
        {
            if(strcmp(jobArray[i].status, "Stopped") == 0)
            {
                kill(pid, SIGCONT);
                kill(pid, SIGINT);
                waitpid(pid, &status, 0);

                jobArray[i].pid = -1;
                strcpy(jobArray[i].status, "NONE");
                strcpy(jobArray[i].command_line, "");
                jobArray[i].state = -1; //-1=dummy
            }
            else
            {
                kill(pid, SIGINT);
                waitpid(pid, &status, 0);

                jobArray[i].pid = -1;
                strcpy(jobArray[i].status, "NONE");
                strcpy(jobArray[i].command_line, "");
                jobArray[i].state = -1; //-1=dummy
            }
            break;
        }
    }
}

int main() {
    char input[1024];
    char command_copy[1024];
    char *args[100] = {NULL};  //args[0] = "cd" args[1] = "bin/ls"
    char *command; //command, excutable file name
    char *arg; // directory, &

    signal(SIGINT, ctrl_c_handler);
    signal(SIGCHLD, sigchld_handler);
    signal(SIGTSTP, ctrl_z_handler);

    // Initialize the jobArray
    
    for (int i = 0; i < 10; i++) 
    {
        jobArray[i].job_id = i;
        jobArray[i].pid = -1;  // Initialize PID to -1 (invalid)
        strcpy(jobArray[i].status, "NONE");
        jobArray[i].state = -1; // Initialize state to -1 (dummy value)
    }

    while (1) 
    {
        for(int i = 0; i < 1024; i++)
        {
            input[i] = '\0';
        }
        printf("prompt > ");
        // fflush(stdout);
        fgets(input, sizeof(input), stdin);
        strcpy(command_copy, input);
        arg = strtok(input, DELIMITER);

        int i = 0;
        // int arg_num = 1;
        int arg_num = 0;

        while (arg != NULL) 
        {
            args[i] = arg;
            arg_num = i;
            // arg_num++;
            i++;
            arg = strtok(NULL, DELIMITER);
        }
        args[i] = NULL;

        command = args[0];

        if (command == NULL)  // if it is just enter or nothing, then retry
        {
            continue;
        }

        if (strcmp(command, "cd") == 0) // args[0] == command   -> cd, pwd, quit, exctuble file name, fg, bg, kill etc     args[1] ~~ args[end]  file's argument,  arg[end] = &
        {
            if (args[1] == NULL) 
            {
                printf("Error: No directory provided.\n");
            } 
            else 
            {
                cd(args[1], arg_num);
            }
        } 
        else if (strcmp(command, "pwd") == 0) 
        {
            if (args[1] != NULL) 
            {
                perform_redirection(args, arg_num, command_copy);
            }
            else
            {
                pwd();
            }
            // pwd();
        } 
        else if (strcmp(command, "quit") == 0) 
        {
            quit();
        }
        else if (command != NULL && strcmp(command, "jobs") != 0 && strcmp(command, "fg") != 0 && strcmp(command, "bg") != 0 && strcmp(command, "kill") != 0 && strcmp(args[arg_num], "&") != 0) // foreground, command = test    args[0] = "test"   arg_num = 0
        {
            run_foreground_command(args, arg_num, command_copy);            
        }
        else if(command != NULL && strcmp(command, "jobs") != 0 && strcmp(command, "fg") != 0 && strcmp(command, "bg") != 0 && strcmp(command, "kill") != 0 &&  strcmp(args[arg_num], "&") == 0) // background
        {
            run_background_command(args, arg_num, command_copy);
        }
        else if (strcmp(command, "jobs") == 0)
        {
            if (args[1] != NULL) 
            {
                perform_redirection(args, arg_num, command_copy);
            }
            else
            {
                jobs();
            }
            // jobs();
        }
        else if(strcmp(command, "fg") == 0)
        {
            size_t length = strlen(args[1]);
            if(length < 3)
            {
                int job_id;
                if (sscanf(args[1], "%%%d", &job_id))
                {
                    fg_by_job_id(job_id); //get only job id
                }
            }
            else
            {
                pid_t pid = atoi(args[1]);
                fg_by_pid(pid);
            }
        }
        else if(strcmp(command, "bg") == 0)
        {
            size_t length = strlen(args[1]);
            if(length < 3)
            {
                int job_id;
                if (sscanf(args[1], "%%%d", &job_id))
                {
                    bg_by_job_id(job_id); //get only job id
                }
            }
            else
            {
                pid_t pid = atoi(args[1]);
                bg_by_pid(pid);
            }
        }
        else if(strcmp(command, "kill") == 0)
        {
            size_t length = strlen(args[1]);
            if(length < 3)
            {
                int job_id;
                if (sscanf(args[1], "%%%d", &job_id))
                {
                    kill_by_job_id(job_id); //get only job id
                }
            }
            else
            {
                pid_t pid = atoi(args[1]);
                kill_by_pid(pid);
            }
        } 
    }
    return 0;
}
