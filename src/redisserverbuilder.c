#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#if defined(__linux__) || defined(__APPLE__) || defined(__ANDROID__)
#   include <sys/types.h>
#   include <unistd.h>
#endif

#include "redisserverbuilder.h"

#ifndef LOGI
#   define LOGI(fmt, ...)                                                      \
    do {                                                                       \
        fprintf(stderr, "[RedisServerBuilder][I] " fmt "\n", ##__VA_ARGS__);   \
    } while (0)
#endif

int RedisServerBuilder_isFileExists(char const *filename) {
    int rc = 0;
    FILE *fp = NULL;
    LOGI("check exists of file %s", filename);
    fp = fopen(filename, "r");
    if (!fp)
        goto failure;

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
    if (fp) {
        fclose(fp);
        fp = NULL;
    }
    goto exit;
}

char* RedisServerBuilder_findInPATH(RedisServerBuilder const* me) {
    char *r = NULL;
    char const *filename_with_slash = "/redis-server";
    char const *filename_without_slash = "redis-server";
    char const *filename = NULL;
    char *path = NULL;
    char const *pos = NULL;
    char const *found = NULL;
    int path_size = FILENAME_MAX + 1;
    int len = 0;
    char const *end = NULL;

    pos = getenv("PATH");
    if (!pos)
        goto failure;
    len = strlen(pos);
    end = pos + len;

    path = (char*) calloc(1, path_size);
    if (!path)
        goto failure;

    /* Try all the paths */
    for (; pos < end && strlen(&path[0]) < 1; pos = found + 1) {
        found = strstr(pos, ":");
        if (!found) {
            /* It's the last part */
            found = end;
        } else if (found - pos < 1) {
            /* skip empty string */
            continue;
        }

        filename = *(found - 1) == '/'
            ? filename_without_slash
            : filename_with_slash;
        if (found - pos + strlen(filename) + 1 > path_size) {
            /* skip truncated */
            continue;
        }
        memset(&path[0], 0, path_size);
        strncpy(&path[0], pos, found - pos);
        strncat(&path[0], filename, strlen(filename));
        if (!RedisServerBuilder_isFileExists(&path[0]))
            path[0] = '\0';
    }

    if (strlen(&path[0]) == 0)
        goto failure;

    goto success;
exit:
    return r;
success:
    r = path;
    path = NULL;
    goto cleanup;
failure:
    goto cleanup;
cleanup:
    if (path) {
        free(path);
        path = NULL;
    }
    goto exit;
}

void
RedisInstance_destroy(RedisInstance *me) {
    int exitcode = 0;
    if (me) {
        if (me->data._M_process) {
            me->data._M_process->calls.kill0(me->data._M_process, SIGTERM);
            me->data._M_process->calls.wait(me->data._M_process, &exitcode);
            LOGI("redis instance exit with code %d", exitcode);
            Process_destroy(me->data._M_process);
            me->data._M_process = NULL;
        }
        free(me);
        me = NULL;
    }
}

static
RedisInstance* RedisInstance_create() {
    RedisInstance *instance = NULL;
    instance = (RedisInstance*) calloc(1, sizeof(*instance));
    return instance;
}

RedisInstance* RedisServerBuilder_build0(RedisServerBuilder const *me,
        char const *executable_path) {
    RedisInstance *instance = NULL;
    RedisInstance *r = NULL;
    int exitcode = 0;
    ProcessBuilder *pb = NULL;
    Process *p = NULL;

    pb = ProcessBuilder_create();
    if (!pb)
        goto failure;
    pb->calls.setFile(pb, executable_path);
    pb->calls.setArguments(pb, (char const**) me->data._M_cfg);
    p = pb->calls.build(pb);
    if (!p)
        goto failure;
    /* wait a while and check whether process is terminated */
    sleep(1);
    if (p->calls.wait0(p, WNOHANG, &exitcode)) {
        /* process is already terminated */
        LOGI("redis process failed with exit code %d", exitcode);
        goto failure;
    }
    instance = RedisInstance_create();
    if (!instance)
        goto failure;
    instance->data._M_process = p;
    p = NULL;

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
        RedisInstance_destroy(instance);
        instance = NULL;
    }
    if (p) {
        Process_destroy(p);
        p = NULL;
    }
    if (pb) {
        ProcessBuilder_destroy(pb);
        pb = NULL;
    }
    goto exit;
}

RedisInstance* RedisServerBuilder_build(RedisServerBuilder const *me) {
    RedisInstance *r = NULL;
    char const *path = NULL;

    path = RedisServerBuilder_findInPATH(me);
    if (!path)
        goto failure;
    r = RedisServerBuilder_build0(me, path);

    goto success;
exit:
    return r;
success:
    goto cleanup;
failure:
    goto cleanup;
cleanup:
    if (path) {
        free((void*) path);
        path = NULL;
    }
    goto exit;
}

RedisServerBuilder* RedisServerBuilder_optionString(RedisServerBuilder *me,
        char const *name, char const *value) {
    char buffer[1024];
    int c = 0;
    char **p = NULL;
    size_t size = 0;
    size_t required_size = 0;
    RedisServerBuilder *r = NULL;

    /* append option string */

    /*
     * FIXME DO NOT add space before "--name value", which  will let
     * redis-server take it as config file name.
     */
    c = snprintf(&buffer[0], sizeof(buffer), "--%s %s", name, value);
    if (c < 0) {
        /* impossbile */
        LOGI("snprintf failed!");
        goto failure;
    } else if (c >= sizeof(buffer)) {
        /* truncated */
        LOGI("snprintf truncated %s => %s", value, &buffer[0]);
        goto failure;
    }
    if (me->data._M_cfg)
        for (p = me->data._M_cfg; *p; ++p)
            ++size;
    required_size = size ? (size + 1 + 1) : 2;
    p = (char**) realloc(me->data._M_cfg,
            required_size * sizeof(*me->data._M_cfg));
    if (!p)
        goto failure;
    if (required_size == 2)
        memset(p, 0, sizeof(*p) * 2);
    me->data._M_cfg = p;
    p = &me->data._M_cfg[required_size - 1];
    /* terminated it with NULL in case of future allocation failure */
    *p = NULL;
    --p;
    *p = (char*) realloc(NULL, strlen(buffer) + 1);
    if (!*p)
        goto failure;
    strncpy(*p, &buffer[0], strlen(buffer) + 1);

    goto success;
exit:
    return r;
success:
    r = me;
    goto cleanup;
failure:
    LOGI("%s failed", __func__);
    r = NULL;
    goto cleanup;
cleanup:
    goto exit;
}

RedisServerBuilder* RedisServerBuilder_optionNumber(RedisServerBuilder *me,
        char const *name, long value) {
    RedisServerBuilder *r = NULL;
    int c = 0;
    char buffer[0xff];

    memset(&buffer[0], 0, sizeof(buffer));
    c = snprintf(&buffer[0], sizeof(buffer), "%ld", value);
    if (c >= sizeof(buffer)) {
        /* truncated is nearly impossible due to buffer size */
        LOGI("truncated option value for long %ld => %s", value, &buffer[0]);
        goto failure;
    } else if (c < 0) {
        goto failure;
    }
    if (!me->calls.optionString(me, name, &buffer[0]))
        goto failure;

    goto success;
exit:
    return r;
success:
    r = me;
    goto cleanup;
failure:
    goto cleanup;
cleanup:
    goto exit;
}

char const** RedisServerBuilder_getParameters(RedisServerBuilder const *me) {
    return (char const**) me->data._M_cfg;
}

RedisServerBuilder* RedisServerBuilder_create() {
    RedisServerBuilder *instance = (RedisServerBuilder*) calloc(1, sizeof(*instance));
    instance->calls.build0 = &RedisServerBuilder_build0;
    instance->calls.build = &RedisServerBuilder_build;
    instance->calls.optionString = &RedisServerBuilder_optionString;
    instance->calls.optionNumber = &RedisServerBuilder_optionNumber;
    instance->calls.getParameters = &RedisServerBuilder_getParameters;
    return instance;
}

void RedisServerBuilder_destroy(RedisServerBuilder *me) {
    char **p = NULL;
    if (me) {
        if (me->data._M_cfg) {
            for (p = me->data._M_cfg; *p; ++p)
                free((void*) *p);
            free(me->data._M_cfg);
            me->data._M_cfg = NULL;
        }
        free(me);
        /* Nonsense assignment */
        me = NULL;
    }
}

