#include "../apilib.h"

typedef struct NodeChar
{
    char data;
    struct NodeChar* next;
}StackChar;

StackChar* InitStackChar()
{
    StackChar* stackHead = (StackChar*)api_malloc(sizeof(StackChar));
    stackHead->next = (void*)0;
    return stackHead;
}

void PushChar(StackChar* stackHead, char data) {
    StackChar* node = (StackChar*)api_malloc(sizeof(StackChar));
    node->data = data;
    node->next = stackHead->next;
    stackHead->next = node;
}

char PopChar(StackChar* stackHead) {
    StackChar* topNode = stackHead->next;
    char topData = topNode->data;
    stackHead->next = topNode->next;
    api_free(topNode, sizeof(StackChar));
    return topData;
}

char TopChar(StackChar* stackHead)
{
    StackChar* topNode = stackHead->next;
    char topData = topNode->data;
    return topData;
}
