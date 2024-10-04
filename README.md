# PennOS
This project is an implementation of **Penn OS** a custom-built operating system designed for process management, scheduling, file system handling, and basic shell operations.

Please note that **this code should not be copied or reused**, as it is part of a group project and may contain proprietary work from all team members. Reuse or replication of the code may violate academic integrity guidelines.

## Compilation Instructions:

To compile **PennOS**, first ensure that the correct `parser.o` object for your machine is present in the `bin` folder. Once verified, use the following command in the root directory to create the **penn-os** binary:

```bash
make
```
To run the binary, use:

```bash
./bin/penn-os [filesystem]
```
Where [filesystem] is an optional argument specifying the name of an already-created filesystem binary located in ./bin.

## Features

### Shell:
The **PennOS** shell serves as the user interface, enabling users to run various commands (e.g., `cat`, `sleep`, `busy`, `echo`, etc.) using user-level functions, ensuring separation between "user land" and "kernel land." The shell parses user input and either creates new processes (for regular commands) or executes shell built-ins (e.g., `nice`, `man`, `bg`, `fg`, `jobs`, etc.). It also integrates file manipulation via our `pennfat` file system for commands like `ls`, `touch`, `mv`, and `cp`.

### Kernel:
The kernel is responsible for managing the core of **PennOS** by creating and handling forked threads, process cleanup, and process management. It initializes essential systems, such as the logger, scheduler, and file system, while isolating kernel operations from user-level access.

### Scheduler:
Our scheduler uses a priority-based system to control process execution. Processes with priority `-1` run 1.5 times more frequently than those with priority `0`, and similarly, priority `0` processes run 1.5 times more frequently than priority `1`, ensuring fairness without starvation. The scheduler handles context switching, thread management, and controls the execution queue.

### File System:
The **PennOS** file system, implemented in `filesystem.c` and `filesystem2.c`, is based on the **File Allocation Table (FAT)** structure. It supports core file operations such as creating, moving, removing, copying, listing, and manipulating files. Advanced functionalities, such as I/O redirection and permission handling, are also implemented.

