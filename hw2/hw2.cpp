#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

using namespace std;

int main(int argc, char* argv[]) {
	if (argc == 1) {
		cout << "no command given." << endl;
		return 0;
	}

	vector<string> v;
	for (int i=1; i<argc; ++i)
		v.push_back(argv[i]);

	string file = "STDERR", sopath = "./logger.so";
	auto it = find(v.begin(), v.end(), "-o");
	if (it != v.end())
		file = *(it+1);

	it = find(v.begin(), v.end(), "-p");
	if (it != v.end())
		sopath = *(it+1);

	vector<string> cmd_arg;
	it = find(v.begin(), v.end(), "--");
	if (it != v.end())
		cmd_arg = vector<string> (it+1, v.end());

	vector<string> check(v.begin(), it);
	for (auto e : check) {
		if (e[0] == '-' and e[1] != 'p' and e[1] != 'o' and e.size() == 2) {
			printf("./logger: invalid option -- '%c'\n", e[1]);
			cout << "usage: ./logger [-o file] [-p sopath] [--] cmd [cmd args ...]" << endl;
			cout << "\t-p: set the path to logger.so, default = ./logger.so" << endl;
			cout << "\t-o: print output to file, print to \"stderr\" if no file specified" << endl;
			cout << "\t--: separate the arguments for logger and for the command" << endl;
			return 0;
		}
	}

	string preload = "LD_PRELOAD=" + sopath;
	string outfile = "OUTPUT_FILE=" + file;
	int len = (cmd_arg.size() == 0) ? v.size() : cmd_arg.size();

	char* cmd[len+1];
	if (cmd_arg.size() == 0)
		for (size_t i=0; i<v.size(); ++i)
			cmd[i] = strdup(v[i].c_str());
	else 
		for (size_t i=0; i<cmd_arg.size(); ++i)
			cmd[i] = strdup(cmd_arg[i].c_str());
	cmd[len] = NULL;

	char* env[3] = {strdup(preload.c_str()), strdup(outfile.c_str()), NULL};
    int ret = execvpe(cmd[0], cmd, env);
	if (ret == -1)
		cout << "Error\n" << endl;

    return 0;
}