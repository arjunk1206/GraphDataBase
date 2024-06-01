#ifndef CLIENT_MSG_H
#define CLIENT_MSG_H

#define MAX_SIZE 256
#define CLIENT_BUFFER_MTYPE 4
#define PRIMARY_SERVER_MTYPE 2
#define SECONDARY_SERVER_ODD_MTYPE 3
#define SECONDARY_SERVER_EVEN_MTYPE 4
#define MSG_QUEUE_ID 12345

#define TERMINATION_SEQUENCE 200
#define TERMINATION_OPERATION 5


struct ClientMessage {
    long mtype;
    int operationNumber;
    int sequenceNumber;
    char graphFileName[MAX_SIZE];
};

struct GraphData {
    int size;
    int array[MAX_SIZE][MAX_SIZE];
};

struct GraphReadData {
    int startVertex;
    char msg[MAX_SIZE];
};


struct ThreadData{
    int sequenceNumber;
    struct GraphData* graph;
    int startVertex;
    int visited[MAX_SIZE];
};
//DFS BFS Traversal Result
struct ResultMessage{
    long mtype;
    int res[MAX_SIZE];
    int counter;
};

//Writing output for Adding and Modifying Files
struct WriteResultMessage{
    long mtype;
    char res[MAX_SIZE];
};

#endif /* CLIENT_MESSAGE_H */