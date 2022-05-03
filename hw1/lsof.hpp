#pragma once
#ifndef __lsof_hpp__
#define __lsof_hpp__

#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <filesystem>

using namespace std;
using namespace std::filesystem;

typedef struct stat Status;

struct lsof{

    string COMMAND;
    string PID;
    string USER;
    
    string PATH;
    long UID;
    

    lsof(){//function1
        COMMAND = "";
        PID = "";
        USER = "";
        PATH = "";
        UID = 0;
    }

   lsof(string pid, string path, int uid)://function2
        COMMAND(""), PID(pid), USER(""), PATH(path), UID(uid)
    {
        get_COMMAND();//function2-1
        get_USER();//function2-2
    };
    void get_COMMAND();
    void get_USER();

    lsof(const lsof &src) ://function3
		PATH(src.PATH), 
		COMMAND(src.COMMAND),
		PID(src.PID),
		UID(src.UID),
		USER(src.USER) {};
    
};

struct inherit : lsof{
    Status *BUF;
    string FD;
    string TYPE;
    string NODE;
    string NAME;
    string PERM;

    inherit(const lsof &src, Status *buf)://function4
        lsof(src), FD(""), TYPE(""), NODE(""), NAME(""), PERM(""), BUF(buf) {};
    

    inherit(const lsof &src, string &des, Status *buf, string &FileDes)://function5
        lsof(src), FD(""), TYPE(""), NODE(""), NAME(des), PERM(""), BUF(buf)
    {
        try{
            TYPE = get_TYPE(status(NAME));//function5-1
        }
        catch(filesystem_error const& ex){
            TYPE = "unknown";
        }
        check(FileDes);//function5-2
    };
    

    bool operator==(const inherit &bas){//function6
        return (this->COMMAND == bas.COMMAND and
                this->PID == bas.PID and
                this->USER == bas.USER and
                this->FD == bas.FD and
                this->TYPE == bas.TYPE and
                this->NODE == bas.NODE and
                this->NAME == bas.NAME and
                this->PERM == bas.PERM);
    }

    void check(string &FileDes);
    string get_TYPE(std::filesystem::file_status s);
    void print_all();//function7
    void list_FD();//function8
    void list_maps();//function9

};

Status *buf = new Status;
lsof temp;
inherit bas(temp, buf);//function4

string jump_MAPS = "";
vector<string> ARG;

//SubFunction

void lsof::get_COMMAND(){//function2-1
    ifstream input;
    input.open(PATH + PID + "/comm");
    getline(input, COMMAND);
    input.close();
}

void lsof::get_USER(){//function2-2
    struct passwd *pwd;
    pwd = getpwuid(UID);
    USER = pwd->pw_name;
}

string inherit::get_TYPE(file_status s){//function5-1
    switch(s.type()){
        case file_type::none:
            return "NONE";
        case file_type::not_found:
            return "";
        case file_type::regular:
            return "REG";
        case file_type::directory:
            return "DIR";
        case file_type::symlink:
            return "SYM";
        case file_type::block:
            return "BLOCK";
        case file_type::character:
            return "CHR";
        case file_type::fifo:
            return "FIFO";
        case file_type::socket:
            return "IPC";
        case file_type::unknown:
            return "unknown";
        default:
            return "default";
    }
}

void inherit::check(string &FileDes){//function5-2
    stat(NAME.c_str(), BUF);
    NODE = to_string(BUF->st_ino);
    if(FileDes == "cwd"){
        FD = "cwd";
        try{
            directory_iterator check(NAME);
            NAME = read_symlink(NAME);
        }
        catch(filesystem_error const& ex){
            NODE = "";
            PERM = "(Permission denied)";
        }
    }
    else if(FileDes == "root"){
        FD = "rtd";
        try{
            directory_iterator check(NAME);
            NAME = read_symlink(NAME);
        }
        catch(filesystem_error const& ex){
            NODE = "";
            PERM = "(Permission denied)";
        }
    }
    else if(FileDes == "exe"){
        FD = "txt";
        try{
            is_symlink(status(NAME));
        }
        catch(filesystem_error const& ex){
            NODE = "";
            PERM = "(Permission denied)";
        }
        if(TYPE == "REG"){
            NAME = read_symlink(NAME), jump_MAPS = NAME;
        }
    }
    else if(FileDes == "fd"){
        FD = "NOFD";
        try{
            directory_iterator check(NAME);
        }
        catch(filesystem_error const& ex){
            NODE = "";
            TYPE = "";
            PERM = "(Permission denied)";
        }
    }
}

void inherit::print_all(){//function7
    if(bas == *this){
        bas = *this;
        return;
    }

    bas = *this;

    smatch m;
    string target = "";

    //to filter command
    auto it = find(ARG.begin(), ARG.end(), "-c");
    if(it != ARG.end()){
        target = *(it+1);
        regex re(target);
        if(not regex_search(COMMAND, m, re))
            return;

    }

    //to filter type
    it = find(ARG.begin(), ARG.end(), "-t");
    vector<string> TYPEs{"REG", "CHR", "DIR", "FIFO", "SOCK", "unknown"};
    if(it != ARG.end()){
        target = *(it+1);
        if(find(TYPEs.begin(), TYPEs.end(), target) == TYPEs.end()){
            cout << "Invalid TYPE option.";
            exit(0);
        }
        regex re(target);
        if(not regex_search(TYPE, m, re))
            return;
    }

    //to filter filename
    it = find(ARG.begin(), ARG.end(), "-f");
    if(it != ARG.end()){
        target = *(it+1);
        regex re(target);
        if(not regex_search(NAME, m, re))
            return;
    }
    if(NAME.find("(deleted)") != string::npos){
        stringstream ss(NAME);
        string tmp;
        vector<string> v;
        while(ss >> tmp)
            v.push_back(tmp);
        NAME = v[0];
    }

    //print
    printf("%-30s %10s %15s %10s %10s %10s %20s %s\n",
            COMMAND.c_str(),
            PID.c_str(),
            USER.c_str(),
            FD.c_str(),
            TYPE.c_str(),
            NODE.c_str(),
		    NAME.c_str(),
		    PERM.c_str());

}

void inherit::list_FD(){//function8
    directory_iterator detail(NAME);
    for(auto d : detail){//proc/[pid]/fd/[number]
        string fdNum = std::filesystem::absolute(d);
        string fdName = d.path().filename();
        stat(fdNum.c_str(), BUF);

        perms p = symlink_status(fdNum).permissions();
        bool owner_read = ((p & perms::owner_read) != perms::none);
        bool owner_write = ((p & perms::owner_write) != perms::none);

        if(owner_read and owner_write)
            FD = fdName + "u";
        else if(owner_read)
            FD = fdName + "r";
        else if(owner_write)
            FD = fdName + "w";
        
        NAME = read_symlink(d);
        TYPE = get_TYPE(status(NAME));
        if(NAME.find("socket:") != string::npos)
            TYPE = "SOCK";
        if(NAME.find("pipe:") != string::npos or NAME.find(".fifo") != string::npos)
            TYPE = "FIFO";
        if(NAME.find("anon_inode:") != string::npos)
            TYPE = "unknown";

        NODE = to_string(BUF->st_ino);

        print_all();
    }
}

void inherit::list_maps(){//function9
    FD = "mem";
    ifstream f;
    f.open(NAME);
    string target;

    while(getline(f, target)){
        stringstream ss(target);
        string tmp;
        vector<string> v;
        while(ss >> tmp)
            v.push_back(tmp);

        int len = v.size();
        if(v[len-1][0] == '/'){
            NAME = v[len-1];
            FD = (NAME == jump_MAPS) ? "txt" : "mem";
            NODE = v[len-2];
        }
        else if(v[len-1] == "(deleted)"){
            NAME = v[len-2];
            NODE = v[len-3];
            FD = "DEL";
        }
        else continue;

        TYPE = get_TYPE(status(NAME));

        print_all();

    }
    f.close();
}

#endif