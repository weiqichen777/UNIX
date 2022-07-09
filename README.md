## hw1: Implement a 'lsof'-like program  
### Program Arguments  
* `-c REGEX`: a regular expression (REGEX) filter for filtering command line. For example `-c sh` would match `bash`, `zsh`, and `share`.
* `-t TYPE`: a TYPE filter. Valid TYPE includes `REG`, `CHR`, `DIR`, `FIFO`, `SOCK`, and `unknown`. TYPEs other than the listed should be considered invalid. For invalid types, your program has to print out an error message `Invalid TYPE option.` in a single line and terminate your program.
* `-f REGEX`: a regular expression (REGEX) filter for filtering filenames.  

## hw2: Monitor File Activities of Dynamically Linked Programs  
### Program Arguments  
Your program should work with the following arguments:
``` 
usage: ./logger [-o file] [-p sopath] [--] cmd [cmd args ...]  
    -p: set the path to logger.so, default = ./logger.so  
    -o: print output to file, print to "stderr" if no file specified  
    --: separate the arguments for logger and for the command  
```  
If an invalid argument is passed to the *logger*, the above message should be displayed.  
### Monitored file access activities  
The list of monitored library calls is shown below.  
``` 
chmod  chown  close   creat   fclose   fopen  fread  fwrite  
open   read   remove  rename  tmpfile  write
```    

## hw3: Extend the Mini Lib C to Handle Signals  
In this homework, you have to extend the mini C library introduced in the class to support signal relevant system calls. You have to implement the following C library functions in Assembly and C using the syntax supported by yasm x86_64 assembler.

1.  setjmp: prepare for long jump by saving the current CPU state. In addition, preserve the signal mask of the current process.
2.  longjmp: perform the long jump by restoring a saved CPU state. In addition, restore the preserved signal mask.
3.  signal and sigaction: setup the handler of a signal.
4.  sigprocmask: can be used to block/unblock signals, and get/set the current signal mask.
5.  sigpending: check if there is any pending signal.
6.  alarm: setup a timer for the current process.
7.  write: write to a file descriptor.
8.  pause: wait for signal
9.  sleep: sleep for a specified number of seconds
10.  exit: cause normal process termination
11.  strlen: calculate the length of the string, excluding the terminating null byte ('\0').
12.  functions to handle sigset_t data type: sigemptyset, sigfillset, sigaddset, sigdelset, and sigismember.
The API interface is the same to what we have in the standard C library. However, because we are attempting to replace the standard C library, our test codes will only be linked against the library implemented in this homework. We will use the following commands to assemble, compile, link, and test your implementation.  
```  
$ make		# Use your provided Makefile to generate `libmini.so`
$ yasm -f elf64 -DYASM -D__x86_64__ -DPIC *start.asm* -o start.o
$ gcc -c -g -Wall -fno-stack-protector -nostdlib -I. -I.. -DUSEMINI *test.c*
$ ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o test test.o start.o -L. -L.. -lmini
```  
Please notice that there is a -nostdlib parameter passed to the compiler, which means that you could not use any existing functions implemented in the standard C library. Only the functions you have implemented in the libmini64.asm and libmini.c file can be used. In addition to your library source code, you also have to provide a corresponding libmini.h file. The testing codes will include this file and use the function prototypes and data types defined in the header file.  
You can build your libsmini.so by the following commands:  
```  
$ yasm -f elf64 -DYASM -D__x86_64__ -DPIC libmini64.asm -o libmini64.o
$ gcc -c -g -Wall -fno-stack-protector -fPIC -nostdlib libmini.c
$ ld -shared -o libmini.so libmini64.o libmini.o
```  
To ensure that your library can handle signals properly, your implemented setjmp function **must save the signal mask** of the current process in your customized jmp_buf data structure. The saved signal mask **must be restored** by the longjmp function. **This requirement is different from the default setjmp/longjmp implementation.**  

## hw4: Simplified Scriptable Instruction Level Debugger  
In this homework, you have to implement a simple instruction-level debugger that allows a user to debug a program interactively at the assembly instruction level. You can implement the debugger by using the ptrace interface. The commands you have to implement are summarized as follows:  
```  
- break {instruction-address}: add a break point
- cont: continue execution
- delete {break-point-id}: remove a break point
- disasm addr: disassemble instructions in a file or a memory region
- dump addr: dump memory content
- exit: terminate the debugger
- get reg: get a single value from a register
- getregs: show registers
- help: show this message
- list: list break points
- load {path/to/a/program}: load a program
- run: run the program
- vmmap: show memory layout
- set reg val: get a single value to a register
- si: step into instruction
- start: start the program and stop at the first instruction
```  
The details of each command are explained below. We use brackets right after a command to enclose the list of the state(s) that the command should support.
* **break** or **b [running]**: Setup a break point. If a program is loaded but is not running, you can simply display an error message. When a break point is hit, you have to output a message and indicate the corresponding address and instruction. The address of the break point should be within the range specified by the text segment in the ELF file and will not be the same as the entry point.
* **cont** or **c [running]**: continue the execution when a running program is stopped (suspended).
* **delete [running]**: remove a break point. Please remember to handle illegal situations, like deleting non-existing break points.
disasm or d [running]: Disassemble instructions in a file or a memory region. The address of each instruction should be within the range specified by the text segment in the ELF file. You only have to dump 10 instructions for each command. If **disasm** command is executed without an address, you can simply output `** no addr is given`. Please note that the output should not have the machine code `cc`. See the demonstration section for the sample output format.
dump or x [running]: Dump memory content. You only have to dump 80 bytes from a given address. The output contains the addresses, the hex values, and printable ASCII characters. If dump command is executed without an address, you can simply output ** no addr is given. Please note that the output should include the machine code cc if there is a break point.
exit or q [any]: Quit from the debugger. The program being debugged should be killed as well.
get or g [running]: Get the value of a register. Register names are all in lowercase.
getregs [running]: Get the value of all registers.
help or h [any]: Show the help message.
list or l [any]: List break points, which contains index numbers (for deletion) and addresses.
load [not loaded]: Load a program into the debugger. When a program is loaded, you have to print out the address of entry point.
run or r [loaded and running]: Run the program. If the program is already running, show a warning message and continue the execution. If the program is loaded, start the program and continue the execution.
vmmap or m [running]: Show memory layout for a running program. If a program is not running, you can simply display an error message. The memory layout is:
[address] [perms] [offset] [pathname]
Check the demonstration section for the sample output format.
set or s [running]: Set the value of a register
si [running]: Run a single instruction, and step into function calls.
start [loaded]: Start the program and stop at the first instruction.
