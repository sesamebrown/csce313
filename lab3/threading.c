#include <threading.h>
#include <stdbool.h>

void t_init()
{
        // TODO: Loop through all 16 contexts and set each one as INVALID.
        // Hint: Use memset to clear the context structure.
        // The first context (index 0) represents the main thread, so set it to VALID.
        // Initialize current_context_ id to o to mark the currently running context,

        for (int i = 0; i < NUM_CTX; i++) {
                memset(contexts, 0, sizeof(contexts));
                contexts[i].state = INVALID;
        }
        contexts[0].state = VALID;
        current_context_idx = 0;
        getcontext(&contexts[0].context);
}

int32_t t_create(fptr foo, int32_t arg1, int32_t arg2)
{
        bool foundContext = false;

        for (volatile int i = 1; i < NUM_CTX; i++) {
                if (contexts[i].state == INVALID) {
                        foundContext = true;

                        contexts[i].context.uc_stack.ss_sp = malloc(STK_SZ);
                        contexts[i].context.uc_stack.ss_size = STK_SZ;
                        contexts[i].context.uc_stack.ss_flags = 0;
                        contexts[i].context.uc_link = &contexts[0].context;

                        getcontext(&contexts[i].context);
                        makecontext(&contexts[i].context, (void (*)(void)) foo, 2, arg1, arg2);
                        contexts[0].state = VALID;
                        break;
                }
        }
        if (!foundContext) {
                return 1;
        }

        return 0;
}

int32_t t_yield()
{       
        int32_t validCount = 0;

        int32_t scheduled = schedule();
        if (scheduled == 1) {
                return validCount;
        }

        for (int i = current_context_idx; i < NUM_CTX; i++) {
                if (contexts[i].state == VALID) {
                        validCount++;
                }
        }

        return validCount;
}

void t_finish()
{
        bool foundContext = false;

        if (contexts[current_context_idx].context.uc_stack.ss_sp) {
                free(contexts[current_context_idx].context.uc_stack.ss_sp);

                contexts[current_context_idx].context.uc_stack.ss_sp = NULL;
                contexts[current_context_idx].context.uc_stack.ss_size = 0;
        }

        contexts[current_context_idx].state = DONE;
        
        for (int i = 0; i < NUM_CTX; i++) {
                if (contexts[i].state == VALID) {
                        swapcontext(&contexts[current_context_idx].context, &contexts[i].context);
                        break;
                }
        }
        
        if (!foundContext) {
                current_context_idx = 0;
                setcontext(&contexts[current_context_idx].context);
                return;
        }
}

int32_t schedule() {
        bool foundContext = false;

        for (int i = 0; i < NUM_CTX; i++) {
                if ((contexts[i].state == VALID) && (i != current_context_idx)) {
                        foundContext = true;

                        swapcontext(&contexts[current_context_idx].context, &contexts[i].context);
                        current_context_idx = (uint8_t) i;

                        break;
                }
        }
        
        if (!foundContext) {
                return 1;
        }

        return 0;
}
