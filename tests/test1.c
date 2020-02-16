#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include <hiredis/hiredis.h>

#include "../src/processbuilder.h"
#include "../src/redisserverbuilder.h"

#if defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
#   include <sys/types.h>
#   include <unistd.h>
#endif

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
    tv.tv_usec = 0;

    ctx = redisConnectWithTimeout("localhost", port, tv);
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
    int exitcode = 0;
    char* arguments[] = { "--port 8888", NULL };
    ProcessBuilder *pb = NULL;
    Process *process = NULL;

    pb = ProcessBuilder_create();
    if (!pb)
        goto failure;

    /* printf("%s\n", pb->calls.getPath(pb)); */
    pb->calls.setFile(pb, "/usr/bin/redis-server");
    pb->calls.setArguments(pb, (char const**) arguments);
    pb->calls.setPath(pb, getenv("HOME"));
    process = pb->calls.build(pb);
    if (!process)
        goto failure;
    sleep(1);
    if (process->calls.wait0(process, WNOHANG, &exitcode)) {
        fprintf(stderr, "child process [redis] exit with code %d\n", exitcode);
        goto failure;
    }
    if (!check_redis_available("localhost", 8888)) {
        goto failure;
    }

    process->calls.kill0(process, SIGTERM);
    process->calls.wait(process, NULL);
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

int main(int argc, char* *argv) {
    int rc = 0;
    int port = 0;
    RedisServerBuilder *builder = NULL;
    RedisInstance *instance = NULL;

    builder = RedisServerBuilder_create();
    if (!builder)
        goto failure;

    srand(time(NULL));
    port = ((double) rand()) / RAND_MAX * 65534 + 1;
    builder->calls.optionNumber(builder, "port", port);
    //fprintf(stderr, "%s\n", builder->data._M_cfg[0]);
    instance = builder->calls.build(builder);
    if (!instance)
        goto failure;
    if (!check_redis_available("localhost", port))
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
    if (instance) {
        RedisInstance_destroy(instance);
        instance = NULL;
    }
    if (builder) {
        RedisServerBuilder_destroy(builder);
        builder = NULL;
    }
    goto exit;
}
