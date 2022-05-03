#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <filesystem>
#include <regex>

#include "lsof.hpp"

using namespace std;
using namespace std::filesystem;


const char *proc = "/proc/";

int main(int argc, char* argv[]){

    
    if(stat(proc, buf) == -1){
        perror("stat");
    }

    for (int i = 0; i < argc; i++)
		if(i>0)
            ARG.push_back(argv[i]);
    

    vector<lsof*> task;

    for(auto p : directory_iterator(proc)){

        if(p.status().type() == file_type::directory){
            string PidPath = absolute(p);//proc/[pid]
            string PidNum = p.path().filename();//[pid]
            if(isdigit(PidNum[0])){
                stat(PidPath.c_str(), buf);
                lsof *info = new lsof(PidNum, proc, buf->st_uid);//function2
                task.push_back(info);
            }
        }
    }

    printf("%-30s %10s %15s %10s %10s %10s %10s\n",
            "COMMAND",
            "PID",
            "USER",
            "FD",
            "TYPE",
            "NODE",
            "NAME");

    vector<string> FileDes{"cwd", "root", "exe", "maps", "fd"};

    for(auto t : task){

        string pid = t->PID;
        directory_iterator detail;
        try{
            detail = directory_iterator(proc + pid);
        }
        catch(filesystem_error const& ex){
            continue;
        }

        for(auto f : FileDes){
            string des = proc + pid + "/" + f;
            inherit *all = new inherit(*t, des, buf, f);//function5
            int state = stat(des.c_str(), buf);

            if(f == "cwd" or f == "root" or f == "exe"){
                all->print_all();//function7
            }

            if(f == "fd"){
                if(all->TYPE == "DIR")
                    all->list_FD();//function8
                else
                    all->print_all();
            }
            
            if(state != -1 and f == "maps")
                all->list_maps();//function9
            
        }
    }

    return 0;
}