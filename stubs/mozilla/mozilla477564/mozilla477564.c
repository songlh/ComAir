#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define RUNNUM 100

char ConstantLocalName[] = "tr";
char ConstantName[] = "tr";
int *data;

typedef enum {
    checkbox,
    radio
} Type;


typedef struct stNode {
    int id;
    char *localName;
    char *name;
    int namespaceURI;
    Type type;
    bool Checked;
    struct stNode *previousSibling;
    struct stNode *nextSibling;
} Node;


Node *createList(int iNum) {

    data = (int *) malloc(sizeof(int) * iNum);
    Node *header = (Node *) malloc(sizeof(Node));
    header->id = -1;
    header->localName = ConstantLocalName;
    header->name = ConstantName;
    header->namespaceURI = 0;
    header->previousSibling = NULL;
    header->nextSibling = NULL;
    header->type = checkbox;
    header->Checked = true;
    Node *pCurrent = header;

    int i = 0;
    for (; i < iNum - 1; i++) {
        Node *tmp = (Node *) malloc(sizeof(Node));
        tmp->localName = ConstantLocalName;
        tmp->name = ConstantName;
        tmp->previousSibling = pCurrent;
        tmp->namespaceURI = 0;
        tmp->id = -1;
        pCurrent->nextSibling = tmp;
        tmp->Checked = true;
        tmp->nextSibling = NULL;
        pCurrent = tmp;
    }

    return header;
}


void DumpList(Node *header, int iNum) {
    int iCount = 0;
    while (header != NULL) {
        header = header->nextSibling;
        iCount++;
    }

    printf("Count: %d\n\n", iCount);
    int i;
    for (i = 0; i < iNum; i++) {
        printf("%d: %d\n", i, data[i]);
    }

}

//firefox-3.5/mozilla-1.9.1/browser/components/sessionstore/src/nsSessionStore.js
int sss_xph_generate(Node *aNode) {
    char *nName = aNode->name;
    Node *n;
    int count = 0;
    for (n = aNode->previousSibling; n != NULL; n = n->previousSibling) {
        if (n->localName == aNode->localName && n->namespaceURI == aNode->namespaceURI &&
            (!nName || n->name == nName))
            count++;

    }

    return count;

}

//
void sss_collectFormDataForFrame(Node *node) {
    do {
        int id = node->id != -1 ? node->id : sss_xph_generate(node);
        data[id] = node->type == checkbox ? node->Checked : 0;
    } while ((node = node->nextSibling));

}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("parameter number is wrong\n");
        exit(-1);
    }

    Node *header = createList(atoi(argv[1]));
    int iNum = 0;
    for (; iNum < RUNNUM; iNum++) {
        sss_collectFormDataForFrame(header);
    }


    DumpList(header, atoi(argv[1]));

}
