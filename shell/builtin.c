#include "builtin.h"

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	return strncmp(cmd, "exit", 4) == 0;
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	if (strncmp(cmd, "cd", 2) == 0) {
		cmd = cmd + 3;
		char buf[BUFLEN] = { 0 };
		if (strlen(cmd) == 0) {
			char *home = getenv("HOME");

			if (chdir(home) < 0) {
				snprintf(buf, sizeof buf, "cannot cd to %s ", home);
				perror(buf);
			} else {
				snprintf(prompt, sizeof prompt, "(%s)", home);
			}
		} else {
			if (chdir(cmd) < 0) {
				snprintf(buf, sizeof buf, "cannot cd to %s ", cmd);
				perror(buf);
			} else {
				if (!getcwd(prompt, sizeof(prompt))) {
					perror("Prompt error");
				}
			}
		}
		return 1;
	}
	return 0;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (strncmp(cmd, "pwd", 3) == 0) {
		char buf[BUFLEN] = { 0 };
		if (!getcwd(buf, sizeof(buf))) {
			perror("Error getting current working directory");
		} else {
			printf("%s\n", buf);
		}
		return true;
	}
	return false;
}

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd)
{
	// Your code here

	return 0;
}
