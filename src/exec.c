#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "exec.h"

int exec_run(char *const argv[])
{
	pid_t pid;
	int status;

	if (!argv || !argv[0])
		return -1;

	pid = fork();
	if (pid < 0)
		return -1;
	if (pid == 0) {
		execvp(argv[0], argv);
		_exit(127);
	}
	if (waitpid(pid, &status, 0) < 0)
		return -1;
	if (WIFEXITED(status))
		return WEXITSTATUS(status);
	return -1;
}

int exec_capture(char *const argv[], char *buf, size_t buflen)
{
	int pipefd[2];
	pid_t pid;
	int status;
	ssize_t n, total = 0;

	if (!argv || !argv[0] || !buf || buflen == 0)
		return -1;
	buf[0] = '\0';

	if (pipe(pipefd) < 0)
		return -1;

	pid = fork();
	if (pid < 0) {
		close(pipefd[0]);
		close(pipefd[1]);
		return -1;
	}
	if (pid == 0) {
		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		dup2(pipefd[1], STDERR_FILENO);
		close(pipefd[1]);
		execvp(argv[0], argv);
		_exit(127);
	}
	close(pipefd[1]);
	while ((n = read(pipefd[0], buf + total, buflen - 1 - (size_t)total)) > 0) {
		total += n;
		if ((size_t)total >= buflen - 1)
			break;
	}
	buf[total] = '\0';
	close(pipefd[0]);
	if (waitpid(pid, &status, 0) < 0)
		return -1;
	if (WIFEXITED(status))
		return WEXITSTATUS(status);
	return -1;
}

int exec_shell(const char *cmd)
{
	char *argv[] = { "/bin/sh", "-c", (char *)cmd, NULL };
	return exec_run(argv);
}
