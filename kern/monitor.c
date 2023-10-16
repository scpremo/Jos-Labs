// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};
// struct Eipdebuginfo{
// 	char *eip_file;
// 	int eip_line;
// 	char *eip_fn_name;
// 	int eip_fn_nameln;
// 	uintptr_t eip_fn_addr;
// 	int eip_fn_narg;
// };

// LAB 1: add your command to here...
static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{"backtrace","dispaly the backtrace of the monitor", mon_backtrace},
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	int *ebp = (int*)read_ebp(); //who could've guessed that the hint was helpful
	struct Eipdebuginfo info;
    cprintf("Stack backtrace:\n");
    while (ebp != 0) {//0 is when the back trace is over
        int eip = ebp[1]; // Return address
        cprintf(" ebp %x eip %x args %08x %08x %08x %08x %08x\n", ebp, eip, ebp[2], ebp[3], ebp[4], ebp[5], ebp[6]);
		//struct Eipdebuginfo *info = kmalloc(sizeof(struct Eipdebuginfo));//didnt need to init it as it is init in the function call
		int test = debuginfo_eip(eip, &info);
		if (test==0){
			char* funname = (char*)info.eip_fn_name;
			if (strlen(funname) >= strlen(":F(0,18)") && strcmp(funname + strlen(funname) - strlen(":F(0,18)"), ":F(0,18)") == 0)
				funname[strlen(funname) - strlen(":F(0,18)")] = '\0';//remove the F(0,18) part of the strings that arent supposed to be displayed
			cprintf("    %s:%d: %s+%d\n", info.eip_file, info.eip_line, funname, eip - info.eip_fn_addr);
		}
        ebp = (int*)ebp[0]; // Move to the next part of the stack
    }
	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");
	// cprintf("x=%d y=%d", 3);




	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
