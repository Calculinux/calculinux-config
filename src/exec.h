#ifndef EXEC_H
#define EXEC_H

#include <stddef.h>

/* Run argv (NULL-terminated) with fork/exec/wait. Returns exit status,
 * or -1 on fork/exec failure. */
int exec_run(char *const argv[]);

/* Like exec_run but captures stdout into buf (truncated). */
int exec_capture(char *const argv[], char *buf, size_t buflen);

/* Run a shell command string via /bin/sh -c. */
int exec_shell(const char *cmd);

#endif /* EXEC_H */
