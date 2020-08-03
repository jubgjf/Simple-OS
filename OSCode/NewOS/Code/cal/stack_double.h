#include "../apilib.h"

typedef struct NodeDouble
{
    double data;
    struct NodeDouble* next;
}StackDouble;

StackDouble* InitStackDouble()
{
    StackDouble* stackHead = (StackDouble*)api_malloc(sizeof(StackDouble));
    stackHead->next = (void*)0;
    return stackHead;
}

void PushDouble(StackDouble* stackHead, double data) {
    StackDouble* node = (StackDouble*)api_malloc(sizeof(StackDouble));
    node->data = data;
    node->next = stackHead->next;
    stackHead->next = node;
}

double PopDouble(StackDouble* stackHead) {
    StackDouble* topNode = stackHead->next;
    double topData = topNode->data;
    stackHead->next = topNode->next;
    api_free(topNode, sizeof(StackDouble));
    return topData;
}

double TopDouble(StackDouble* stackHead)
{
    StackDouble* topNode = stackHead->next;
    double topData = topNode->data;
    return topData;
}
