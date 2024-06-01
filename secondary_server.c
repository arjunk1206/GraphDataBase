#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <pthread.h>
#include <fcntl.h>

// #define LOCK_SH 1   /* Shared lock */
// #define LOCK_EX 2   /* Exclusive lock */
// #define LOCK_UN 8   /* Unlock */

#include "client_msg.h"
#include "bfs.h"

// Global variable for message queue ID
int msqid;

pthread_t requestThread[101];

struct ResultMessage result[100];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *DFS_Thread(void *arg)
{
    struct ThreadData *data = (struct ThreadData *)arg;
    int vertex = data->startVertex;
    int isLeaf = 1;
    pthread_t thread[256];
    int numthreads = 0;
    for (int i = 0; i < data->graph->size; ++i)
    {
        if (data->graph->array[vertex][i])
        {
            if (data->visited[i] == 0)
            {
                isLeaf = 0;
                data->visited[i] = 1;
                struct ThreadData *newData = (struct ThreadData *)malloc(sizeof(struct ThreadData));
                newData->sequenceNumber = data->sequenceNumber;
                newData->startVertex = i;
                newData->graph = data->graph;
                memcpy(newData->visited, data->visited, sizeof(data->visited));
                pthread_create(&thread[numthreads], NULL, DFS_Thread, newData);
                numthreads++;
            }
        }
    }
    for (int i = 0; i < numthreads; i++)
    {
        pthread_join(thread[i], NULL);
    }
    if (isLeaf)
    {
        pthread_mutex_lock(&mutex);
        int pos = result[data->sequenceNumber].counter;
        result[data->sequenceNumber].res[pos] = vertex;
        result[data->sequenceNumber].counter++;
        pthread_mutex_unlock(&mutex);
    }

    free(data);
    pthread_exit(NULL);
}

// Function to update an existing graph based on the client message
void dfs(struct ClientMessage clientMessage)
{
    // Update existing graph file
    char *filePath = clientMessage.graphFileName;
    struct GraphData graph;

    // Open the graph file for writing
    FILE *file = fopen(filePath, "r");

    if (file == NULL)
    {
        perror("Failed to open graph file");
    }
    else
    {

        // Obtain the file descriptor from the file pointer
        int fd = fileno(file);

        // Acquire an shareable lock for reading
        if (flock(fd, LOCK_SH) == -1)
        {
            perror("Error acquiring lock");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        fscanf(file, "%d", &graph.size);

        for (int i = 0; i < graph.size; i++)
        {
            for (int j = 0; j < graph.size; j++)
            {
                fscanf(file, "%d", &graph.array[i][j]);
            }
        }

        // Release the lock
        if (flock(fd, LOCK_UN) == -1)
        {
            perror("Error releasing lock");
        }

        // Close the file
        fclose(file);

        // Write new content to the file
        struct GraphReadData graphReadData;

        // Get the shared memory key using ftok
        key_t key = ftok("client.c", CLIENT_BUFFER_MTYPE + clientMessage.sequenceNumber);

        // Get the shared memory ID
        int shmid = shmget(key, sizeof(struct GraphReadData), 0666 | IPC_CREAT);

        // Attach to the shared memory
        void *shmAddr = shmat(shmid, NULL, 0);
        if (shmAddr == (void *)-1)
        {
            perror("shmat");
            exit(EXIT_FAILURE);
        }

        // Copy data from shared memory to local struct
        memcpy(&graphReadData, shmAddr, sizeof(struct GraphReadData));

        printf("start: %d\n", graphReadData.startVertex);

        struct ThreadData *threadData = (struct ThreadData *)malloc(sizeof(struct ThreadData));
        threadData->startVertex = graphReadData.startVertex;
        threadData->graph = &graph;
        memset(&threadData->visited, 0, MAX_SIZE*sizeof(int));
        threadData->visited[threadData->startVertex] = 1;
        threadData->sequenceNumber = clientMessage.sequenceNumber;

        pthread_t thread;
        pthread_create(&thread, NULL, DFS_Thread, threadData);
        pthread_join(thread, NULL);
        printf("done\n");
    }
}

void bfs(int n, int src, int arr[MAX_SIZE][MAX_SIZE], int sno)
{
    // void* bfs(struct ClientMessage clientMessage){
    struct Queue *queue = (struct Queue *)malloc(sizeof(struct Queue));
    queue->len = 0;
    queue->head = NULL;
    queue->tail = NULL;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    queue->qmutex = &mutex;
    push(queue, src);
    int *visited = (int *)malloc(n * sizeof(int));
    memset(visited, 0, n * sizeof(int));
    visited[src] = 1;
    while (queue->len != 0)
    {
        printf("lvl\n");
        pthread_t thread[256];
        int numthreads = 0;
        int sz = queue->len;
        for (int i = 0; i < sz; i++)
        {
            int node = pop(queue);

            pthread_mutex_lock(&mutex);
            int pos = result[sno].counter;
            result[sno].res[pos] = node;
            result[sno].counter++;
            pthread_mutex_unlock(&mutex);

            printf("node: %d\n", node);
            struct BfsData *bfsData = (struct BfsData *)malloc(sizeof(struct BfsData));
            bfsData->node = node;
            bfsData->queue = queue;
            bfsData->visited = visited;
            bfsData->graph = (struct GraphData *)malloc(sizeof(struct GraphData));
            bfsData->graph->size = n;
            for (int r = 0; r < n; r++)
            {
                for (int c = 0; c < n; c++)
                {
                    bfsData->graph->array[r][c] = arr[r][c];
                }
            }
            pthread_create(&thread[numthreads], NULL, nodeProcessor, bfsData);
            numthreads++;
        }
        for (int i = 0; i < numthreads; i++)
        {
            pthread_join(thread[i], NULL);
        }
    }
}

void readcommon(struct ClientMessage clientMessage)
{
    char *filePath = clientMessage.graphFileName;
    struct GraphData graph;

    // Open the graph file for writing
    FILE *file = fopen(filePath, "r");

    if (file == NULL)
    {
        perror("Failed to open graph file");
    }
    else
    {

        // Obtain the file descriptor from the file pointer
        int fd = fileno(file);

        // Acquire an exclusive lock for writing
        if (flock(fd, LOCK_SH) == -1)
        {
            perror("Error acquiring lock");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        fscanf(file, "%d", &graph.size);

        for (int i = 0; i < graph.size; i++)
        {
            for (int j = 0; j < graph.size; j++)
            {
                fscanf(file, "%d", &graph.array[i][j]);
            }
        }

        // Release the lock
        if (flock(fd, LOCK_UN) == -1)
        {
            perror("Error releasing lock");
        }

        // Close the file
        fclose(file);

        // Write new content to the file
        struct GraphReadData graphReadData;

        // Get the shared memory key using ftok
        key_t key = ftok("client.c", CLIENT_BUFFER_MTYPE + clientMessage.sequenceNumber);

        // Get the shared memory ID
        int shmid = shmget(key, sizeof(struct GraphReadData), 0666 | IPC_CREAT);

        // Attach to the shared memory
        void *shmAddr = shmat(shmid, NULL, 0);
        if (shmAddr == (void *)-1)
        {
            perror("shmat");
            exit(EXIT_FAILURE);
        }

        // Copy data from shared memory to local struct
        memcpy(&graphReadData, shmAddr, sizeof(struct GraphReadData));

        printf("start: %d\n", graphReadData.startVertex);

        bfs(graph.size, graphReadData.startVertex, graph.array, clientMessage.sequenceNumber);
    }
}

void *handleRequest(void *args)
{
    struct ClientMessage *clientMessage = (struct ClientMessage *)args;

    // Print the graph file name received in the client message
    printf("Received graphFileName: %s\n", clientMessage->graphFileName);

    // Check the operation type
    if (clientMessage->operationNumber == 3)
    {
        result[clientMessage->sequenceNumber].counter = 0;
        dfs(*clientMessage);
        result[clientMessage->sequenceNumber].mtype = CLIENT_BUFFER_MTYPE + clientMessage->sequenceNumber;
        if (msgsnd(msqid, &result[clientMessage->sequenceNumber], sizeof(struct ResultMessage), 0) == -1)
        {
            perror("msgsnd");
            exit(EXIT_FAILURE);
        }
    }
    else if (clientMessage->operationNumber == 4)
    {
        result[clientMessage->sequenceNumber].counter = 0;
        readcommon(*clientMessage);
        result[clientMessage->sequenceNumber].mtype = CLIENT_BUFFER_MTYPE + clientMessage->sequenceNumber;
        if (msgsnd(msqid, &result[clientMessage->sequenceNumber], sizeof(struct ResultMessage), 0) == -1)
        {
            perror("msgsnd");
            exit(EXIT_FAILURE);
        }
    }

    free(clientMessage);
    return NULL;
}

int main()
{

    int requestCounter=0;
    int servernumber;
    printf("Enter secondary server number (1 or 2): ");
    scanf("%d", &servernumber);
    int listenOn = 4; //mtype for request
    if (servernumber == 1)
    {
        listenOn = 3;
    }

    // Get the message queue ID
    msqid = msgget(MSG_QUEUE_ID, IPC_CREAT | 0666);
    if (msqid == -1)
    {
        perror("msgget failed for primary server");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        // Receive a client message
        struct ClientMessage *clientMessage = (struct ClientMessage *)malloc(sizeof(struct ClientMessage));


        printf("Listening...%d\n", listenOn);
        // Receive the client message from the message queue
        size_t bytesReceived = msgrcv(msqid, clientMessage,
                                      sizeof(struct ClientMessage), listenOn, 0);
        if (bytesReceived == -1)
        {
            perror("msgrcv failed");
            exit(EXIT_FAILURE);
        }

        if(clientMessage->operationNumber==5){
            break;
        }

        if (pthread_create(&requestThread[requestCounter], NULL, handleRequest, (void *)clientMessage) != 0)
        {
            perror("Error creating thread");
            free(clientMessage); // Free the allocated memory for the message
        }
        else
        {
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
