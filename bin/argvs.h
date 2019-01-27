#ifndef __ARGVS__
#define __ARGVS__

/********/
#define UNUSED       __attribute__((__unused__))
#define COMMAND_ARGS int argc UNUSED, char **argv UNUSED, void *userdata
/*******/

void argvs_add   (int (*fun)(COMMAND_ARGS),
                  const char *name,
                  int         required_arguments,
                  const char *argument_help,
                  const char *help);
int argvs_eval_argv (char **cargv, int cargc);
int argvs_eval   (const char *cmdline); /* this evals one command,
                                           no newlines or semicolons are taken into account. */
int argvs_source (const char *path);
int argvs_command_exist (const char *command);
void argvs_register (void);

#endif
