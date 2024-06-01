#ifndef BFS_H
#define BFS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include<time.h>
#include <pthread.h>
#include "client_msg.h"

int msqid;
// pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


struct BfsData{
    struct Queue* queue;
    struct GraphData* graph;
    int node;
    int * visited;
};

struct Node{
    struct Node* next;
    int val;
};

struct Queue{
    struct Node* head;
    struct Node* tail;
    pthread_mutex_t* qmutex;
    int len;
};

void push(struct Queue* ll, int val){
    struct Node* n = (struct Node*)malloc(sizeof(struct Node*));
    n->val=val;
    n->next=NULL;

    pthread_mutex_lock(ll->qmutex);
    if(ll->head){
        ll->tail->next=n;
        ll->tail=n;
    }
    else{
        ll->head=n;
        ll->tail=n;
    }
    ll->len++;
    pthread_mutex_unlock(ll->qmutex);
}

int pop(struct Queue* ll){
    pthread_mutex_lock(ll->qmutex);
    struct Node* n = ll->head;
    int ans=n->val;
    ll->head=ll->head->next;
    ll->len--;
    pthread_mutex_unlock(ll->qmutex);
    free(n);
    return ans;
}

void* nodeProcessor(void* args){
    struct BfsData* bfsData = (struct BfsData*)args;
    int cur=bfsData->node;
    for(int i=0; i<bfsData->graph->size; i++){
        if(bfsData->graph->array[cur][i]==1){
            if(bfsData->visited[i]==0){
                bfsData->visited[i]=1;
                printf("cur node: %d, adj node: %d\n", cur, i);
                push(bfsData->queue,i);
            }
        }
    }
    pthread_exit(NULL);
}


void changer(struct Node* a){
    sleep(1);
    a->val=1;
}

// void printer(struct Node* a){
//     sleep(2);
//     printf("%d", a->val);
// }

// int main(){
//     // int **arrarg=(int**)malloc(sizeof(5*sizeof(int*)));
//     // for(int i=0; i<5; i++){
//     //     arrarg[i]=(int*)malloc(5*sizeof(int));
//     // }
//     int arr[5][5]={
//         {0,1,0,0,0},
//         {1,0,1,1,1},
//         {0,1,0,0,0},
//         {0,1,0,0,0},
//         {0,1,0,0,0}
//     };

//     // memcpy(arrarg, &arr, arr);
//     printf("bfs\n");
//     bfs(5, 0, arr);
// }

#endif
