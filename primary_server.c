#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <pthread.h>
#include <fcntl.h>
#include<string.h>


#include "client_msg.h"
#include <bits/fcntl-linux.h>

// Global variable for message queue ID
int msqid;

pthread_t requestThread[101];

// Function to update an existing graph based on the client message
void updateGraph(struct ClientMessage clientMessage) {
    // Update existing graph file
    char *filePath = clientMessage.graphFileName;

    // Open the graph file for writing
    FILE *file = fopen(filePath, "w");
    int fd = fileno(file);

    if (file == NULL) {
        perror("Failed to open graph file");
    } else {
        // Write new content to the file
        struct GraphData graphData;

        // Get the shared memory key using ftok
        key_t key = ftok("client.c", CLIENT_BUFFER_MTYPE + clientMessage.sequenceNumber);

        // Get the shared memory ID
        int shmid = shmget(key, sizeof(struct GraphData), 0666 | IPC_CREAT);

        // Attach to the shared memory
        void *shmAddr = shmat(shmid, NULL, 0);
        if (shmAddr == (void *)-1) {
            perror("shmat");
            exit(EXIT_FAILURE);
        }

        // Copy data from shared memory to local struct
        memcpy(&graphData, shmAddr, sizeof(struct GraphData));

        // Acquire an exclusive lock for writing
        if (flock(fd, LOCK_EX) == -1) {
            perror("Error acquiring lock");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        // Write graph size to the file
        fprintf(file, "%d\n", graphData.size);

        // Write graph data to the file
        for (int i = 0; i < graphData.size; i++) {
            for (int j = 0; j < graphData.size; j++) {
                fprintf(file, "%d ", graphData.array[i][j]);
            }
            fprintf(file, "\n");
        }

        // Release the lock
        if (flock(fd, LOCK_UN) == -1) {
            perror("Error releasing lock");
        }

        // Close the file
        fclose(file);
    }
}

void* handleRequest(void* args){
    struct ClientMessage* clientMessage = (struct ClientMessage*)args;

    // Print the graph file name received in the client message
    printf("Received graphFileName: %s\n", clientMessage->graphFileName);

    // Check the operation type
    if (clientMessage->operationNumber == 1) {
        // Make a new graph (operation not implemented in this example)
        updateGraph(*clientMessage);
    } else if (clientMessage->operationNumber == 2) {
        // Update an existing graph based on the client message
        updateGraph(*clientMessage);
    }

    struct WriteResultMessage writeResultMessage;
    writeResultMessage.mtype=CLIENT_BUFFER_MTYPE+clientMessage->sequenceNumber;
    if(clientMessage->operationNumber==1){
        const char* messageText = "File successfully added";
        strcpy(writeResultMessage.res, messageText);
    }else{
        const char* messageText = "File successfully modified";
        strcpy(writeResultMessage.res, messageText);
    }
    msgsnd(msqid, &writeResultMessage, sizeof(struct WriteResultMessage), 0);
    free(clientMessage);

    return NULL;
}

int main() {

    int requestCounter=0;
    // Get the message queue ID
    msqid = msgget(MSG_QUEUE_ID, IPC_CREAT | 0666);
    if (msqid == -1) {
        perror("msgget failed for primary server");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Receive a client message
        struct ClientMessage* clientMessage = (struct ClientMessage*)malloc(sizeof(struct ClientMessage));
        printf("Listening...\n");

        // Receive the client message from the message queue
        size_t bytesReceived = msgrcv(msqid, clientMessage, sizeof(struct ClientMessage), PRIMARY_SERVER_MTYPE, 0);
        if (bytesReceived == -1) {
            perror("msgrcv failed");
            exit(EXIT_FAILURE);
        }

        if(clientMessage->operationNumber==5){
            //termination message
            break;
        }

        if (pthread_create(&requestThread[requestCounter], NULL, handleRequest, (void *)clientMessage) != 0) {
            perror("Error creating thread");
            free(clientMessage);  // Free the allocated memory for the message
        }else{
            requestCounter++;
        }
    }

    //wait for all threads to terminate
    for(int i=0; i<requestCounter; i++){
        pthread_join(requestThread[i], NULL);
    }

    //inform termination to load balancer
    struct ClientMessage* deletedMessage = (struct ClientMessage*)malloc(sizeof(struct ClientMessage));
    deletedMessage->mtype=1;

    msgsnd(msqid, deletedMessage, sizeof(deletedMessage), 0);


    return 0;
}
