#include <iostream>
#include <dlfcn.h>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <stdarg.h>

using namespace std;

int flag_fd = 0;
int stderrfd = 2;

void file_init(){
    auto org_open = (int (*)(const char *, int, mode_t)) dlsym(RTLD_NEXT, "open");
    char* env = getenv("OUTPUT_FILE");

    if(strcmp(env ,"STDERR") != 0){
        stderrfd = org_open(env, O_RDWR | O_CREAT | O_APPEND, 0644);
    }
    else{
        stderrfd = dup(2);
    }
    
    flag_fd = 1;
}

void print(string output){
    auto org_write = (ssize_t (*)(int,const void*,size_t))dlsym(RTLD_NEXT, "write");
    org_write(stderrfd, output.c_str(), strlen(output.c_str()));
}

int creat(const char *path, mode_t mode){
    if(!flag_fd) file_init();
    
    int (*org_creat) (const char *, mode_t) = (int (*)(const char *, mode_t)) dlsym(RTLD_NEXT, "creat");
    int ans = org_creat(path, mode);

    char abs_path[1024];
    realpath(path, abs_path);

    //printf("[logger] creat(\"%s\", %o) = %d\n", abs_path, mode, ans);

    char output_mode[20];
    sprintf(output_mode, "%o", mode);
    string output = "[logger] creat(\"" + string(abs_path) + "\", " + string(output_mode) + ") = " + to_string(ans) + "\n";
    print(output);

    return ans;
}

int chmod(const char *path, mode_t mode){
    if(!flag_fd) file_init();    
    
    int (*org_chmod) (const char *, mode_t) = (int (*)(const char *, mode_t)) dlsym(RTLD_NEXT, "chmod");
    int ans = org_chmod(path, mode);

    char abs_path[1024];
    realpath(path, abs_path);

    //printf("[logger] chmod(\"%s\", %o) = %d\n", abs_path, mode, ans);

    char output_mode[20];
    sprintf(output_mode, "%o", mode);
    string output = "[logger] chmod(\"" + string(abs_path) + "\", " + string(output_mode) + ") = " + to_string(ans) + "\n";
    print(output);

    return ans;
}

int chown(const char *pathname, uid_t owner, gid_t group){
    if(!flag_fd) file_init();
    
    int (*org_chown) (const char *, uid_t, gid_t) = (int (*)(const char *, uid_t, gid_t)) dlsym(RTLD_NEXT, "chown");
    int ans = org_chown(pathname, owner, group);

    char abs_path[1024];
    realpath(pathname, abs_path);

    //printf("[logger] chown(\"%s\", %d, %d) = %d\n", abs_path, owner, group, ans);

    string output = "[logger] chown(\"" + string(abs_path) + "\", " + to_string(owner) + ", " + to_string(group) + ") = " + to_string(ans) + "\n";
    print(output);

    return ans;
}

int rename(const char *oldpath, const char *newpath){
    if(!flag_fd) file_init();
    
    int (*org_rename) (const char *, const char *) = (int (*)(const char *, const char *)) dlsym(RTLD_NEXT, "rename");
    int ans = org_rename(oldpath, newpath);

    char abs_oldpath[1024];
    realpath(oldpath, abs_oldpath);

    char abs_newpath[1024];
    realpath(newpath, abs_newpath);

    //printf("[logger] rename(\"%s\", \"%s\") = %d\n", abs_oldpath, abs_newpath, ans);

    string output = "[logger] rename(\"" + string(abs_oldpath) + "\", " + string(abs_newpath) + ") = " + to_string(ans) + "\n";
    print(output);

    return ans;
}

int open(const char *pathname, int flags, ...){
    if(!flag_fd) file_init();
    
    mode_t mode = 0;
    int (*org_open) (const char *, int, ...) = (int (*)(const char *, int, ...)) dlsym(RTLD_NEXT, "open");

    int ans;
    if(__OPEN_NEEDS_MODE (flags)){
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, int);
        va_end(args);
        ans = org_open(pathname, flags, mode);
    }
    else{
        ans = org_open(pathname, flags);
    }

    char abs_path[1024];
    realpath(pathname, abs_path);

    //printf("[logger] open(\"%s\", %d, %o) = %d\n", abs_path, flags, mode, ans);

    string output = "[logger] open(\"" + string(abs_path) + "\", " + to_string(flags) + to_string(mode) + ") = " + to_string(ans) + "\n";
    print(output);

    return ans;
}

ssize_t write(int fd, const void *buf, size_t nbyte){
    if(!flag_fd) file_init();
    
    ssize_t (*org_write) (int, const void *, size_t) = (ssize_t (*)(int, const void *, size_t)) dlsym(RTLD_NEXT, "write");
    ssize_t ans = org_write(fd, buf, nbyte);

    char abs_path[1024];
    const char *pathname = string("/proc/self/fd/" + to_string(fd)).c_str();
    realpath(pathname, abs_path);

    string buffer;
    for(int i = 0; i < nbyte; i++){
        
        if(*((char*)buf+i) == '\0')
            break;

        if(isprint(*((char*)buf+i)))
            buffer += *((char*)buf+i);
        
        else
            buffer += '.';
    }
    
    //printf("[logger] write(\"%s\", \"%s\", %ld) = %ld\n", abs_path, buffer, nbyte, ans);

    string output = "[logger] write(\"" + string(abs_path) + "\", \"" + buffer + "\", " +to_string(nbyte) + ") = " + to_string(ans) + "\n";
    print(output);

    return ans;

}

int close(int fd){
    if(!flag_fd) file_init();
    int (*org_close) (int) = (int (*)(int)) dlsym(RTLD_NEXT, "close");

    char abs_path[1024];
    string output_path;
    int length = readlink(string("/proc/self/fd/"+to_string(fd)).c_str(), abs_path, 1023);
    if(length != -1){
        abs_path[length] = '\0';
        output_path = string(abs_path);
    }

    int ans = org_close(fd);
    
    //printf("[logger] close(\"%s\") = %d\n", abs_path, ans);
    
    string output = "[logger] close(\"" + output_path + "\") = " + to_string(ans) + "\n";
    print(output);
    
    return ans;
}

ssize_t read(int fd, void *buf, size_t count){
    if(!flag_fd) file_init();
    
    ssize_t (*org_read) (int, void *, size_t) = (ssize_t (*)(int, void *, size_t)) dlsym(RTLD_NEXT, "read");
    ssize_t ans = org_read(fd, buf, count);

    char abs_path[1024];
    string output_path;
    int length = readlink(string("/proc/self/fd/" + to_string(fd)).c_str(), abs_path, 1023);
    if(length != -1){
        abs_path[length] = '\0';
        output_path = string(abs_path);
    }

    string buffer;
    for(int i = 0; i < ans; i++){
        if(*((char*)buf+i) == '\0')
            break;

        if(isprint(*((char*)buf+i)))
            buffer += *((char*)buf+i);
        else
            buffer += '.';
    }

    //printf("[logger] read(\"%s\", \"%s\", %ld) = %ld\n", abs_path, buffer, count, ans);
    
    string output = "[logger] read(\"" + output_path + "\", \"" + buffer + "\", " + to_string(count) + ") = " + to_string(ans) + "\n";
    print(output);
    
    return ans;
}

FILE *tmpfile(void){
    if(!flag_fd) file_init();
    
    FILE* (*org_tmpfile) (void) = (FILE* (*)(void)) dlsym(RTLD_NEXT, "tmpfile");
    FILE* ans = org_tmpfile();

    //printf("[logger] tmpfile() = %p\n", ans);

    char output_ans[20];
    sprintf(output_ans, "%p", ans);
    string output = "[logger] tmpfile() = " + string(output_ans) + "\n";
    print(output);

    return ans;
}

size_t fwrite(const void *ptr, size_t size, size_t nitems, FILE *stream){
    if(!flag_fd) file_init();
    size_t (*org_fwrite) (const void *, size_t, size_t, FILE *) = (size_t (*)(const void *, size_t, size_t, FILE *)) dlsym(RTLD_NEXT, "fwrite");

    string buffer;
    for(int i = 0; i < nitems; i++){
        if(*((char*)ptr+i) == '\0')
            break;

        if(isprint(*((char*)ptr+i)))
            buffer += *((char*)ptr+i);
        else
            buffer += '.';
    }

    char abs_path[1024];
    string output_path;
    int fd = fileno(stream);
    int length = readlink(string("/proc/self/fd/"+to_string(fd)).c_str(), abs_path, 1023);
    if(length != -1){
        abs_path[length] = '\0';
        output_path = string(abs_path);
    }

    size_t ans = org_fwrite(ptr, size, nitems, stream);

    //printf("[logger] fwrite(\"%s\", %ld, %ld, \"%s\") = %ld\n", buffer, size, nitems, abs_path, ans);
    
    string output = "[logger] fwrite(\"" + buffer + "\", " + to_string(size) + ", " + to_string(nitems) + ", \"" + output_path + "\") = " + to_string(ans) + "\n";
    print(output);
    
    return ans;
}

int fclose(FILE *stream){
    if(!flag_fd) file_init();
    int (*org_fclose) (FILE *) =  (int (*)(FILE *)) dlsym(RTLD_NEXT, "fclose");
    
    char abs_path[1024];
    string output_path;
    int fd = fileno(stream);
    int length = readlink(string("/proc/self/fd/"+to_string(fd)).c_str(), abs_path, 1023);
    if(length != -1){
        abs_path[length] = '\0';
        output_path = string(abs_path);
    }
    int ans = org_fclose(stream);
    
    //printf("[logger] fclose(\"%s\") = %d\n", abs_path, ans);

    string output = "[logger] fclose(\"" + output_path + "\") = " + to_string(ans) + "\n";
    print(output);

    return ans;
}

FILE *fopen(const char *pathname, const char *mode){
    if(!flag_fd) file_init();
    
    FILE * (*org_fopen) (const char *, const char *) = (FILE * (*)(const char *, const char *)) dlsym(RTLD_NEXT, "fopen");
    FILE * ans = org_fopen(pathname, mode);

    char abs_path[1024];
    realpath(pathname, abs_path);

    //printf("[logger] fopen(\"%s\", \"%s\") = %p\n", abs_path, mode, ans);

    char output_ans[20];
    sprintf(output_ans, "%p", ans);
    string output = "[logger] fopen(\"" + string(abs_path) + "\", \"" + mode + "\") = " + string(output_ans) + "\n";
    print(output);
    
    return ans;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream){
    if(!flag_fd) file_init();
    
    size_t (*org_fread) (void *, size_t, size_t, FILE *) = (size_t (*)(void *, size_t, size_t, FILE *)) dlsym(RTLD_NEXT, "fread");
    size_t ans = org_fread(ptr, size, nmemb, stream);

    string buffer;
    for(int i = 0; i < ans; i++){
        if(*((char*)ptr+i) == '\0')
            break;

        if(isprint(*((char*)ptr+i)))
            buffer += *((char*)ptr+i);
        else
            buffer += '.';
    }

    char abs_path[1024];
    int fd = fileno(stream);
    const char *pathname = string("/proc/self/fd/" + to_string(fd)).c_str();
    realpath(pathname, abs_path);

    //printf("[logger] fread(\"%s\", %ld, %ld, \"%s\") = %ld\n", buffer, size, nmemb, abs_path, ans);
    
    string output = "[logger] fread(\"" + buffer + "\", " + to_string(size) + ", " + to_string(nmemb) + ", \"" + string(abs_path) + "\") = " + to_string(ans) + "\n";
    print(output);
    
    return ans;
}

int remove(const char *pathname){
    if(!flag_fd) file_init();
    
    int (*org_remove) (const char *) = (int (*)(const char *)) dlsym(RTLD_NEXT, "remove");
    int ans = org_remove(pathname);

    char abs_path[1024];
    realpath(pathname, abs_path);

    //printf("[logger] remove(\"%s\") = %d\n", abs_path, ans);
    
    string output = "[logger] remove(\"" + string(abs_path) + "\") = " + to_string(ans) + "\n";
    print(output);
    
    return ans;
}


