///t/ermination Mtype - 200
///send prompt to load balancer
//operationNumber = 5 for cleanup

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include "client_msg.h"

int msqid;

int main(){
    
    msqid = msgget(MSG_QUEUE_ID, IPC_CREAT | 0666);

    if (msqid == -1) {
        perror("msgget failed for cleanup");
        exit(EXIT_FAILURE);
    }
    
    while(1){
        
        struct ClientMessage delete_msg;
        printf("Do you wish to terminate [Y|N]?: ");
        char response;
        scanf(" %c",&response);



        if(response == 'Y' || response == 'y')
        {
            delete_msg.mtype = 1;
            delete_msg.sequenceNumber = TERMINATION_SEQUENCE;
            delete_msg.operationNumber = TERMINATION_OPERATION;
            if (msgsnd(msqid, &delete_msg, sizeof(delete_msg), 0) == -1)
            {
                perror("msgsnd");
                exit(EXIT_FAILURE);
            }

            printf("Termination Request Sent\n");
            break;
            
        }
        else{
            continue;
        }

    }
}