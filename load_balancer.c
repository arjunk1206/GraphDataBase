#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include "client_msg.h"

#define MAX_MESSAGE_SIZE 256

void route_to_server(struct ClientMessage *client_msg, int msg_id)
{

    // Implement logic to send the message to the server

    if (msgsnd(msg_id, client_msg, sizeof(struct ClientMessage), 0) == -1)
    {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }

    printf("Routing to Server: Mtype=%d, Sequence %d, Operation %d, Graph File: %s\n",
           client_msg->mtype, client_msg->sequenceNumber, client_msg->operationNumber, client_msg->graphFileName);
    // Add your logic to send the message to the primary server
}

int main()
{
    key_t key;
    int msg_id;

    // Create a message queue
    msg_id = msgget(MSG_QUEUE_ID, IPC_CREAT | 0666);

    if (msg_id == -1)
    {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    // Simulate receiving messages from clients
    while (1)
    {
        struct ClientMessage client_msg;
        // Receive message from the client
        msgrcv(msg_id, &client_msg, sizeof(struct ClientMessage), 1, 0);
        // Route messages based on the operation number
        if (client_msg.operationNumber == 1 || client_msg.operationNumber == 2)
        {
            client_msg.mtype = PRIMARY_SERVER_MTYPE;
            route_to_server(&client_msg, msg_id);
        }
        else if (client_msg.operationNumber == 3 || client_msg.operationNumber == 4)
        {
            if (client_msg.sequenceNumber % 2 == 1)
            {
                client_msg.mtype = 3;
            }
            else
            {
                client_msg.mtype = 4;
            }
            route_to_server(&client_msg, msg_id);
        }
        else if (client_msg.operationNumber == 5)
        {
            struct ClientMessage clientAck;
            client_msg.mtype = 2;
            route_to_server(&client_msg, msg_id);
            msgrcv(msg_id, &clientAck, sizeof(struct ClientMessage), 1, 0);
            printf("Primary Server Closed\n");
            client_msg.mtype = 3;
            route_to_server(&client_msg, msg_id);
            msgrcv(msg_id, &clientAck, sizeof(struct ClientMessage), 1, 0);
            printf("Secondary Server 1 Closed\n");
            client_msg.mtype = 4;
            route_to_server(&client_msg, msg_id);
            msgrcv(msg_id, &clientAck, sizeof(struct ClientMessage), 1, 0);
            printf("Secondary Server 2 Closed\n");

            sleep(5);
            msgctl(MSG_QUEUE_ID, IPC_RMID, NULL);
            return 0;
        }
        else
        {
            // Handle unsupported operation numbers
            printf("Unsupported Operation Number: %d\n", client_msg.operationNumber);
        }

        // Simulate processing time before receiving the next message
        sleep(1);
    }

    return 0;
}