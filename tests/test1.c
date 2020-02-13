#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <hiredis/hiredis.h>

#if defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
#   include <sys/types.h>
#   include <sys/wait.h>
#   include <unistd.h>
#endif

struct tagRedisInstance;
struct tagRedisBuilder;

struct tagProcess;
struct tagProcessBuilder;

typedef struct tagRedisInstance RedisInstance;
typedef struct tagRedisBuilder RedisBuilder;

typedef struct tagProcess Process;
typedef struct tagProcessBuilder ProcessBuilder;

struct tagProcess {
    struct {
        int         (*wait0)    (Process*, int);
        int         (*wait)     (Process*);
        int         (*kill0)    (Process*, int);
        int         (*kill)     (Process*);
        int         (*getPID)   (Process const*);
        Process*    (*setPID)   (Process*, int);
    } calls;

    struct {
        int _M_pid;
    } data;
};

struct tagProcessBuilder {
    struct {
        ProcessBuilder* (*setPath)          (ProcessBuilder*, char const*);
        char const*     (*getPath)          (ProcessBuilder const*);

        ProcessBuilder* (*setFile)          (ProcessBuilder*, char const*);
        char const*     (*getFile)          (ProcessBuilder const*);

        ProcessBuilder* (*setArguments)     (ProcessBuilder*, char const**arguments);
        char const**    (*getArguments)     (ProcessBuilder const*);

        ProcessBuilder* (*setEnvironments)  (ProcessBuilder*, char const**arguments);
        char const**    (*getEnvironments)  (ProcessBuilder const*);

        Process*        (*build)            (ProcessBuilder const*);
    } calls;
    struct {
        char* _M_path;
        char* _M_file;
        char* *_M_arguments;
        char* *_M_environments;
    } data;
};

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
        if ((int) pid == -1)
            goto failure;
    } while (!WIFEXITED(stat_loc) && !WIFSIGNALED(stat_loc));
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
    if (!values)
        return n;
    for (char const **p = values; *p; ++p)
        ++n;
    return n;
}

static
void ProcessBuilder_freeValues(char **values) {
    for (char **p = values; *p; ++p)
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

    /* TODO copy all the values */
    pid = fork();
    if ((int) pid == 0) {
        /* running in child process */
        if (pwd) {
            if (chdir(pwd) != 0) {
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
    return pid;
success:
    goto cleanup;
failure:
    /* _exit(0) in child process? */
    if (pid == 0) {
        fprintf(stderr, "exit from child process with status code 1\n");
        _exit(1);
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

struct tagRedisInstance {
};

struct tagRedisBuilder {
    struct {
        RedisInstance*  (*build)(RedisBuilder const*);
        RedisInstance*  (*build0)(RedisBuilder const*, int port);
        RedisBuilder*   (*optionString)(RedisBuilder*, char const *name, char const *value);
        RedisBuilder*   (*optionNumber)(RedisBuilder*, char const *name, long value);
        char const*     (*getParameters)(RedisBuilder const*);
    } calls;

    struct {
        char *_M_cfg;
        size_t _M_size;
        size_t _M_capacity;
    } data;
};



void RedisBuilder_locateExecutable(RedisBuilder const* me) { }

RedisInstance* RedisBuilder_build0(RedisBuilder const *me, int port) {
    RedisInstance *instance = NULL;
    return instance;
}

RedisBuilder* RedisBuilder_optionString(RedisBuilder *me,
        char const *name, char const *value) {
    char buffer[1024];
    size_t required_size = 0;
    char *p = NULL;
    int c = 0;
    size_t capacity = 0;
    RedisBuilder *r = NULL;

    fprintf(stdout, "DEBUG [%s]=[%s]\n", name, value);
    /* TODO append option string */
    c = snprintf(&buffer[0], sizeof(buffer), "\n--%s %s", name, value);
    if (c < 0) {
        /* impossbile */
        fprintf(stderr, "snprintf failed!");
        goto failure;
    } else if (c >= sizeof(buffer)) {
        /* truncated */
        fprintf(stderr, "snprintf truncated %s => %s\n", value, &buffer[0]);
        goto failure;
    }
    required_size = me->data._M_capacity;
    if (!me->data._M_cfg
            || me->data._M_capacity - me->data._M_size < required_size) {
        capacity = me->data._M_capacity == 0 ? 16 : me->data._M_capacity * 2;
        p = (char*) realloc(me->data._M_cfg, capacity);
        if (!p) {
            /* memory allocation failed */
            fprintf(stderr, "realloc failed!\n");
            goto failure;
        }
        strncat(p, &buffer[0], c);
        me->data._M_cfg = p;
        me->data._M_capacity = capacity;
        me->data._M_size += c;
    }

    goto success;
exit:
    return r;
success:
    r = me;
    goto cleanup;
failure:
    r = NULL;
    goto cleanup;
cleanup:
    goto exit;
}

RedisBuilder* RedisBuilder_optionNumber(RedisBuilder *me,
        char const *name, long value) {
    int c = 0;
    char buffer[0xff];
    memset(&buffer[0], 0, sizeof(buffer));
    c = snprintf(&buffer[0], sizeof(buffer), "%ld", value);
    if (c >= sizeof(buffer)) {
        /* truncated is nearly impossible due to buffer size */
        fprintf(stderr, "truncated option value for long %ld => %s\n", value, &buffer[0]);
    } else if (c < 0) {
        /* failed */
        return NULL;
    }
    return me->calls.optionString(me, name, &buffer[0]);
}

char const* RedisBuilder_getParameters(RedisBuilder const *me) {
    return me->data._M_cfg;
}


RedisBuilder* RedisBuilder_create() {
    RedisBuilder *instance = (RedisBuilder*) calloc(1, sizeof(*instance));
    instance->calls.build0 = &RedisBuilder_build0;
    instance->calls.optionString = &RedisBuilder_optionString;
    instance->calls.optionNumber = &RedisBuilder_optionNumber;
    instance->calls.getParameters = &RedisBuilder_getParameters;
    return instance;
}

void RedisBuilder_destroy(RedisBuilder *builder) {
    if (builder) {
        if (builder->data._M_cfg) {
            free(builder->data._M_cfg);
            builder->data._M_size = 0;
            builder->data._M_capacity = 0;
            builder->data._M_cfg = NULL;
        }
        free(builder);
        /* Nonsense assignment */
        builder = NULL;
    }
}

pid_t runChild(char const *path, char **args, char **env) {
    pid_t pid;

#ifdef COPY_GID_UID
    uid_t uid;
    gid_t gid;

    gid = getgid();
    uid = getuid();
#endif
    pid = fork();

    if ((int) pid == 0) {
#ifdef COPY_GID_UID
        if (setgid(gid) == -1) {
            perror("setgid failed");
            goto failure;
        }
        if (setuid(uid) == -1) {
            perror("setuid failed");
            goto failure;
        }
#endif
        if (execve(path, args, env) == -1) {
            perror("execve failed");
            goto failure;
        }
        /* running in child process */
    } else if ((int) pid == -1) {
        /* no child process created */
    }

    goto success;
exit:
    return pid;
success:
    goto cleanup;
failure:
    _exit(0);
    goto cleanup;
cleanup:
    goto exit;
}

#ifndef ENUM_BEGIN
#   define ENUM_BEGIN(name, type) \
    static char const *get##name##String(type value) { \
        char const *s = "<UNKNOWN>"; \
        switch (value) {
#   define ENUM_ITEM(item) \
            case item: s = #item; break;
#   define ENUM_END \
            default: break; \
        } \
        return s; \
    }
#endif

ENUM_BEGIN(RedisReplyType, int)
    ENUM_ITEM(REDIS_REPLY_STRING)
    ENUM_ITEM(REDIS_REPLY_ARRAY)
    ENUM_ITEM(REDIS_REPLY_INTEGER)
    ENUM_ITEM(REDIS_REPLY_NIL)
    ENUM_ITEM(REDIS_REPLY_STATUS)
    ENUM_ITEM(REDIS_REPLY_ERROR)
ENUM_END

static
int check_redis_available(char const *ip, int port) {
    int rc = 0;
    redisContext *ctx = NULL;
    redisReply *reply = NULL;
    struct timeval tv;

    tv.tv_sec = 2;

    ctx = redisConnectWithTimeout("localhost", 8888, tv);
    if (!ctx)
        goto failure;
    if (ctx->err != REDIS_OK) {
        fprintf(stderr, "[redis] %s\n", &ctx->errstr[0]);
        goto failure;
    }
    *(void**) &reply = redisCommand(ctx, "SET %s %s", "a", "a");
    reply->type = REDIS_REPLY_STRING;
    fprintf(stderr, "[redis] reply = %s (%s)\n",
            getRedisReplyTypeString(reply->type),
            reply->str);
    freeReplyObject(reply); reply = NULL;

    *(void**) &reply = redisCommand(ctx, "GET %s", "a");
    fprintf(stderr, "[redis] reply = %s (%s)\n",
            getRedisReplyTypeString(reply->type),
            reply->str);
    freeReplyObject(reply); reply = NULL;

    *(void**) &reply = redisCommand(ctx, "SAVE");
    fprintf(stderr, "[redis] reply = %s (%s)\n",
            getRedisReplyTypeString(reply->type),
            reply->str);
    freeReplyObject(reply); reply = NULL;

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
    if (reply) {
        freeReplyObject(reply);
        reply = NULL;
    }
    if (ctx) {
        redisFree(ctx);
        ctx = NULL;
    }
    goto exit;
}

int main2(int argc, char* *argv) {
    int rc = 0;
    pid_t pid = -1;
    int stat_loc = 0;
    char* arguments[] = { "/usr/local/bin/redis-server", "--port 8888", NULL };
    char* environments[] = { NULL };
    ProcessBuilder *pb = NULL;
    RedisBuilder *builder = NULL;

    pb = ProcessBuilder_create();
    if (!pb)
        goto failure;

    pb->calls.setPath(pb, "Yes, I'm Jimmy!");
    printf("%s\n", pb->calls.getPath(pb));
    pb->calls.setArguments(pb, (char const**) arguments);

    builder = RedisBuilder_create();
    if (!builder)
        goto failure;
    builder->calls.optionNumber(builder, "port", 8888);

    pid = runChild(arguments[0], arguments, environments);
    fprintf(stderr, "pid = %d\n", pid);
    /* wait for redis startup complete */
    sleep(1);
    if (!check_redis_available("localhost", 8888))
        goto failure;

    goto success;
exit:
    return rc;
success:
    rc = EXIT_SUCCESS;
    goto cleanup;
failure:
    rc = EXIT_FAILURE;
    goto cleanup;
cleanup:
    if (pid > 0) {
        fprintf(stderr, "kill start...\n");
        if (kill(pid, SIGKILL) == -1) {
            perror("kill failed");
        }
        fprintf(stderr, "kill stopped.\n");
        fprintf(stderr, "waitpid start...\n");
        fflush(stderr);
        waitpid(pid, &stat_loc, WNOHANG);
        fprintf(stderr, "waitpid stopped.\n");
    }
    if (builder) {
        RedisBuilder_destroy(builder);
        builder = NULL;
    }
    if (pb) {
        ProcessBuilder_destroy(pb);
        pb = NULL;
    }
    goto exit;
}

int main(int argc, char* *argv) {
    int rc = 0;
    char* arguments[] = { "--port 8888", "--dbfilename a.rdb", NULL };
    ProcessBuilder *pb = NULL;
    Process *process = NULL;

    pb = ProcessBuilder_create();
    if (!pb)
        goto failure;

    printf("%s\n", pb->calls.getPath(pb));
    pb->calls.setFile(pb, "/usr/local/bin/redis-server");
    pb->calls.setArguments(pb, (char const**) arguments);
    pb->calls.setPath(pb, "/Users/shengquangang");
    process = pb->calls.build(pb);
    if (!process)
        goto failure;
    sleep(1);
    check_redis_available("localhost", 8888);
    sleep(1);
    process->calls.kill0(process, SIGTERM);
    sleep(1);

    goto success;
exit:
    return rc;
success:
    rc = EXIT_SUCCESS;
    goto cleanup;
failure:
    rc = EXIT_FAILURE;
    goto cleanup;
cleanup:
    if (process) {
        Process_destroy(process);
        process = NULL;
    }
    if (pb) {
        ProcessBuilder_destroy(pb);
        pb = NULL;
    }
    goto exit;
}
