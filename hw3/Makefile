all: libmini.h libmini.c libmini64.asm
	yasm -f elf64 -DYASM -D__x86_64__ -DPIC libmini64.asm -o libmini64.o
	gcc -c -g -Wall -fno-stack-protector -fPIC -nostdlib libmini.c
	gcc -shared -fno-stack-protector -fPIC -nostdlib -o libmini.so libmini64.o libmini.o 

clean:
	rm *.o libmini.so 
