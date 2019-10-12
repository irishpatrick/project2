#include <sys/mman.h>
#include <linux/unistd.h>
#include <sys/mman.h>
#include <linux/unistd.h>
#include <stdio.h>
#include "condvar.h"
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

/* FUNCTION DECLARATIONS */

int randint(int, int);
void allocate(void **, unsigned long);
void deallocate(void **, unsigned long);

void tenantArrives();
void agentArrives();
void viewApt();
void openApt();
void tenantLeaves();
void agentLeaves();

/* UTILITIES */

void allocate(void **ptr, unsigned long mmap_size)
{
    *ptr = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    if (*ptr == NULL)
    {
        fprintf(stderr, "allocate error\n");
        exit(0);
    }
    memset(*ptr, 0, mmap_size);
}

void deallocate(void **ptr, unsigned long mmap_size)
{
    int ret = munmap(*ptr, mmap_size);
    if (ret == -1)
    {
        fprintf(stderr, "dealloc error\n");
        exit(0);
    }
    *ptr = NULL;
}

int randint(int min, int max)
{
    return (rand() % (max - min + 1)) + min;
}

/* MONITOR */

typedef struct _Monitor
{
    struct cs1550_lock *lock;
    struct cs1550_condition *agent_arrive;
    struct cs1550_condition *cond2;
    
    int num_tenants;
    int num_agents;
} Monitor;

void tenantArrives()
{
}

void tenantLeaves()
{
}

void agentArrives()
{
}

void agentLeaves()
{
}

void viewApt()
{
}

void openApt()
{
}


/* PROCESSES */

void agent_proc()
{

}

void tenant_proc()
{

}

int main(int argc, char **argv)
{
    if (argc < 5)
    {
        printf("usage: ./aptsim -m <num tenants> -k <num agents>\n");
        return 0;
    }

    srand(time(NULL));

    Monitor aptsim;

    aptsim.num_tenants = -1;
    aptsim.num_agents = -1;

    int i;
    for (i = 1; i < 5; i += 2)
    {
        if (strcmp(argv[i], "-m") == 0)
        {
            aptsim.num_tenants = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-k") == 0)
        {
            aptsim.num_agents = atoi(argv[i + 1]);
        }
    }

    if (aptsim.num_tenants < 1 || aptsim.num_agents < 1)
    {
        printf("bad arguments\n");
        return 0;
    }

    int num_cond_vars = 2;
    int num_flags = 2;

   /* SHARE MEMORY */

    struct cs1550_lock *lock = mmap(
        NULL, 
 
        sizeof(struct cs1550_lock) + 
        num_cond_vars * sizeof(struct cs1550_condition) + 
        num_flags * sizeof(bool),

        PROT_READ | PROT_WRITE, 
        MAP_SHARED | MAP_ANONYMOUS, 
        0, 0);
    
    /* CREATE ALL SYNC STUFF */
    
    struct cs1550_condition *cond1 = (struct cs1550_condition *)(lock + 1);
    struct cs1550_condition *cond2 = cond1 + 1;
    bool *done1 = (bool *)(cond2 + 1);
    bool *done2 = done1 + 1;

    cs1550_init_lock(lock);
    cs1550_init_condition(cond1, lock);
    cs1550_init_condition(cond2, lock);

    *done1 = false;
    *done2 = false;

    /* CREATE PROCESSES */

    int pid = fork();
    if (pid == 0)
    {
        // tenant spawner
        int i;
        while (i < num_tenants)
        {
            int prob = randint(1, 10);
            if (prob == 5)
            {
                ++i;
                int spawn = fork();
                if (pid == 0)
                {
                    // tenant
                    tenant_proc();
                }
                else
                {
                    // parent proc
                }
            }
        }        
    }
    else
    {
        // parent
        pid = fork();
        if (pid == 0)
        {
            // agent spawner
            int i;
            while (i < num_agents)
            {
                int prob = randint(1, 10);
                if (prob == 5)
                {
                    ++i;
                    int spawn = fork();
                    if (spawn == 0)
                    {
                        // agent
                        agent_proc();
                    }
                    else
                    {
                        // parent
                    }
                } 
            }
        }
        else
        {
            // parent


            wait(NULL);
            wait(NULL);
            int i;
            for (i = 0; i < num_tenants; ++i)
            {
                wait(NULL);
            }
            for (i = 0; i < num_agents; ++i)
            {
                wait(NULL);
            }
        }
    }

    return 0;
}
