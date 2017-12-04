#include <stdio.h>
#include <malloc.h>
#include <assert.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>


#define BUFFERSIZE 1 << 20


//./os_cpu/windows_x86/vm/copy_windows_x86.inline.hpp
//Linux version uses assembly language
static void pd_conjoint_oops_atomic(char * from, char * to, size_t count) {
  // Do better than this: inline memmove body  NEEDS CLEANUP
  if (from > to) {
    while (count-- > 0) {
      // Copy forwards
      *to++ = *from++;
    }
  } else {
    from += count - 1;
    to   += count - 1;
    while (count-- > 0) {
      // Copy backwards
      *to-- = *from--;
    }
  }
}

//jdk/openjdk7/hotspot/src/share/vm/oops/objArrayKlass.cpp
void do_copy(char * s, char * src, char * d, char * dst, int length)
{
  pd_conjoint_oops_atomic(src, dst, length);
}

void copy_array(char * s, int src_pos, char * d, int dst_pos, int length)
{
  if( src_pos < 0 || dst_pos < 0 || length < 0)
  {
    assert(0);
  }

  if(length == 0 )
  {
    return;
  }

  char * src = s + src_pos;
  char * dst = d + dst_pos;
  do_copy(s, src, d, dst, length);
}


typedef struct stStringBuffer
{
  char * value;
  int count;   //current length
  int length;  //capacity
} StringBuffer;

//./jdk/src/share/classes/java/lang/AbstractStringBuilder.java
void expandCapacity(StringBuffer * pBuffer, int minimumCapacity) {
  int newCapacity = (pBuffer->length + 1) * 2;
  if (newCapacity < 0) {
    newCapacity = INT_MAX;
  } else if (minimumCapacity > newCapacity) {
    newCapacity = minimumCapacity;
  }
  char * newData = (char *)malloc(sizeof(char) * newCapacity);
  char * oldData = pBuffer->value;
  copy_array(oldData, 0, newData, 0, pBuffer->count);
  pBuffer->value = newData;
  free(oldData);
}

void InitStringBuffer(StringBuffer * pBuffer)
{
  pBuffer->value = (char *)malloc(sizeof(char) * BUFFERSIZE);
  pBuffer->count = 0;
  pBuffer->length = BUFFERSIZE;
}

void append(StringBuffer * pBuffer, char c)
{
  int newCount = pBuffer->count + 1;
  if (newCount > pBuffer->length)
    expandCapacity(pBuffer, newCount);

  pBuffer->value[pBuffer->count++] = c;
}

//./jdk/src/share/classes/java/lang/String.java
static int indexOf(char* source, int sourceOffset, int sourceCount, char * target, int targetOffset, int targetCount, int fromIndex) 
{
  if (fromIndex >= sourceCount) {
    return (targetCount == 0 ? sourceCount : -1);
  }
  if (fromIndex < 0) {
    fromIndex = 0;
  }
  if (targetCount == 0) {
    return fromIndex;
  }

  char first  = target[targetOffset];
  int max = sourceOffset + (sourceCount - targetCount);
  for (int i = sourceOffset + fromIndex; i <= max; i++) {
    if (source[i] != first) {
      while (++i <= max && source[i] != first);
    }

    if (i <= max) {
      int j = i + 1;
      int end = j + targetCount - 1;
      for (int k = targetOffset + 1; j < end && source[j] ==
               target[k]; j++, k++);

      if (j == end) {
        return i - sourceOffset;
      }
    }
  }
  return -1;
}

static int indexOf00(StringBuffer * pBuffer, char * target, int iLength)
{
  int iTmp = indexOf(pBuffer->value, 0, pBuffer->count, target, 0, iLength, 0);
  //printf("%d\n", iTmp);
  return iTmp;
}



//ant34464_BUG/src/main/org/apache/tools/ant/taskdefs/optional/net/TelnetTask.java
typedef struct stTelnetTask
{ 
   FILE *fp;
   StringBuffer sb;
} TelnetTask;

void InitTelnetTask(TelnetTask * pTelnet)
{
  pTelnet->fp = NULL;
  InitStringBuffer(&(pTelnet->sb));
}

void InitIOStream(TelnetTask * pTelnet, char * s )
{
  pTelnet->fp = fopen(s, "rt");
}

void CloseIOStream(TelnetTask * pTelnet)
{
  fclose(pTelnet->fp);
}

void WaitForString(TelnetTask * pTelnet, char * s, int iLength )
{
  while( indexOf00(&(pTelnet->sb) , s, iLength ) == -1 )
  {
    append( &(pTelnet->sb), (char)fgetc(pTelnet->fp));
  }
}


int main(int argv, char ** argc)
{
  TelnetTask task;
  InitTelnetTask(&task);
  InitIOStream(&task, argc[1]);
  WaitForString(&task, argc[2], strlen(argc[2]));
  CloseIOStream(&task);
  return 0;
}
