#include <bits/stdc++.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <capstone/capstone.h>
#include <string.h>
#include <limits.h>
#include <elf.h>
#include <inttypes.h>
#include <string>
#include <string_view>

#if defined(__LP64__)
#define ElfW(type) Elf64_ ## type
#else
#define ElfW(type) Elf32_ ## type
#endif

#define MAX_CMD     128
#define DISASM_INS  10
#define WORD        8

using namespace std;
using namespace std::literals;

struct ELF_info {
    ElfW(Ehdr) *header;
    ElfW(Shdr) *text;
} elf_info;

struct Breakpoint_info {
    int id;
    long backup;
};

enum stage {NLOADED, LOADED, RUN, WAIT, EXIT};
enum stage state;
struct user_regs_struct regs{};
char loaded_prog[MAX_CMD];
unordered_map<unsigned long long, Breakpoint_info *> breakpoints;
pid_t pid;
int bp_id = 0;

void errquit(const char *msg) {
    perror(msg);
    exit(-1);
}

int32_t user_regs_lookup(std::string_view name) {
    // map from register name to offset of user_regs_struct
    static const auto lookup = []() {
        std::unordered_map<std::string_view, uint32_t> m;
        m.emplace("r15"sv, offsetof(user_regs_struct, r15));
        m.emplace("r14"sv, offsetof(user_regs_struct, r14));
        m.emplace("r13"sv, offsetof(user_regs_struct, r13));
        m.emplace("r12"sv, offsetof(user_regs_struct, r12));
        m.emplace("rbp"sv, offsetof(user_regs_struct, rbp));
        m.emplace("rbx"sv, offsetof(user_regs_struct, rbx));
        m.emplace("r11"sv, offsetof(user_regs_struct, r11));
        m.emplace("r10"sv, offsetof(user_regs_struct, r10));
        m.emplace("r9"sv, offsetof(user_regs_struct, r9));
        m.emplace("r8"sv, offsetof(user_regs_struct, r8));
        m.emplace("rax"sv, offsetof(user_regs_struct, rax));
        m.emplace("rcx"sv, offsetof(user_regs_struct, rcx));
        m.emplace("rdx"sv, offsetof(user_regs_struct, rdx));
        m.emplace("rsi"sv, offsetof(user_regs_struct, rsi));
        m.emplace("rdi"sv, offsetof(user_regs_struct, rdi));
        m.emplace("orig_rax"sv, offsetof(user_regs_struct, orig_rax));
        m.emplace("rip"sv, offsetof(user_regs_struct, rip));
        m.emplace("cs"sv, offsetof(user_regs_struct, cs));
        m.emplace("eflags"sv, offsetof(user_regs_struct, eflags));
        m.emplace("rsp"sv, offsetof(user_regs_struct, rsp));
        m.emplace("ss"sv, offsetof(user_regs_struct, ss));
        m.emplace("fs_base"sv, offsetof(user_regs_struct, fs_base));
        m.emplace("gs_base"sv, offsetof(user_regs_struct, gs_base));
        m.emplace("ds"sv, offsetof(user_regs_struct, ds));
        m.emplace("es"sv, offsetof(user_regs_struct, es));
        m.emplace("fs"sv, offsetof(user_regs_struct, fs));
        m.emplace("gs"sv, offsetof(user_regs_struct, gs));
        return m;
    }();

    auto it = lookup.find(name);
    if (it == lookup.end()) {
        return -1;
    }
    return (int32_t)(it->second);
}

int disasm(const uint8_t *code, size_t code_size, uint64_t addr, size_t _count) {
    csh handle;
	cs_insn *insn;
	size_t count;

	if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK)
		return -1;
	count = cs_disasm(handle, code, code_size, addr, _count, &insn);
	if (count > 0) {
		size_t j;
		for (j = 0; j < count; j++) {
            if(insn[j].address < elf_info.text->sh_addr || insn[j].address >= elf_info.text->sh_addr + elf_info.text->sh_size) {
                printf("** the address is out of the range of the text segment\n");
                return -1;
            }
            printf("\t%" PRIx64 ": ", insn[j].address);
            for(size_t k = 0; k < 12; k++) {
                if(k < insn[j].size) printf("%02x ", (unsigned int) insn[j].bytes[k]);
                else printf("  ");
            }
			printf("\t%-5s\t%s\n", insn[j].mnemonic, insn[j].op_str);
		}
		cs_free(insn, count);
	} else
		printf("** ERROR: Failed to disassemble given code!\n");

	cs_close(&handle);

    return 0;
}

void read_elf_header(const char* elfFile, ElfW(Ehdr) *header, ElfW(Shdr) *text) {
    FILE* file = fopen(elfFile, "rb");
    if(file) {
        // read the header
        fread(header, 1, sizeof(ElfW(Ehdr)), file);
    }
    if (memcmp(header->e_ident, ELFMAG, SELFMAG) == 0) {
        // this is a valid elf file
        // read all sections header
        ElfW(Shdr) sectHdr;
        fseek(file, header->e_shoff + header->e_shstrndx * sizeof(sectHdr), SEEK_SET);
        fread(&sectHdr, 1, sizeof(sectHdr), file);
        char *SectNames = (char *)malloc(sectHdr.sh_size);
        fseek(file, sectHdr.sh_offset, SEEK_SET);
        fread(SectNames, 1, sectHdr.sh_size, file);
        for(int i = 0; i < header->e_shnum; i++) {
            fseek(file, header->e_shoff + i * sizeof(ElfW(Shdr)), SEEK_SET);
            fread(&sectHdr, 1, sizeof(ElfW(Shdr)), file);
            
            if(sectHdr.sh_name && !strncmp(SectNames + sectHdr.sh_name, ".text", 5)) {
                memcpy(text, &sectHdr, sizeof(sectHdr));
                break;
            }
        }
    }
    // finally close the file
    fclose(file);
}

void parse_elf_header(const char* elfFile) {
    ElfW(Ehdr) *tmp = new ElfW(Ehdr);
    ElfW(Shdr) *text = new ElfW(Shdr);
    // parse header
    read_elf_header(elfFile, tmp, text);
    elf_info.header = tmp;
    elf_info.text = text;
}

// All do functions appear here
void do_help() {
    printf("- break {instruction-address}: add a break point\n");
    printf("- cont: continue execution\n");
    printf("- delete {break-point-id}: remove a break point\n");
    printf("- disasm addr: disassemble instructions in a file or a memory region\n");
    printf("- dump addr: dump memory content\n");
    printf("- exit: terminate the debugger\n");
    printf("- get reg: get a single value from a register\n");
    printf("- getregs: show registers\n");
    printf("- help: show this message\n");
    printf("- list: list break points\n");
    printf("- load {path/to/a/program}: load a program\n");
    printf("- run: run the program\n");
    printf("- vmmap: show memory layout\n");
    printf("- set reg val: get a single value to a register\n");
    printf("- si: step into instruction\n");
    printf("- start: start the program and stop at the first instruction\n");
}

void do_load(char *file) {
    if(strncmp(file, "./", 2)) strncpy(loaded_prog, "./", MAX_CMD-1);
    strncat(loaded_prog, file, MAX_CMD-1);
    // Read ELF file
    parse_elf_header(loaded_prog);
    printf("** program '%s' loaded. entry point 0x%lx\n", loaded_prog, elf_info.header->e_entry);
    state = LOADED;
}

void do_start() {
    if((pid = fork()) < 0) errquit("** fork");
    if(pid == 0) {
        if(ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) errquit("** ptrace@child");
        char *args[] = {loaded_prog, NULL};
        execvp(loaded_prog, args);
        errquit("** execvp");
    }
    else {
        printf("** pid %d\n", pid);
        int status;
        if(waitpid(pid, &status, 0) < 0) errquit("** waitpid");
        if(ptrace(PTRACE_SETOPTIONS, pid, NULL, PTRACE_O_EXITKILL) < 0) errquit("** ptrace@PTRACE_SETOPTIONS");
        if(ptrace(PTRACE_GETREGS, pid, NULL, &regs) < 0) errquit("** ptrace(GETREGS)");
        state = RUN;
    }
}

void do_vmmap() {
    FILE *maps = fopen(("/proc/" + to_string(pid) + "/maps").c_str(), "rb");
    if(!maps) {
        printf("** vmmap fails to open the maps file\n");
        return;
    }
    char addr[MAX_INPUT];
    char flags[MAX_INPUT];
    long offset;
    char filename[MAX_INPUT];
    while(fscanf(maps, "%s %s %ld %*s %*d %s\n", addr, flags, &offset, filename) == 4) {
        flags[3] = '\0';
        unsigned long long faddr, baddr;
        faddr = strtoul(strtok(addr, "-"), NULL, 16);
        baddr = strtoul(strtok(NULL, "-"), NULL, 16);
        printf("%016llx-%016llx %s %ld\t%s\n", faddr, baddr, flags, offset, filename);
    }
}

void do_disasm(unsigned long long addr) {
    unsigned long long int target_addr = addr;
    long codes[DISASM_INS];
    for (int i = 0; i < DISASM_INS; i++) {
        long content = ptrace(PTRACE_PEEKTEXT, pid, target_addr, 0);
        codes[i] = content;
        target_addr += WORD;
    }
    // Restore Context
    for(int i = 0; i < DISASM_INS; i++) {
        for(int j = 0; j < WORD; j++) {
            if((((unsigned char *)&codes[i])[j]) == 0xcc) {
                (((unsigned char *)&codes[i])[j]) = breakpoints[addr + i*WORD + j]->backup;
            }
        }
    }
    disasm((uint8_t *)codes, sizeof(codes) - 1, addr, DISASM_INS);
}

void do_getregs() {
    printf("RAX %-16llxRBX %-16llxRCX %-16llxRDX %-16llx\n", regs.rax, regs.rbx, regs.rcx, regs.rdx);
    printf("R8  %-16llxR9  %-16llxR10 %-16llxR11 %-16llx\n", regs.r8, regs.r9, regs.r10, regs.r11);
    printf("R12 %-16llxR13 %-16llxR14 %-16llxR15 %-16llx\n", regs.r12, regs.r13, regs.r14, regs.r15);
    printf("RDI %-16llxRSI %-16llxRBP %-16llxRSP %-16llx\n", regs.rdi, regs.rsi, regs.rbp, regs.rsp);
    printf("RIP %-16llxFLAGS %016llx\n", regs.rip, regs.eflags);
}

void do_cont() {
    // Need to check if is on breakpoint, if yes, call step
    unsigned long addr = 0;
    if(breakpoints.find(regs.rip) != breakpoints.end()) {
        addr = regs.rip;
        long code = (ptrace(PTRACE_PEEKTEXT, pid, addr, 0) & 0xffffffffffffff00) | (breakpoints[addr]->backup & 0x00000000000000ff);
        if(ptrace(PTRACE_POKETEXT, pid, addr, code) < 0) errquit("** ptrace(POKETEXT)"); 
        if(ptrace(PTRACE_SINGLESTEP, pid, 0, 0) < 0) errquit("** ptrace(STEP)");
        int status;
        if(waitpid(pid, &status, 0) < 0) errquit("** waitpid");
        if(ptrace(PTRACE_POKETEXT, pid, addr, (code & 0xffffffffffffff00) | 0xcc) < 0) errquit("** ptrace(POKETEXT)"); 
    }
    if(ptrace(PTRACE_CONT, pid, 0, 0) < 0) errquit("** ptrace(CONT)");
    state = WAIT; 
}

void do_run() {
    if(state == RUN) printf("** program %s is already running\n", loaded_prog);
    else if(state == LOADED) {
        do_start();
        // restore all breakpoints
        for(auto bp: breakpoints) {
            bp.second->backup = ptrace(PTRACE_PEEKTEXT, pid, bp.first, 0);
            if(ptrace(PTRACE_POKETEXT, pid, bp.first, (bp.second->backup & 0xffffffffffffff00) | 0xcc) < 0) errquit("** ptrace(POKETEXT)"); 
        }
    }
    do_cont();
}

void do_break(unsigned long long addr) {
    if(addr < elf_info.text->sh_addr || addr >= elf_info.text->sh_addr + elf_info.text->sh_size) {
        printf("** the address is out of the range of the text segment\n");
        return;
    }
    if(breakpoints.find(addr) != breakpoints.end()) {
        printf("** breakpoint already exsists\n");
        return;
    }
    Breakpoint_info *bp = new Breakpoint_info;
    bp->backup = ptrace(PTRACE_PEEKTEXT, pid, addr, 0);
    bp->id = bp_id++;
    breakpoints[addr] = bp;
    if(ptrace(PTRACE_POKETEXT, pid, addr, (bp->backup & 0xffffffffffffff00) | 0xcc) < 0) errquit("** ptrace(POKETEXT)"); 
}

void hit_break(unsigned long long addr) {
    printf("** breakpoint @ ");
    disasm((uint8_t *)&breakpoints[addr]->backup, sizeof(long) - 1, addr, 1);

    regs.rip = addr;
    if(ptrace(PTRACE_SETREGS, pid, 0, &regs) < 0) errquit("** ptrace(SETREGS)");
}

void do_list() {
    unsigned long addr[bp_id];
    for(int i = 0; i < bp_id; i++) addr[i] = 0;
    for(auto b: breakpoints) addr[b.second->id] = b.first;
    for(int i = 0 ; i < bp_id; i++) {
        if(addr[i]) printf("%3d: %" PRIx64 "\n", i, addr[i]);
    }
}

void do_delete(int id) {
    for(auto b: breakpoints) {
        if(b.second->id == id) {
            // restore context first
            long code = (ptrace(PTRACE_PEEKTEXT, pid, b.first, 0) & 0xffffffffffffff00) | (breakpoints[b.first]->backup & 0x00000000000000ff);
            if(ptrace(PTRACE_POKETEXT, pid, b.first, code) < 0) errquit("** ptrace(POKETEXT)"); 
            free(b.second);
            breakpoints.erase(b.first);

            for(auto bp: breakpoints) if(bp.second->id > id) --bp.second->id;
            --bp_id;
            return;
        }
    }
    printf("** id given is not available.\n");
}

void do_step() {
    // 1. Check if rip is on breakpoint, if yes, restore and step, than set breakpoint.
    // 2. Check if new rip is on breakpoint, if yes, print breakpoint info
    unsigned long addr = 0;
    long code;
    if(breakpoints.find(regs.rip) != breakpoints.end()) {
        addr = regs.rip;
        code = (ptrace(PTRACE_PEEKTEXT, pid, addr, 0) & 0xffffffffffffff00) | (breakpoints[addr]->backup & 0x00000000000000ff);
        if(ptrace(PTRACE_POKETEXT, pid, addr, code) < 0) errquit("** ptrace(POKETEXT)"); 
    }

    if(ptrace(PTRACE_SINGLESTEP, pid, 0, 0) < 0) errquit("** ptrace(STEP)");
    int status;
    if(waitpid(pid, &status, 0) < 0) errquit("** waitpid");

    if(addr) {
        if(ptrace(PTRACE_POKETEXT, pid, addr, (code & 0xffffffffffffff00) | 0xcc) < 0) errquit("** ptrace(POKETEXT)"); 
    }

    if(WIFEXITED(status)) {
        printf("** child process %d terminiated normally (code %d)\n", pid, status);
        state = LOADED;
    }
    else if(WIFSTOPPED(status)) {
        if(ptrace(PTRACE_GETREGS, pid, NULL, &regs) < 0) errquit("** ptrace(GETREGS)");
        state = RUN;
        if(WSTOPSIG(status) == SIGTRAP && breakpoints.size() != 0) {
            if(breakpoints.find(regs.rip) != breakpoints.end()) {
                hit_break(regs.rip);
            }
        }
    }
}

void do_set(char *reg, unsigned long value) {
    *((int32_t *)((char *)&regs + user_regs_lookup(reg))) = value;
    if(ptrace(PTRACE_SETREGS, pid, 0, &regs) < 0) errquit("** ptrace(SETREGS)");
}

void do_dump(unsigned long addr) {
    int size = 80, line = 16;
    for (int i = 0; i < size; i += line) {
        printf("\t%" PRIx64 ": ", addr);
        string mem = "";
        for (auto j = 0; j < line; j += WORD, addr += WORD) {
            auto tracee_mem = ptrace(PTRACE_PEEKTEXT, pid, addr, NULL);
            mem += string((char *)&tracee_mem, 8);
        }

        for (auto _char : mem) {
            printf("%02x ", (unsigned char)_char);
        }
        printf("|");
        for (auto _char : mem) {
            printf("%c", (!isprint(_char) ? '.' : _char));
        }
       printf("|\n");
    }
}

void run_cmd(char *cmd) {
    if(!strncmp(cmd, "break", 5) || !strncmp(cmd, "b", 1)) {
        if(state != RUN) return;
        char *tmp = strtok(cmd, " ");
        tmp = strtok(NULL, " ");
        if(tmp == NULL) {
            printf("** no addr is given.\n");
            return;
        }
        unsigned long long addr = strtoul(tmp, NULL, 16);
        do_break(addr);
    }
    else if(!strncmp(cmd, "cont", 4) || !strcmp(cmd, "c")) {
        if(state != RUN) {
            printf("** cont: wrong state %d\n", state);
            return;
        }
        do_cont();
    }
    else if(!strncmp(cmd, "delete", 6)) {
        if(state != RUN) return;
        char *tmp = strtok(cmd, " ");
        tmp = strtok(NULL, " ");
        if(tmp == NULL) {
            printf("** no id is given.\n");
            return;
        }
        do_delete(atoi(tmp));
    }
    else if(!strncmp(cmd, "dump", 4) || !strncmp(cmd, "x", 1)) {
        if(state != RUN) return;
        char *addr = strtok(cmd, " ");
        addr = strtok(NULL, " ");
        if(addr == NULL) {
            printf("** no address is given\n");
            return;
        }
        do_dump(strtoul(addr, 0, 16));
    }
    else if(!strncmp(cmd, "disasm", 6) || !strncmp(cmd, "d", 1)) {
        if(state != RUN) {
            printf("** disasm: wrong state %d\n", state);
            return;
        }
        char *tmp = strtok(cmd, " ");
        tmp = strtok(NULL, " ");
        if(tmp == NULL) {
            printf("** no address is given\n");
            return;
        }
        unsigned long long addr = strtoul(tmp, NULL, 16);
        do_disasm(addr);
    }
    else if(!strncmp(cmd, "exit", 4) || !strcmp(cmd, "q")) {
        if(state == RUN) {
            if(ptrace(PTRACE_KILL, pid, 0, 0) < 0) errquit("** ptrace(KILL)");
        }
        state = EXIT;
    }
    else if(!strncmp(cmd, "getregs", 7)) {
        if(state != RUN) return;
        do_getregs();
    }
    else if(!strncmp(cmd, "get", 3) || !strncmp(cmd, "g", 1)) {
        if(state != RUN) return;
        char *reg = strtok(cmd, " ");
        reg = strtok(NULL, " ");
        if(reg == NULL) {
            printf("** no reg is given\n");
            return;
        }
        int32_t ret = *((int32_t *)((char *)&regs + user_regs_lookup(reg)));
        printf("%-4s = %d (0x%x)\n", reg, ret, ret);
    }
    else if(!strncmp(cmd, "help", 4) || !strcmp(cmd, "h")) {
        do_help();
    }
    else if(!strncmp(cmd, "list", 4) || !strcmp(cmd, "l")) {
        do_list();
    }
    else if(!strncmp(cmd, "load", 4)) {
        if(state != NLOADED) return;
        do_load(cmd+5);
    }
    else if(!strncmp(cmd, "run", 3) || !strcmp(cmd, "r")) {
        if(state != LOADED && state != RUN) return;
        do_run();
    }
    else if(!strncmp(cmd, "vmmap", 5) || !strcmp(cmd, "m")) {
        if(state != RUN) return;
        do_vmmap();
    }
    else if(!strncmp(cmd, "si", 2)) {
        if(state != RUN) return;
        do_step();
    }
    else if(!strncmp(cmd, "start", 5)) {
        if(state != LOADED) return;
        do_start();
    }
    else if(!strncmp(cmd, "set", 3) || !strncmp(cmd, "s", 1)) {
        if(state != RUN) return;
        char *reg;
        char *value;
        reg = strtok(cmd, " ");
        reg = strtok(NULL, " ");
        if(reg == NULL) {
            printf("** No reg is given\n");
            return;
        }
        value = strtok(NULL, " ");
        if(value == NULL) {
            printf("** No value is given\n");
            return;
        }
        do_set(reg, strtoul(value, 0, 16));
    }
}

int main(int argc, char *argv[]) {
    state = NLOADED;
    char cmd[MAX_CMD];
    FILE *rf = stdin;
    setvbuf(stdout, NULL, _IONBF, 0);
    // Not loaded state
    if(argc > 1) {
        int i = 1;
        while(i < argc) {
            if(!strcmp(argv[i], "-s")) rf = fopen(argv[i+1], "rb");
            else do_load(argv[i]);
            i += 2;
        }
    }

    // Other states
    int status;
    while(state != EXIT) {
        if(state == WAIT) {
            if(waitpid(pid, &status, 0) < 0) errquit("** waitpid");
            if(WIFEXITED(status)) {
                printf("** child process %d terminiated normally (code %d)\n", pid, status);
                state = LOADED;
            }
            else if(WIFSTOPPED(status)){
                if(ptrace(PTRACE_GETREGS, pid, NULL, &regs) < 0) errquit("** ptrace(GETREGS)");
                state = RUN;
                if(WSTOPSIG(status) == SIGTRAP && breakpoints.size() != 0) {
                    if(breakpoints.find(regs.rip-1) != breakpoints.end()) {
                        hit_break(regs.rip-1);
                    }
                    else {
                        printf("** Miss, fail to hit, rip:0x%llx\n", regs.rip);
                        return -1;
                    }
                }
            }
        }
        else {
            if(rf == stdin) printf("sdb> ");
            if(fgets(cmd, MAX_CMD, rf)) {
                cmd[strcspn(cmd, "\n")] = 0;
                run_cmd(cmd);
            }
            else state = EXIT;
        }
    }
    return 0;
}