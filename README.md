# smplshll
This is a C program to experiment with how the shell works.
Aside from commonly known functionalities such as 'pwd' or 'cd', the program aims to experiment with
foreground and background process, utilizing fork() system call.

To start the program, simply build and execute inside the terminal:
```
gcc -o shl shl.c
./shl
```

List of commands supported follows
* cd
* pwd
* jobs
* bg
* fg
* kill
* quit
