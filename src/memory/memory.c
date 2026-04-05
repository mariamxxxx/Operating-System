#include "memory.h"
#include "pcb.h"
#include <stdio.h>
#include <string.h>

MemoryWord mem[MEMORY_SIZE];

void init_memory()
{
    for (int i = 0; i < MEMORY_SIZE; i++)
    {
        mem[i].isFree = 1;
        mem[i].ownerPid = -1;
        mem[i].varName[0] = '\0';
        mem[i].value[0] = '\0';
    }
}

int allocate_memory(int pid, int num_words)
{
    int count = 0;
    for (int i = 0; i < MEMORY_SIZE; i++)
    {
        if (mem[i].isFree)
        {
            count++;
            if (count == num_words)
            {
                // Mark the block as owned by pid
                for (int j = i - num_words + 1; j <= i; j++)
                {
                    mem[j].isFree = 0;
                    mem[j].ownerPid = pid;
                }
                return i - num_words + 1; // Return starting index
            }
        }
        else
        {
            count = 0; // Reset count if we hit an occupied slot
        }
    }
    return -1; // No contiguous block found
}
