#include <threading.h>

void t_init()
{
        // TODO: Loop through all 16 contexts and set each one as INVALID.
        // Hint: Use memset to clear the context structure.
        // The first context (index 0) represents the main thread, so set it to VALID.
        // Initialize current_context_ id to o to mark the currently running context,

        for (int i = 0; i < 16; i++) {
                memset(&contexts[i].context, 0, sizeof(contexts[i].context));
                contexts[i].state = INVALID;
        }
        contexts[0].state = VALID;

        current_context_idx = 0;
}

int32_t t_create(fptr foo, int32_t arg1, int32_t arg2)
{
        for (int i = 0; i < 16; i++) {
                if (contexts[i].state == INVALID) {
                        getcontext(&contexts[i].context);
                        contexts[i].context.uc_stack.ss_sp = (char *) malloc(STK_SZ);
                        contexts[i].context.uc_stack.ss_size = STK_SZ;
                        contexts[i].context.uc_stack.ss_flags = 0;
                        contexts[i].context.uc_link = NULL;
                        makecontext(&contexts[i].context, (void (*)()) foo, 2, arg1, arg2);
                        break;
                }
                else if (i == 15 && contexts[i].state == VALID) {
                        return 1;
                }
        }
        return 0;
}

int32_t t_yield()
{
        // TODO
}

void t_finish()
{
        // TODO
}
