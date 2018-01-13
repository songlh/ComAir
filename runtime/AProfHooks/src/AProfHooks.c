#include <stdio.h>

#include "AProfHooks.h"
#include "common.h"
#include "hashMap.h"
#include "inputPro.h"


int aprof_init() {

	int r = createHashMap();
	if (r == 0) {
		printf("create hashMap error!");
		return STATUS_ERROR;
	}

	r = createShadowStack();
	if (r == 0) {
		printf("create shadow stack error!");
		return STATUS_ERROR;
	}

	return STATUS_SUCCESS;

}


void PrintExecutionCost(long cost) 
{
	printf("TOTAL COST: %ld\n", cost);
}
