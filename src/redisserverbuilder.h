#ifndef REDISSERVERBUILDER_H_INCLUDED
#define REDISSERVERBUILDER_H_INCLUDED

#include <stddef.h>

#include "processbuilder.h"

#ifdef __cplusplus
extern {
#endif

    struct tagRedisInstance;
    struct tagRedisServerBuilder;

    typedef struct tagRedisInstance RedisInstance;
    typedef struct tagRedisServerBuilder RedisServerBuilder;

    struct tagRedisInstance {
        struct {
            Process *_M_process;
        } data;
    };

    struct tagRedisServerBuilder {
        struct {
            RedisInstance*      (*build)        (RedisServerBuilder const*);
            RedisInstance*      (*build0)       (RedisServerBuilder const*, char const*);
            RedisServerBuilder* (*optionString) (RedisServerBuilder*, char const *name, char const *value);
            RedisServerBuilder* (*optionNumber) (RedisServerBuilder*, char const *name, long value);
            char const**        (*getParameters)(RedisServerBuilder const*);
        } calls;

        struct {
            char    **_M_cfg;
        } data;
    };

    extern RedisServerBuilder*  RedisServerBuilder_create();
    extern void                 RedisServerBuilder_destroy(RedisServerBuilder*);

    extern void                 RedisInstance_destroy(RedisInstance*);

#ifdef __cplusplus
}
#endif

#endif /* REDISSERVERBUILDER_H_INCLUDED */
