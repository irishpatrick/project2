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
    
    /* CONDITION VARIABLES */
    struct cs1550_condition *apt_empty;
    struct cs1550_condition *want_to_view;
    struct cs1550_condition *last_tenant;

    /* FLAGS */

    bool *apt_open;

    /* OTHER STUFF */  

    int *num_views;
    int *num_inside;

    /* CONSTANTS */
    
    int num_tenants;
    int num_agents;

} Monitor;

void tenantArrives(Monitor *m)
{
       
}

void tenantLeaves(Monitor *m)
{
}

void agentArrives(Monitor *m)
{
}

void agentLeaves(Monitor *m)
{
}

void viewApt(Monitor *m)
{
}

void openApt(Monitor *m)
{
}


/* PROCESSES */

void agent_proc(Monitor *m)
{

}

void tenant_proc(Monitor* m)
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

    int num_cond_vars = 3;
    int num_vals = 2;
    int num_flags = 2;

   /* SHARE MEMORY */

    struct cs1550_lock *lock = mmap(
        NULL, 
 
        sizeof(struct cs1550_lock) + 
        num_cond_vars * sizeof(struct cs1550_condition) + 
        num_flags * sizeof(bool) + 
        num_vals * sizeof(int),

        PROT_READ | PROT_WRITE, 
        MAP_SHARED | MAP_ANONYMOUS, 
        0, 0);
    
    /* CREATE ALL SYNC STUFF */
    
    struct cs1550_condition *cond1 = (struct cs1550_condition *)(lock + 1);
    struct cs1550_condition *cond2 = cond1 + 1;
    struct cs1550_condition *cond3 = cond2 + 1;
    bool *done1 = (bool *)(cond3 + 1);
    bool *done2 = done1 + 1;
    int *val1 = (int *)(done2 + 1);
    int *val2 = val1 + 1;

    cs1550_init_lock(lock);
    cs1550_init_condition(cond1, lock);
    cs1550_init_condition(cond2, lock);

    *done1 = false;
    *done2 = false;

    aptsim.apt_empty = cond1;
    aptsim.want_to_view = cond2;
    aptsim.last_tenant = cond3;

    aptsim.apt_open = done1;

    aptsim.num_views = val1;
    aptsim.num_inside = val2;

    /* CREATE PROCESSES */

    int pid = fork();
    if (pid == 0)
    {
        // tenant spawner
        int i;
        while (i < aptsim.num_tenants)
        {
            int prob = randint(1, 10);
            if (prob == 5)
            {
                ++i;
                int spawn = fork();
                if (pid == 0)
                {
                    // tenant
                    tenant_proc(&aptsim);
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
            while (i < aptsim.num_agents)
            {
                int prob = randint(1, 10);
                if (prob == 5)
                {
                    ++i;
                    int spawn = fork();
                    if (spawn == 0)
                    {
                        // agent
                        agent_proc(&aptsim);
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


            // TODO wait for all procs
            wait(NULL);
            wait(NULL);
            int i;
            for (i = 0; i < aptsim.num_tenants; ++i)
            {
                wait(NULL);
            }
            for (i = 0; i < aptsim.num_agents; ++i)
            {
                wait(NULL);
            }
        }
    }

    return 0;
}
