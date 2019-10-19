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

/* GLOBAL VARS */ 

int start_time; 

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

int cur_time()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return (int)(tv.tv_sec);
}

int elapsed()
{
    return cur_time() - start_time;
}

/* MONITOR */

typedef struct _Monitor
{
    struct cs1550_lock *lock;
    
    /* CONDITION VARIABLES */
    struct cs1550_condition *apt_empty;
    struct cs1550_condition *want_to_view;
    struct cs1550_condition *last_tenant;
    struct cs1550_condition *first_tenant;

    /* FLAGS */

    bool *apt_open;
    bool *agent_inside;

    /* OTHER STUFF */  

    int *num_views;
    int *num_inside;
    int *num_waiting;

    /* CONSTANTS */
    
    int num_tenants;
    int num_agents;

} Monitor;

void tenantArrives(Monitor *m, int id)
{
    cs1550_acquire(m->lock);

    printf("Tenant %d arrives at time %d.\n", id, elapsed());

    if (*(m->num_waiting) == 0)
    {
        cs1550_signal(m->first_tenant);
    }

    *(m->num_waiting) += 1;
    
    while (!*(m->apt_open) || *(m->num_views) >= 10)
    {
        cs1550_wait(m->want_to_view);
    }

    printf("got thru\n");

    *(m->num_waiting) -= 1;
    *(m->num_inside) += 1;
    *(m->num_views) += 1;

    cs1550_release(m->lock);
}

void tenantLeaves(Monitor *m, int id)
{
    cs1550_acquire(m->lock);

    printf("Tenant %d leaves at time %d.\n", id, elapsed());

    *(m->num_inside) -= 1;
    if (*(m->num_inside) == 0)
    {
        cs1550_signal(m->last_tenant);
    }

    cs1550_release(m->lock);
}

void agentArrives(Monitor *m, int id)
{
    cs1550_acquire(m->lock);

    printf("Agent %d arrives at time %d.\n", id, elapsed());

    while (*(m->num_waiting) < 1)
    {
        cs1550_wait(m->first_tenant);
    }

    while (*(m->apt_open) || *(m->agent_inside))
    {
        cs1550_wait(m->apt_empty);
    }

    //*(m->apt_open) = true;
    *(m->agent_inside) = true;

    printf("going to open the apt\n");

    cs1550_release(m->lock);
}

void agentLeaves(Monitor *m, int id)
{
    cs1550_acquire(m->lock);

    printf("num_inside=%d\tnum_waiting=%d\n", *m->num_inside, *m->num_waiting);

    while (*(m->num_inside) > 0 || (*(m->num_waiting) > 0 && *(m->num_views) < 10))
    //while (*(m->apt_open))
    {
        cs1550_wait(m->last_tenant);
    }

    *(m->num_inside) = 0;
    *(m->num_views) = 0;
    *(m->apt_open) = false;
    *(m->agent_inside) = false;

    cs1550_signal(m->apt_empty);

    printf("Agent %d leaves at time %d.\n", id, elapsed());

    cs1550_release(m->lock);
}

void viewApt(Monitor *m, int id)
{
    cs1550_acquire(m->lock);
    
    printf("Tenant %d inspects the apartment at time %d\n", id, elapsed());

    sleep(2);
    
    cs1550_release(m->lock); 
}

void openApt(Monitor *m, int id)
{
    cs1550_acquire(m->lock); 
    
    sleep(2);

    *(m->apt_open) = true; 

    printf("Agent %d opens the apartment for inspection at time %d\n", id, elapsed());

    //cs1550_broadcast(m->want_to_view);
    printf("%d, %d\n", !*(m->apt_open), *(m->num_views) >= 10);
    int i;
    for (i = 0; i < *(m->num_waiting); i++)
    {
        printf("signal\n");
        cs1550_signal(m->want_to_view);
    }

    cs1550_release(m->lock);
}


/* PROCESSES */

void agent_proc(Monitor *m, int id)
{
    agentArrives(m, id);
    openApt(m, id);
    agentLeaves(m, id);
}

void tenant_proc(Monitor* m, int id)
{
    tenantArrives(m, id);
    viewApt(m, id);
    tenantLeaves(m, id);
}

/* MAIN METHOD */ 

int main(int argc, char **argv)
{
    // validate argc
    if (argc < 5)
    {
        printf("usage: ./aptsim -m <num tenants> -k <num agents>\n");
        return 0;
    }

    // seed random
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

    int num_cond_vars = 4;
    int num_vals = 3;
    int num_flags = 3;

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
    struct cs1550_condition *cond4 = cond3 + 1;
    bool *done1 = (bool *)(cond4 + 1);
    bool *done2 = done1 + 1;
    bool *done3 = done2 + 1;
    int *val1 = (int *)(done3 + 1);
    int *val2 = val1 + 1;
    int *val3 = val2 + 1;

    *done1 = false;
    *done2 = false;
    *done3 = false;
    *val1 = 0;
    *val2 = 0;
    *val3 = 0;
    cs1550_init_lock(lock);
    cs1550_init_condition(cond1, lock);
    cs1550_init_condition(cond2, lock);
    cs1550_init_condition(cond3, lock);
    cs1550_init_condition(cond4, lock);

    aptsim.lock = lock;

    aptsim.apt_empty = cond1;
    aptsim.want_to_view = cond2;
    aptsim.last_tenant = cond3;
    aptsim.first_tenant = cond4;

    aptsim.apt_open = done1;
    aptsim.agent_inside = done2;

    aptsim.num_views = val1;
    aptsim.num_inside = val2;
    aptsim.num_waiting = val3;

    printf("set start time\n");
    start_time = cur_time();

    /* CREATE PROCESSES */

    int pid = fork();
    if (pid == 0)
    {
        // tenant spawner
        int i = 0;
        while (i < aptsim.num_tenants)
        {
            //printf("l");
            int prob = randint(1, 1000000);
            if (prob == 5)
            {
                ++i;
                int spawn = fork();
                if (spawn == 0)
                {
                    // tenant
                    tenant_proc(&aptsim, i);
                    break;
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
        int pid2 = fork();
        if (pid2 == 0)
        {
            // agent spawner
            int j = 0;
            while (j < aptsim.num_agents)
            {
                int prob = randint(1, 1000000);
                if (prob == 5)
                {
                    ++j;
                    int spawn = fork();
                    if (spawn == 0)
                    {
                        // agent
                        agent_proc(&aptsim, j);
                        break;
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
        }
    }

    /* WAIT FOR CHILDREN */
    int k = 0;

    /* WAIT FOR TENANTS */
    for (k = 0; k < aptsim.num_tenants; ++k)
    {
        wait(NULL);
    }

    /* WAIT FOR AGENTS */
    for (k = 0; k < aptsim.num_agents; ++k)
    {
        wait(NULL);
    }

    /* WAIT FOR SPAWNERS */
    wait(NULL);
    wait(NULL);

    return 0;
}
