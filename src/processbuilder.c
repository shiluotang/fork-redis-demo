#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#if defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
#   include <sys/types.h>
#   include <sys/wait.h>
#   include <unistd.h>
#endif

#include "processbuilder.h"

static
int Process_getPID(Process const *me) {
    return me->data._M_pid;
}

static
Process* Process_setPID(Process *me, int value) {
    me->data._M_pid = value;
    return me;
}

static
int Process_kill0(Process *me, int sig) {
    int rc = 0;

    if (me->data._M_pid < 0)
        goto success;

    if (kill((pid_t) me->data._M_pid, sig) == -1)
        goto failure;
    me->data._M_pid = -1;

    goto success;
exit:
    return rc;
success:
    rc = 1;
    goto cleanup;
failure:
    rc = 0;
    goto cleanup;
cleanup:
    goto exit;
}

static
int Process_kill(Process *me) {
    return Process_kill0(me, SIGKILL);
}

static
int Process_wait0(Process *me, int options) {
    int rc = 0;
    int stat_loc = 0;
    pid_t pid = -1;

    if (me->data._M_pid < 0)
        goto success;

    do {
        pid = waitpid((pid_t) me->data._M_pid, &stat_loc, options);
        if ((int) pid == -1) {
            perror("waitpid");
            goto failure;
        }
    } while (!WIFEXITED(stat_loc) && !WIFSIGNALED(stat_loc));
#if 0
    fprintf(stderr, "WIFEXITED(stat_loc) = %d\n", WIFEXITED(stat_loc));
    if (WIFEXITED(stat_loc)) {
        WEXITSTATUS(stat_loc);
        fprintf(stderr, "exit status = %d\n", (int) WEXITSTATUS(stat_loc));
    }
    fprintf(stderr, "WIFSIGNALED(stat_loc) = %d\n", WIFSIGNALED(stat_loc));
    fprintf(stderr, "waitpid(pid = %d, stat_loc = %p (%d), options = %d) = %d\n",
            me->data._M_pid,
            &stat_loc, stat_loc,
            options,
            pid
            );
#endif
    me->data._M_pid = -1;

    goto success;
exit:
    return rc;
success:
    rc = 1;
    goto cleanup;
failure:
    rc = 0;
    goto cleanup;
cleanup:
    goto exit;
}

static
int Process_wait(Process *me) {
    return Process_wait0(me, WNOHANG);
}

void Process_destroy(Process *me) {
    if (me) {
        me->calls.kill(me);
        me->calls.wait(me);
        free(me);
        me = NULL;
    }
}

Process* Process_create() {
    Process *r = NULL;
    Process *instance = NULL;

    instance = (Process*) calloc(1, sizeof(*instance));
    if (!instance)
        goto failure;

    instance->calls.getPID = &Process_getPID;
    instance->calls.setPID = &Process_setPID;
    instance->calls.kill0 = &Process_kill0;
    instance->calls.kill = &Process_kill;
    instance->calls.wait0 = &Process_wait0;
    instance->calls.wait = &Process_wait;

    goto success;
exit:
    return r;
success:
    r = instance;
    instance = NULL;
    goto cleanup;
failure:
    goto cleanup;
cleanup:
    if (instance) {
        Process_destroy(instance);
        instance = NULL;
    }
    goto exit;
}

static
char const* ProcessBuilder_getPath(ProcessBuilder const *me) {
    return me->data._M_path;
}

static
char const* ProcessBuilder_getFile(ProcessBuilder const *me) {
    return me->data._M_file;
}

static
char const** ProcessBuilder_getArguments(ProcessBuilder const *me) {
    return (char const**) me->data._M_arguments;
}

static
char const** ProcessBuilder_getEnvironments(ProcessBuilder const *me) {
    return (char const**) me->data._M_environments;
}

static
ProcessBuilder* ProcessBuilder_setPath(ProcessBuilder *me, char const *value) {
    ProcessBuilder *r = NULL;
    char *p = NULL;
    size_t len = strlen(value);

    if (!value)
        goto failure;

    p = (char*) realloc(me->data._M_path, len + 1);
    if (!p)
        goto failure;
    /* require one more to make NUL terminated */
    strncpy(p, value, len + 1);
    me->data._M_path = p;
    p = NULL;

    goto success;
exit:
    return r;
success:
    r = me;
    goto cleanup;
failure:
    goto cleanup;
cleanup:
    if (p) {
        free(p);
        p = NULL;
    }
    goto exit;
}

static
ProcessBuilder* ProcessBuilder_setFile(ProcessBuilder *me, char const *value) {
    ProcessBuilder *r = NULL;
    char *p = NULL;
    size_t len = strlen(value);

    if (!value)
        goto failure;

    p = (char*) realloc(me->data._M_file, len + 1);
    if (!p)
        goto failure;
    /* require one more to make NUL terminated */
    strncpy(p, value, len + 1);
    me->data._M_file = p;
    p = NULL;

    goto success;
exit:
    return r;
success:
    r = me;
    goto cleanup;
failure:
    goto cleanup;
cleanup:
    if (p) {
        free(p);
        p = NULL;
    }
    goto exit;
}

static
size_t ProcessBuilder_countof(char const **values) {
    size_t n = 0;
    if (values) {
        for (; *values; ++values)
            ++n;
    }
    return n;
}

static
void ProcessBuilder_freeValues(char **values) {
    char **p = NULL;
    for (p = values; *p; ++p)
        free((void*) *p);
    free(values);
}

size_t ProcessBuilder_getArgumentsCount(ProcessBuilder const *me) {
    return ProcessBuilder_countof((char const**) me->data._M_arguments);
}

size_t ProcessBuilder_getEnvironmentsCount(ProcessBuilder const *me) {
    return ProcessBuilder_countof((char const**) me->data._M_environments);
}

static
ProcessBuilder* ProcessBuilder_setArguments(ProcessBuilder *me, char const **values) {
    ProcessBuilder *r = NULL;
    size_t i = 0;
    size_t n = 0;
    size_t len = 0;

    size_t count = 0;
    char **args = NULL;
    char *val = NULL;

    count = ProcessBuilder_countof(values);

    /* FIXME TODO Should the first element be empty? */
    args = (char**) realloc(me->data._M_arguments, sizeof(*args) * (count + 1));
    if (!args)
        goto failure;
    me->data._M_arguments = NULL;
    memset(args, 0, sizeof(*args) * (count + 1));

    for (i = 0, n = count; i < n; ++i) {
        len = strlen(values[i]);
        val = (char*) realloc(args[i], len + 1);
        if (!val)
            goto failure;
        strncpy(val, values[i], len + 1);
        args[i] = val;
        val = NULL;
    }

    me->data._M_arguments = args;
    args = NULL;

    goto success;
exit:
    return r;
success:
    r = me;
    goto cleanup;
failure:
    goto cleanup;
cleanup:
    if (args) {
        ProcessBuilder_freeValues(args);
        args = NULL;
    }
    goto exit;
}

static
ProcessBuilder* ProcessBuilder_setEnvironments(ProcessBuilder *me, char const **values) {
    ProcessBuilder *r = NULL;
    size_t i = 0;
    size_t n = 0;
    size_t len = 0;

    size_t count = 0;
    char **args = NULL;
    char *val = NULL;

    count = ProcessBuilder_countof(values);

    /* FIXME TODO Should the first element be empty? */
    args = (char**) realloc(me->data._M_environments, sizeof(char*) * count + 1);
    if (!args)
        goto failure;
    memset(args, 0, sizeof(*args) * (count + 1));

    for (i = 0, n = count; i < n; ++i) {
        len = strlen(values[i]);
        val = (char*) realloc(args[i], len + 1);
        if (!val)
            goto failure;
        strncpy(val, values[i], len + 1);
        args[i] = val;
        val = NULL;
    }

    me->data._M_environments = args;
    args = NULL;

    goto success;
exit:
    return r;
success:
    r = me;
    goto cleanup;
failure:
    goto cleanup;
cleanup:
    if (args) {
        ProcessBuilder_freeValues(args);
        args = NULL;
    }
    goto exit;
}

static
pid_t ProcessBuilder_runProcess(char const *pwd, char **args, char **envs) {
    pid_t pid = -1;
    int exitcode = 0;

    /* TODO copy all the values */
    pid = fork();
    if ((int) pid == 0) {
        /* running in child process */
        if (pwd) {
            if (chdir(pwd) != 0) {
                /* FIXME TODO Notify parent process chdir failed */
                perror("chdir");
                goto failure;
            }
        }
        if (execve(args[0], args, envs) == -1) {
            perror("execve");
            goto failure;
        }
    } else if (pid == -1) {
        perror("fork");
        goto failure;
    }

    goto success;
exit:
    if (pid == 0)
        _exit(exitcode);
    return pid;
success:
    goto cleanup;
failure:
    /* _exit(0) in child process? */
    if (pid == 0) {
        fprintf(stderr, "exit from child process with status code 1\n");
        exitcode = 1;
    }
    goto cleanup;
cleanup:
    goto exit;
}

static
Process* ProcessBuilder_build(ProcessBuilder const *me) {
    Process *r = NULL;
    Process *process = NULL;
    pid_t pid = -1;
    int nargs = 0;
    int size = 0;
    char **args = NULL;
    char **src = NULL;
    char **dest = NULL;

    if (!me->data._M_file || strlen(me->data._M_file) < 1)
        goto failure;

    nargs = ProcessBuilder_countof((char const**) me->data._M_arguments);

    /* the first one for executable file, the last one for end mark (NULL) */
    args = (char**) realloc(NULL, (nargs + 2) * sizeof(*args));
    if (!args) {
        perror("realloc");
        goto failure;
    }

    memset(args, 0, (nargs + 2) * sizeof(*args));
    src = me->data._M_arguments;
    dest = args;
    size = nargs;

    *dest++ = me->data._M_file;
    /* copy pointers */
    if (src)
        memcpy(dest, src, (size + 1) * sizeof(src));

    pid = ProcessBuilder_runProcess(me->data._M_path, args,
            me->data._M_environments);

    if (pid == -1)
        goto failure;
    process = Process_create();
    if (!process)
        goto failure;
    process->calls.setPID(process, (int) pid);

    goto success;
exit:
    return r;
success:
    r = process;
    process = NULL;
    goto cleanup;
failure:
    goto cleanup;
cleanup:
    if (process) {
        Process_destroy(process);
        process = NULL;
    }
    goto exit;
}

void ProcessBuilder_destroy(ProcessBuilder *me) {
    if (me) {
        if (me->data._M_path) {
            free(me->data._M_path);
            me->data._M_path = NULL;
        }
        if (me->data._M_file) {
            free(me->data._M_file);
            me->data._M_file = NULL;
        }
        if (me->data._M_arguments) {
            ProcessBuilder_freeValues(me->data._M_arguments);
            me->data._M_arguments = NULL;
        }
        if (me->data._M_environments) {
            ProcessBuilder_freeValues(me->data._M_environments);
            me->data._M_environments = NULL;
        }
        free(me);
    }
}

ProcessBuilder* ProcessBuilder_create() {
    ProcessBuilder *r = NULL;
    ProcessBuilder *builder = NULL;

    builder = (ProcessBuilder*) calloc(1, sizeof(*builder));
    if (!builder)
        goto failure;

    builder->calls.getPath = &ProcessBuilder_getPath;
    builder->calls.getFile = &ProcessBuilder_getFile;
    builder->calls.getArguments = &ProcessBuilder_getArguments;
    builder->calls.getEnvironments = &ProcessBuilder_getEnvironments;

    builder->calls.setPath = &ProcessBuilder_setPath;
    builder->calls.setFile = &ProcessBuilder_setFile;
    builder->calls.setArguments = &ProcessBuilder_setArguments;
    builder->calls.setEnvironments = &ProcessBuilder_setEnvironments;
    builder->calls.build = &ProcessBuilder_build;

    goto success;
exit:
    return r;
success:
    r = builder;
    builder = NULL;
    goto cleanup;
failure:
    goto cleanup;
cleanup:
    if (builder) {
        ProcessBuilder_destroy(builder);
        builder = NULL;
    }
    goto exit;
}
