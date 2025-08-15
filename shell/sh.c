#include "defs.h"
#include "types.h"
#include "readline.h"
#include "runcmd.h"

char prompt[PRMTLEN] = { 0 };

// runs a shell command
static void
run_shell()
{
	char *cmd;

	while ((cmd = read_line(prompt)) != NULL)
		if (run_cmd(cmd) == EXIT_SHELL)
			return;
}

static void
sigchild_handler()
{
	pid_t pid;
	int status;

	while ((pid = waitpid(0, &status, WNOHANG)) > 0) {
		printf_debug("Terminado: PID:%d\n", pid);
	}
}

// initializes the shell
// with the "HOME" directory
static void
init_shell()
{
	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		snprintf(prompt, sizeof prompt, "(%s)", home);
	}

	setpgid(0, 0);

	struct sigaction sa;
	sa.sa_handler = sigchild_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_ONSTACK;
	sigaltstack(NULL, NULL);
	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}
}

int
main(void)
{
	init_shell();

	run_shell();

	return 0;
}