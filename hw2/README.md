## Monitor File Activities of Dynamically Linked Programs  
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
