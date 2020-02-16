#ifndef PROCESSBUILDER_H_INCLUDED
#define PROCESSBUILDER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

struct tagProcess;
struct tagProcessBuilder;

typedef struct tagProcess Process;
typedef struct tagProcessBuilder ProcessBuilder;

struct tagProcess {
    struct {
        int         (*wait0)    (Process*, int, int*);
        int         (*wait)     (Process*, int*);
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

extern ProcessBuilder*  ProcessBuilder_create();
extern void             ProcessBuilder_destroy(ProcessBuilder*);
extern void             Process_destroy(Process*);

#ifdef __cplusplus
}
#endif


#endif /* PROCESSBUILDER_H_INCLUDED */
