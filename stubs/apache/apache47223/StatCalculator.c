#include <stdio.h>
#include <malloc.h>
#include <assert.h>

//./os_cpu/windows_x86/vm/copy_windows_x86.inline.hpp
//Linux version uses assembly language
static void pd_conjoint_oops_atomic(int *from, int *to, size_t count) {
    // Do better than this: inline memmove body  NEEDS CLEANUP
    if (from > to) {
        while (count-- > 0) {
            // Copy forwards
            *to++ = *from++;
        }
    } else {
        from += count - 1;
        to += count - 1;
        while (count-- > 0) {
            // Copy backwards
            *to-- = *from--;
        }
    }
}

//jdk/openjdk7/hotspot/src/share/vm/oops/objArrayKlass.cpp
void do_copy(int *s, int *src, int *d, int *dst, int length) {
    pd_conjoint_oops_atomic(src, dst, length);
}


void copy_array(int *s, int src_pos, int *d, int dst_pos, int length) {
    if (src_pos < 0 || dst_pos < 0 || length < 0) {
        assert(0);
    }

    if (length == 0) {
        return;
    }

    int *src = s + src_pos;
    int *dst = d + dst_pos;
    do_copy(s, src, d, dst, length);
}


//./openjdk7/jdk/src/share/classes/java/util/Collections.java
int binarySearch(int *list, int key, int count) {
    int low = 0;
    int high = count - 1;

    while (low <= high) {
        int mid = (low + high) >> 1;

        if (list[mid] < key)
            low = mid + 1;
        else if (list[mid] > key)
            high = mid - 1;
        else
            return mid;
    }

    return -(low + 1);
}


//./openjdk7/jdk/src/share/classes/java/util/ArrayList.java
typedef struct stArrayList {
    int *elementData;
    int size;
    int capacity;
} ArrayList;


void InitArrayList(ArrayList *pAL, int initialCapacity) {
    pAL->elementData = (int *) malloc(sizeof(int) * initialCapacity);
    pAL->capacity = initialCapacity;
    pAL->size = 0;
}

void rangeCheckForAdd(ArrayList *pAL, int index) {
    assert(index <= pAL->size && index >= 0);
}

void ensureCapacity(ArrayList *pAL, int minCapacity) {
    int oldCapacity = pAL->capacity;
    if (minCapacity > oldCapacity) {
        int *oldData = pAL->elementData;
        int newCapacity = (oldCapacity * 3) / 2 + 1;
        if (newCapacity < minCapacity)
            newCapacity = minCapacity;
        int *newData = (int *) malloc(sizeof(int) * newCapacity);
        copy_array(oldData, 0, newData, 0, pAL->size);
        pAL->elementData = newData;
        pAL->capacity = newCapacity;
        free(oldData);
    }
}


void add(ArrayList *pAL, int index, int element) {
    rangeCheckForAdd(pAL, index);
    ensureCapacity(pAL, pAL->size + 1);
    copy_array(pAL->elementData, index, pAL->elementData, index + 1, pAL->size - index);
    pAL->elementData[index] = element;
    pAL->size++;
}

void add0(ArrayList *pAL, int element) {
    pAL->elementData[pAL->size++] = element;
}

//jmeter47723_BUG/src/jorphan/org/apache/jorphan/math/StatCalculator.java
typedef struct stStatCalculator {
    ArrayList values;
} StatCalculator;


void addSortedValue(StatCalculator *stat, int val) {
    int index = binarySearch(stat->values.elementData, val, stat->values.size);
    if (index >= 0 && index < stat->values.size) {
        add(&(stat->values), index, val);
    } else if (index == stat->values.size || stat->values.size == 0) {
        add0(&(stat->values), val);
    } else {
        add(&(stat->values), index * (-1) - 1, val);
    }

}

void print(StatCalculator *pStat) {
    int iIndex;
    for (iIndex = 0; iIndex < pStat->values.size; iIndex++) {
        printf("%d ", pStat->values.elementData[iIndex]);
    }

    printf("\n");
}


void test(int iNum) {
    StatCalculator stat;
    InitArrayList(&(stat.values), 10);
    int i;
    for (i = 0; i < iNum; i++) {
        int iTmp = rand() % 100000;
        addSortedValue(&stat, iTmp);
    }
    //print(&stat);
}

int main(int argc, char **argv) {

    if (argc != 2) {
        printf("parameter number is wrong\n");
        return -1;
    }
    int iInsert = 0;
    sscanf(argv[1], "%d", &iInsert);

    test(iInsert);
    return 0;
}
