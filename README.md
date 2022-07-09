# hw1: Implement a 'lsof'-like program  
## Program Arguments  
* `-c REGEX`: a regular expression (REGEX) filter for filtering command line. For example `-c sh` would match `bash`, `zsh`, and `share`.
* `-t TYPE`: a TYPE filter. Valid TYPE includes `REG`, `CHR`, `DIR`, `FIFO`, `SOCK`, and `unknown`. TYPEs other than the listed should be considered invalid. For invalid types, your program has to print out an error message `Invalid TYPE option.` in a single line and terminate your program.
* `-f REGEX`: a regular expression (REGEX) filter for filtering filenames.  

# hw2: Monitor File Activities of Dynamically Linked Programs  
## Program Arguments  
Your program should work with the following arguments:
``` 
usage: ./logger [-o file] [-p sopath] [--] cmd [cmd args ...]  
    -p: set the path to logger.so, default = ./logger.so  
    -o: print output to file, print to "stderr" if no file specified  
    --: separate the arguments for logger and for the command  
```  
If an invalid argument is passed to the logger, the above message should be displayed.  
## Monitored file access activities  
The list of monitored library calls is shown below.  
``` 
chmod chown close creat fclose fopen fread fwrite  
open read remove rename tmpfile write
```    
 

