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

There are two types of commands which the shell supports;
1. Local executables, and
2. Built-in commands

The local executable commands are the names of any compiled executable in the local directory.
The executable file ciykd run as either a foreground process or background process.
* A foreground job is one that blocks the shell process, causing it to wait until the foreground job is complete.
* A background job does not block the shell process while the job is executing. We use ampersand (&) operator to indicate the executable will run as a background job.

To run the executable file `helloWorld` as a foreground job, simply run as follows
```
prompt > helloWorld
```
