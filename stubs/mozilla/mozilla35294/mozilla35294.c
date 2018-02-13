#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>

//mozilla-source-0.9.6/dbm/src/memmove.c
typedef int word;
#define    wsize    sizeof(word)
#define    wmask    (wsize - 1)


void mozilla35294_memmove(void *dst0, const void *src0, size_t length) {
    char *dst = dst0;
    const char *src = src0;
    size_t t;

    if (length == 0 || dst == src)
        goto done;

#undef    TLOOP
#define    TLOOP(s) if (t) TLOOP1(s)
#undef    TLOOP1
#define    TLOOP1(s) do { s; } while (--t)
    if ((unsigned long) dst < (unsigned long) src) {
        t = (size_t) src;    /* only need low bits */
        if ((t | (size_t) dst) & wmask) {
            if ((t ^ (size_t) dst) & wmask || length < wsize)
                t = length;
            else
                t = wsize - (t & wmask);
            length -= t;
            TLOOP1(*dst++ = *src++);
        }
        t = length / wsize;
        TLOOP(*(word *) dst = *(word *) src;
                      src += wsize;
                      dst += wsize);
        t = length & wmask;
        TLOOP(*dst++ = *src++);
    } else {
        src += length;
        dst += length;
        t = (size_t) src;
        if ((t | (size_t) dst) & wmask) {
            if ((t ^ (size_t) dst) & wmask || length <= wsize)
                t = length;
            else
                t &= wmask;
            length -= t;
            TLOOP1(*--dst = *--src);
        }
        t = length / wsize;
        TLOOP(src -= wsize;
                      dst -= wsize;
                      *(word *) dst = *(word *) src);
        t = length & wmask;
        TLOOP(*--dst = *--src);
    }

    done:
    return;
}


//mozilla-source-0.9.6/xpcom/ds/nsVoidArray.cpp
typedef struct stImpl {
    unsigned int mBits;
    unsigned int mCount;
    int *mArray;
} Impl;


typedef struct stnsVoidArray {

    Impl *mImpl;
} nsVoidArray;


int nsVoidArray_Count(nsVoidArray *pArray) {
    return pArray->mImpl ? pArray->mImpl->mCount : 0;
}

bool nsVoidArray_RemoveElementsAt(nsVoidArray *pArray, int aIndex, int aCount) {
    int oldCount = nsVoidArray_Count(pArray);
    if (aIndex >= oldCount) {
        // An invalid index causes the replace to fail
        return false;
    }

    if (aCount + aIndex > oldCount)
        aCount = oldCount - aIndex;

    if (aIndex < (oldCount - aCount)) {
        mozilla35294_memmove(pArray->mImpl->mArray + aIndex, pArray->mImpl->mArray + aIndex + aCount,
                             (oldCount - (aIndex + aCount)) * sizeof(pArray->mImpl->mArray[0]));
    }

    pArray->mImpl->mCount -= aCount;
    return true;
}

bool nsVoidArray_RemoveElementAt(nsVoidArray *pArray, int aIndex) {
    return nsVoidArray_RemoveElementsAt(pArray, aIndex, 1);
}

//mozilla-source-0.9.6/content/base/src/nsGenericElement.cpp
typedef struct stnsCheapVoidArray {
    void *mChildren;
} nsCheapVoidArray;


nsVoidArray *nsCheapVoidArray_GetChildVector(nsCheapVoidArray *pArray) {
    return (nsVoidArray *) (pArray->mChildren);
}


bool nsCheapVoidArray_RemoveElementAt(nsCheapVoidArray *pArray, int aIndex) {
    nsVoidArray *vector = nsCheapVoidArray_GetChildVector(pArray);
    if (vector) {
        return nsVoidArray_RemoveElementAt(vector, aIndex);
    }
    return false;
}


typedef struct stnsGenericElement {
    nsCheapVoidArray mChildren;
} nsGenericElement;


void nsGenericElement_RemoveChildAt(nsGenericElement *pElement, int aIndex) {
    nsCheapVoidArray_RemoveElementAt(&(pElement->mChildren), aIndex);
}

void nsGenericElement_doReplaceChild(nsGenericElement *pElement, int count) {

    int i;
    for (i = 0; i < count; i++) {
        nsGenericElement_RemoveChildAt(pElement, 0);
    }
}

void InitializensGenericElement(nsGenericElement *pElement, int size) {
    pElement->mChildren.mChildren = (nsVoidArray *) malloc(sizeof(nsVoidArray));
    ((nsVoidArray *) pElement->mChildren.mChildren)->mImpl = (Impl *) malloc(sizeof(Impl));
    ((nsVoidArray *) pElement->mChildren.mChildren)->mImpl->mArray = (int *) malloc(sizeof(int) * size);

    int i = 0;
    for (; i < size; i++) {
        ((nsVoidArray *) pElement->mChildren.mChildren)->mImpl->mArray[i] = i;
    }

    ((nsVoidArray *) pElement->mChildren.mChildren)->mImpl->mCount = size;

}

void DestroyGenericElement(nsGenericElement *pElement) {
    free(((nsVoidArray *) pElement->mChildren.mChildren)->mImpl->mArray);
    free(((nsVoidArray *) pElement->mChildren.mChildren)->mImpl);
    free(pElement->mChildren.mChildren);
}

void DumpGenericElement(nsGenericElement *pElement) {
    int i = 0;
    int size = ((nsVoidArray *) pElement->mChildren.mChildren)->mImpl->mCount;

    for (; i < size; i++) {
        printf("%d ", ((nsVoidArray *) pElement->mChildren.mChildren)->mImpl->mArray[i]);
    }

    printf("\n");
}


int main(int argc, char **argv) {
    if (argc != 3) {
        printf("parameter number is wrong\n");
        return -1;
    }

    int size;
    int count;
    sscanf(argv[1], "%d", &size);
    sscanf(argv[2], "%d", &count);

    printf("%d\n", size);
    printf("%d\n", count);

    nsGenericElement element;
    InitializensGenericElement(&element, size);
    nsGenericElement_doReplaceChild(&element, count);
    DumpGenericElement(&element);
    DestroyGenericElement(&element);

    return 0;
}
