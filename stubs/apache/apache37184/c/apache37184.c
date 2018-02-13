#include <stdlib.h>
#include <stdio.h>

typedef int bool;
#define true 1
#define false 0

typedef struct stIntrospectionHelper {
    void *id;
    //int iID;
} IntrospectionHelper;

typedef struct stBuildEvent {
    void *id;

} BuildEvent;

void IntrospectionHelper_messageLogged(IntrospectionHelper *pHelper, BuildEvent *pEvent) {
    printf("Linster: %p Event: %p\n", pHelper, pEvent);
}

IntrospectionHelper **listeners;
IntrospectionHelper *reallisteners;
int iEchoTest;


void CreateLinster() {
    listeners = (IntrospectionHelper **) malloc(sizeof(IntrospectionHelper *) * (iEchoTest * 3 + 1));
    reallisteners = (IntrospectionHelper *) malloc(sizeof(IntrospectionHelper) * 3);

    int i, j;
    for (i = 0; i < 3; i++) {
        reallisteners[i].id = &reallisteners[i];
        //reallisteners[i].iID = i;
    }

    for (i = 0; i < iEchoTest; i++) {
        for (j = 0; j < 3; j++) {
            listeners[i * 3 + j + 1] = &reallisteners[j];
        }
    }

}


void DestroyLinster() {
    free(reallisteners);
    free(listeners);
}

bool hasNext(void *p) {
    if (p < (void *) (listeners) + sizeof(IntrospectionHelper *) * iEchoTest * 3) {
        return true;
    }

    return false;
}

void *next(void *p) {
    return p + sizeof(IntrospectionHelper *);
}

void fireMessageLoggedEvent(BuildEvent *event) {
    void *iter = listeners;
    while (hasNext(iter)) {
        iter = next(iter);
        IntrospectionHelper *listener = *((IntrospectionHelper **) iter);
        IntrospectionHelper_messageLogged(listener, event);
    }
}


int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Parameter Number Is Wrong!\n");
        exit(-1);
    }

    iEchoTest = atoi(argv[1]);

    CreateLinster();

    int i;
    BuildEvent *event = (BuildEvent *) malloc(sizeof(BuildEvent));
    event->id = event;

    for (i = 0; i < 1; i++) {
        fireMessageLoggedEvent(event);
    }

    DestroyLinster();
}
