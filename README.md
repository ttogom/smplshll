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
- Foreground job : When the job is running in foreground mode, user could press Ctrl+C on the keyboard to kill the process which will call SIGCHLD handler. User can also press Ctrl+Z which would invoke SIGSTP and put the foreground job into a stopped state.
- Stopped job : When the job is in a stopped state, user could either resume the process into background/foreground mode, or to kill the process. User could accomplish this by inputting `fg` or `bg` commands followed by job_id or pid. The command would look like this if the user puts a pid: `fg 12345`. If the user puts a pid, ampersand should be placed right before the id starts: `fg %12345`. Background job will have a similar format except for it has `bg` instead of `fg`.
- Background job : When the job is running in background mode, user could turn it into foreground mode by inputting the command `fg` with job_id or pid. It could also receive kill (with job_id or pid) where it would terminate the process.
> To access which processes are running currently, user may type `jobs` into the prompt to show the background and stopped jobs (`jobs` command won't be typed when the process is running in foreground mode because it blocks and wait for the child process to fully execute and terminated).

Below is a rough diagram of how the job control works in the program.

![controlflow](https://github.com/ttogom/smplshll/assets/16681048/1f20b5ae-1f8e-4922-8ff8-54ae29448b37)
