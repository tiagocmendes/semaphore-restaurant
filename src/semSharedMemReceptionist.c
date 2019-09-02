/**
 *  \file semSharedReceptionist.c (implementation file)
 *
 *  \brief Problem name: Restaurant
 *
 *  Synchronization based on semaphores and shared memory.
 *  Implementation with SVIPC.
 *
 *  Definition of the operations carried out by the receptionist:
 *     \li waitForGroup
 *     \li provideTableOrWaitingRoom
 *     \li receivePayment
 *
 *  \author Nuno Lau - December 2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "probConst.h"
#include "probDataStruct.h"
#include "logging.h"
#include "sharedDataSync.h"
#include "semaphore.h"
#include "sharedMemory.h"

/** \brief logging file name */
static char nFic[51];

/** \brief shared memory block access identifier */
static int shmid;

/** \brief semaphore set access identifier */
static int semgid;

/** \brief pointer to shared memory region */
static SHARED_DATA *sh;

/* constants for groupRecord */
#define TOARRIVE 0
#define WAIT     1
#define ATTABLE  2
#define DONE     3

/** \brief receptioninst view on each group evolution (useful to decide table binding) */
static int groupRecord[MAXGROUPS];

/* STUDENTS CODE: constant to know if a table is occupied */
#define TABLEOCCUPIED -2


/** \brief receptionist waits for next request */
static request waitForGroup ();

/** \brief receptionist waits for next request */
static void provideTableOrWaitingRoom (int n);

/** \brief receptionist receives payment */
static void receivePayment (int n);



/**
 *  \brief Main program.
 *
 *  Its role is to generate the life cycle of one of intervening entities in the problem: the receptionist.
 */
int main (int argc, char *argv[])
{
    int key;                                            /*access key to shared memory and semaphore set */
    char *tinp;                                                       /* numerical parameters test flag */

    /* validation of command line parameters */
    if (argc != 4) { 
        freopen ("error_RT", "a", stderr);
        fprintf (stderr, "Number of parameters is incorrect!\n");
        return EXIT_FAILURE;
    }
    else { 
        freopen (argv[3], "w", stderr);
        setbuf(stderr,NULL);
    }

    strcpy (nFic, argv[1]);
    key = (unsigned int) strtol (argv[2], &tinp, 0);
    if (*tinp != '\0') {   
        fprintf (stderr, "Error on the access key communication!\n");
        return EXIT_FAILURE;
    }

    /* connection to the semaphore set and the shared memory region and mapping the shared region onto the
       process address space */
    if ((semgid = semConnect (key)) == -1) { 
        perror ("error on connecting to the semaphore set");
        return EXIT_FAILURE;
    }
    if ((shmid = shmemConnect (key)) == -1) { 
        perror ("error on connecting to the shared memory region");
        return EXIT_FAILURE;
    }
    if (shmemAttach (shmid, (void **) &sh) == -1) { 
        perror ("error on mapping the shared region on the process address space");
        return EXIT_FAILURE;
    }

    /* initialize random generator */
    srandom ((unsigned int) getpid ());              

    /* initialize internal receptionist memory */
    int g;
    for (g=0; g < sh->fSt.nGroups; g++) {
       groupRecord[g] = TOARRIVE;
    }

    /* simulation of the life cycle of the receptionist */
    int nReq=0;
    request req;
    while( nReq < sh->fSt.nGroups*2 ) {
        req = waitForGroup();
        switch(req.reqType) {
            case TABLEREQ:
                   provideTableOrWaitingRoom(req.reqGroup); 
                   break;
            case BILLREQ:
                   receivePayment(req.reqGroup);
                   break;
        }
        nReq++;
    }

    /* unmapping the shared region off the process address space */
    if (shmemDettach (sh) == -1) {
        perror ("error on unmapping the shared region off the process address space");
        return EXIT_FAILURE;;
    }

    return EXIT_SUCCESS;
}

/**
 *  \brief decides table to occupy for group n or if it must wait.
 *
 *  Checks current state of tables and groups in order to decide table or wait.
 *
 *  \return table id or -1 (in case of wait decision)
 */
static int decideTableOrWait(int n)
{
     /* STUDENTS CODE */
     int g, tablesOccupied, i, table[NUMTABLES];
     for(i=0; i < NUMTABLES; i++)
        table[i] = i;
     for(g=0; g < sh->fSt.nGroups; g++)
     {
         if(groupRecord[g] == ATTABLE)
         {
             table[sh->fSt.assignedTable[g]] = TABLEOCCUPIED; 
             tablesOccupied++;
         }
     }

     if(tablesOccupied != NUMTABLES) {
         for(i=0; i < NUMTABLES; i++)
         {
             if(table[i] != TABLEOCCUPIED)
             {
                 return table[i];
             }
         }
     }
     /* END OF STUDENTS CODE REGION */

     return -1;
}

/**
 *  \brief called when a table gets vacant and there are waiting groups 
 *         to decide which group (if any) should occupy it.
 *
 *  Checks current state of tables and groups in order to decide group.
 *
 *  \return group id or -1 (in case of wait decision)
 */
static int decideNextGroup()
{
     /* STUDENTS CODE */
     int g;
     for(g=0; g < sh->fSt.nGroups; g++)
     {
         if(groupRecord[g] == WAIT)
         {
             return g;
         }
     }
     /* END OF STUDENTS CODE REGION */

     return -1;
}

/**
 *  \brief receptionist waits for next request 
 *
 *  Receptionist updates state and waits for request from group, then reads request,
 *  and signals availability for new request.
 *  The internal state should be saved.
 *
 *  \return request submitted by group
 */
static request waitForGroup()
{
    request ret; 

    /* enter critical region */
    if (semDown (semgid, sh->mutex) == -1)  {                                                  
        perror ("error on the down operation for mutex semaphore access (RT)");
        exit (EXIT_FAILURE);
    }

    /* STUDENTS CODE */
    sh->fSt.st.receptionistStat = WAIT_FOR_REQUEST;
    saveState(nFic,&sh->fSt);
    /* END OF STUDENTS CODE REGION */
    
    /* exit critical region */
    if (semUp (semgid, sh->mutex) == -1) {                                             
        perror ("error on the up operation for mutex semaphore access (RT)");
        exit (EXIT_FAILURE);
    }

    /* STUDENTS CODE */
    if (semDown (semgid, sh->receptionistReq) == -1)  {                                                  
        perror ("error on the down operation for receptionistReq semaphore access (RT)");
        exit (EXIT_FAILURE);
    }
    /* END OF STUDENTS CODE REGION */

    /* enter critical region */
    if (semDown (semgid, sh->mutex) == -1)  {                                                  
        perror ("error on the down operation for mutex semaphore access (RT)");
        exit (EXIT_FAILURE);
    }

    /* STUDENTS CODE */
    ret.reqGroup = sh->fSt.receptionistRequest.reqGroup;
    ret.reqType = sh->fSt.receptionistRequest.reqType;
    /* END OF STUDENTS CODE REGION */

    /* exit critical region */
    if (semUp (semgid, sh->mutex) == -1) {                                                  
     perror ("error on the up operation for mutex semaphore access (RT)");
        exit (EXIT_FAILURE);
    }

    /* STUDENTS CODE */
    if (semUp (semgid, sh->receptionistRequestPossible) == -1) {                                                  
     perror ("error on the up operation for receptionistRequestPossible semaphore access (RT)");
        exit (EXIT_FAILURE);
    }
    /* END OF STUDENTS CODE REGION */

    return ret;

}

/**
 *  \brief receptionist decides if group should occupy table or wait
 *
 *  Receptionist updates state and then decides if group occupies table
 *  or waits. Shared (and internal) memory may need to be updated.
 *  If group occupies table, it must be informed that it may proceed. 
 *  The internal state should be saved.
 *
 */
static void provideTableOrWaitingRoom (int n)
{
    /* enter critical region */
    if (semDown (semgid, sh->mutex) == -1)  {                                                  
        perror ("error on the down operation for mutex semaphore access (RT)");
        exit (EXIT_FAILURE);
    }

    /* STUDENTS CODE */
    sh->fSt.st.receptionistStat = ASSIGNTABLE;
    saveState(nFic, &sh->fSt);

    int table_id = decideTableOrWait(n);

    if(table_id != -1)
    {
        groupRecord[n] = ATTABLE;
        sh->fSt.assignedTable[n] = table_id;
        sh->fSt.st.receptionistStat = WAIT_FOR_REQUEST;
        saveState(nFic, &sh->fSt);
        if (semUp (semgid, sh->waitForTable[n]) == -1) {
            perror ("error on the up operation for waitForTable semaphore access (RT)");
            exit (EXIT_FAILURE);
        }
    }
    else
    {
        groupRecord[n] = WAIT;
        sh->fSt.groupsWaiting++;
        saveState(nFic, &sh->fSt);
    }
    /* END OF STUDENTS CODE REGION */

    /* exit critical region */
    if (semUp (semgid, sh->mutex) == -1) {                                               
        perror ("error on the up operation for mutex semaphore access (RT)");
        exit (EXIT_FAILURE);
    }

}

/**
 *  \brief receptionist receives payment 
 *
 *  Receptionist updates its state and receives payment.
 *  If there are waiting groups, receptionist should check if table that just became
 *  vacant should be occupied. Shared (and internal) memory should be updated.
 *  The internal state should be saved.
 *
 */

static void receivePayment (int n)
{   
    int nextGroup, empty_table;

    /* enter critical region */
    if (semDown (semgid, sh->mutex) == -1)  {                                                  
        perror ("error on the down operation for mutex semaphore access (RT)");
        exit (EXIT_FAILURE);
    }

    /* STUDENTS CODE */
    sh->fSt.st.receptionistStat = RECVPAY;
    saveState(nFic, &sh->fSt);
    
    groupRecord[n] = DONE;
    empty_table = sh->fSt.assignedTable[n];
    sh->fSt.assignedTable[n] = -1;

    if(sh->fSt.groupsWaiting > 0)
    {
        nextGroup = decideNextGroup();
        groupRecord[nextGroup] = ATTABLE;
        sh->fSt.assignedTable[nextGroup] = empty_table;
        sh->fSt.groupsWaiting--;
        sh->fSt.st.receptionistStat = WAIT_FOR_REQUEST;
        saveState(nFic, &sh->fSt);
        if (semUp (semgid, sh->waitForTable[nextGroup]) == -1) {
            perror ("error on the up operation for waitForTable semaphore access (RT)");
            exit (EXIT_FAILURE);
        }
    }
    /* END OF STUDENTS CODE REGION */

    /* exit critical region */
    if (semUp (semgid, sh->mutex) == -1)  {                                                  
     perror ("error on the up operation for mutex semaphore access (RT)");
        exit (EXIT_FAILURE);
    }

    /* STUDENTS CODE */
    if (semUp (semgid, sh->tableDone[empty_table]) == -1) {                                               
        perror ("error on the up operation for tableDone semaphore access (RT)");
        exit (EXIT_FAILURE);
    }
    /* END OF STUDENTS CODE REGION */
}

