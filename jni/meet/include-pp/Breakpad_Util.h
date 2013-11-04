#ifndef GOOGLE_BREAKPAD_H_
#define GOOGLE_BREAKPAD_H_

namespace android {

typedef bool (*INIT_BREAKPAD_HANDLER)(void**, const char*);
typedef bool (*FINIT_BREAKPAD_HANDLER)(void*);

typedef struct BreakpadHandle {
    void* breakpad_lib_handle;

    INIT_BREAKPAD_HANDLER init_breakpad_handler;
    FINIT_BREAKPAD_HANDLER finit_breakpad_handler;

    BreakpadHandle();
    ~BreakpadHandle();
} BreakpadHandle;

bool BreakpadHandle_Create(BreakpadHandle**, const char*);

static bool fun_setUp(void*, void**, const char*);
}

#endif
