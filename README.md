# smplshll
This is a C program to experiment with how the shell works.
Aside from commonly known functionalities such as 'pwd' or 'cd', the program aims to experiment with
foreground and background processes, utilizing fork() system call.

To start the program, simply build and execute inside the terminal:
```
gcc -o shl shl.c
./shl
```

When succesfully executed, the shell will print a prompt line for the user to input supported commands.
```
prompt > 
```

## commands
There are two types of commands which the shell supports;
1. Local executables, and
2. Built-in commands

The local executable commands are the names of any compiled executable in the local directory.
The executable file run as either a foreground process or background process.
* A foreground job is one that blocks the shell process, causing it to wait until the foreground job is complete.
* A background job does not block the shell process while the job is executing. We use ampersand (&) operator to indicate the executable will run as a background job.
> To guarantee that the child process is reaped after getting terminated, we use `SIGCHLD` signal (for background job)

### local executables
To run the executable file `helloWorld` as a foreground job, simply run as follows
```
prompt > helloWorld
```
To run the executable file `helloWorld` as a background job, run as like
```
prompt > helloWorld &
```

### job control
- Foreground job : When the job is running in foreground mode, user can press Ctrl+C on the keyboard to kill the process which will call SIGCHLD handler. User can also press Ctrl+Z which would invoke SIGSTP and put the foreground job into a stopped state.
- Stopped job : When the job is in a stopped state, user could either resume the process into background/foreground mode, or to kill the process. User could accomplish this by inputting `fg` or `bg` commands followed by job_id or pid. The command would look like this if the user puts a pid: `fg 12345`. If the user puts a pid, ampersand should be placed right before the id starts: `fg %12345`
