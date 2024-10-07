#include <threading.h>
#include <stdbool.h>

void t_init()
{
        // TODO: Loop through all 16 contexts and set each one as INVALID.
        // Hint: Use memset to clear the context structure.
        // The first context (index 0) represents the main thread, so set it to VALID.
        // Initialize current_context_ id to o to mark the currently running context,

        memset(contexts, 0, sizeof(contexts));
        
        for (int i = 0; i < NUM_CTX; i++) {
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
                        contexts[i].state = VALID;
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
    if (contexts[current_context_idx].context.uc_stack.ss_sp) {
        free(contexts[current_context_idx].context.uc_stack.ss_sp);

        contexts[current_context_idx].context.uc_stack.ss_sp = NULL;
        contexts[current_context_idx].context.uc_stack.ss_size = 0;
    }

    contexts[current_context_idx].state = DONE;

    if (schedule() == 1) {
        current_context_idx = 0;
        setcontext(&contexts[0].context);
    }
}


int32_t schedule() {
    bool foundContext = false;

    int next_idx = (current_context_idx + 1) % NUM_CTX;

    for (int i = 0; i < NUM_CTX; i++) {
        if (contexts[next_idx].state == VALID && next_idx != current_context_idx) {
            foundContext = true;
            
            int prev_idx = current_context_idx;
            current_context_idx = (uint8_t) next_idx;
            swapcontext(&contexts[prev_idx].context, &contexts[current_context_idx].context);
            break;
        }
        next_idx = (next_idx + 1) % NUM_CTX;
    }

    if (!foundContext) {
        return 1;
    }

    return 0;
}
