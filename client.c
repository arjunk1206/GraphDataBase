#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "client_msg.h"

int msqid;

void initializeMessageQueue()
{
    msqid = msgget(MSG_QUEUE_ID, IPC_CREAT | 0666);

    if (msqid == -1)
    {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
}

int handleGraphReadInput(key_t key, int shmid, void *shmaddr)
{
    struct GraphReadData graphReadData;
    printf("Enter start vertex: ");
    scanf("%d", &graphReadData.startVertex);
    graphReadData.startVertex--;

    shmid = shmget(key, sizeof(graphReadData), 0666 | IPC_CREAT);
    shmaddr = shmat(shmid, NULL, 0);
    if (shmaddr == (void *)-1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    memcpy(shmaddr, &graphReadData, sizeof(graphReadData));
    return shmid;
}

int handleGraphWriteInput(key_t key, int shmid, void *shmaddr)
{
    struct GraphData graphData;
    printf("Enter number of nodes: ");
    scanf("%d", &graphData.size);
    printf("Enter Adjacency matrix: \n");
    for (int i = 0; i < graphData.size; i++)
    {
        for (int j = 0; j < graphData.size; j++)
        {
            scanf("%d", &graphData.array[i][j]);
        }
    }
    shmid = shmget(key, sizeof(graphData), 0666 | IPC_CREAT);
    shmaddr = shmat(shmid, NULL, 0);
    if (shmaddr == (void *)-1)
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    memcpy(shmaddr, &graphData, sizeof(graphData));
    return shmid;
}

int main()
{
    initializeMessageQueue();

    while (1)
    {
        struct ClientMessage client_msg;

        client_msg.mtype = 1;

        // input sequence number
        printf("Enter Sequence Number: ");
        scanf("%d", &client_msg.sequenceNumber);

        // input operation number
        printf("Enter Operation Number: ");
        scanf("%d", &client_msg.operationNumber);

        // input graph name
        printf("Enter Graph File Name: ");
        gets(&client_msg.graphFileName);
        gets(&client_msg.graphFileName);

        key_t key = ftok("client.c", CLIENT_BUFFER_MTYPE + client_msg.sequenceNumber);
        int shmid;
        void *shmaddr;

        // if op number 1 or 2 take graph data input
        if (client_msg.operationNumber == 1 || client_msg.operationNumber == 2)
        {
            shmid=handleGraphWriteInput(key, shmid, shmaddr);
        }
        else if (client_msg.operationNumber == 3 || client_msg.operationNumber == 4)
        {
            shmid=handleGraphReadInput(key, shmid, shmaddr);
        }

        // send the message from the client to load balancer
        if (msgsnd(msqid, &client_msg, sizeof(client_msg), 0) == -1)
        {
            perror("msgsnd");
            exit(EXIT_FAILURE);
        }

        // // receive the message with the result

        int restype = client_msg.sequenceNumber + CLIENT_BUFFER_MTYPE;

        if (client_msg.operationNumber == 3 || client_msg.operationNumber == 4)
        {

            struct ResultMessage result_msg;
            if (msgrcv(msqid, &result_msg, sizeof(result_msg), restype, 0) == -1)
            {
                perror("msgrcv");
                exit(EXIT_FAILURE);
            }
            else
            {
                for (int i = 0; i < result_msg.counter; i++)
                {
                    printf("%d ", result_msg.res[i]+1);
                }
                printf("\n");

                if (-1 == (shmctl(shmid, IPC_RMID, NULL)))
                {
                    perror("shmctl");
                }
            }
        }
        else
        {
            struct WriteResultMessage result_msg;
            if (msgrcv(msqid, &result_msg, sizeof(result_msg), restype, 0) == -1)
            {
                perror("msgrcv");
                exit(EXIT_FAILURE);
            }
            else
            {
                printf("%s\n", result_msg.res);
                if (-1 == (shmctl(shmid, IPC_RMID, NULL)))
                {
                    perror("shmctl");
                }
            }
        }
    }
}