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
struct tagProcessBuilder;
struct tagProcess;

typedef struct tagRedisInstance RedisInstance;
typedef struct tagRedisBuilder RedisBuilder;
typedef struct tagProcessBuilder ProcessBuilder;
typedef struct tagProcess Process;

struct tagProcess {
    struct {
        int     (*wait) (Process const*, int);
        int     (*kill) (Process const*, int);
        int     (*getPID) (Process const*);
        int     (*setPID) (Process*, int);
    } calls;

    struct {
        int _M_pid;
    } data;
};

struct tagProcessBuilder {
    struct {
        ProcessBuilder* (*setPath) (ProcessBuilder*, char const*);
        char const*     (*getPath) (ProcessBuilder const*);

        ProcessBuilder* (*setFile) (ProcessBuilder*, char const*);
        char const*     (*getFile) (ProcessBuilder const*);

        ProcessBuilder* (*setArguments) (ProcessBuilder*, char const**arguments);
        char const**    (*getArguments) (ProcessBuilder const*);

        ProcessBuilder* (*setEnvironments) (ProcessBuilder*, char const**arguments);
        char const**    (*getEnvironments) (ProcessBuilder const*);

        Process*        (*build)();
    } calls;
    struct {
        char* _M_path;
        char* _M_file;
        char* *_M_arguments;
        char* *_M_environments;
    } data;
};

char const* ProcessBuilder_getPath(ProcessBuilder const *me) {
    return me->data._M_path;
}

char const* ProcessBuilder_getFile(ProcessBuilder const *me) {
    return me->data._M_file;
}

char const** ProcessBuilder_getArguments(ProcessBuilder const *me) {
    return (char const**) me->data._M_arguments;
}

char const** ProcessBuilder_getEnvironments(ProcessBuilder const *me) {
    return (char const**) me->data._M_environments;
}

void ProcessBuilder_destroy(ProcessBuilder *builder) {
    if (builder) {
        free(builder);
    }
}

ProcessBuilder* ProcessBuilder_create() {
    ProcessBuilder *r = NULL;
    ProcessBuilder *builder = NULL;

    builder = (ProcessBuilder*) calloc(1, sizeof(*builder));
    if (!builder)
        goto failure;
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
        me->data._M_cfg = p;
        me->data._M_capacity = capacity;
        strncat(me->data._M_cfg, &buffer[0], c);
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
    uid_t uid;
    gid_t gid;

    gid = getgid();
    uid = getuid();
    pid = fork();

    if ((int) pid == 0) {
        if (setgid(gid) == -1) {
            perror("setgid failed");
            goto failure;
        }
        if (setuid(uid) == -1) {
            perror("setuid failed");
            goto failure;
        }
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

int main(int argc, char* *argv) {
    int rc = 0;
    pid_t pid = -1;
    int stat_loc = 0;
    char *arguments[] = { "/usr/local/bin/redis-server", "--port 8888", NULL };
    char *environments[] = { NULL };

    RedisBuilder *builder = RedisBuilder_create();
    if (!builder)
        goto failure;
    builder->calls.optionNumber(builder, "port", 8888);

    pid = runChild(arguments[0], arguments, environments);
    fprintf(stderr, "pid = %d\n", pid);
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
    goto exit;
}
