#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hash_file.h"
#define MAX_OPEN_FILES 20

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return HT_ERROR;        \
  }                         \
}

typedef struct {
  Record **item;
  int local_depth;
  int maxSize;
}buckets;

typedef struct {
  char file_name[40];
  buckets **bucket;
  int global_depth;
}HashTable;

int Open_files[MAX_OPEN_FILES]=NULL;



HT_ErrorCode HT_Init() {
  //insert code here  
  return HT_OK;
}

HT_ErrorCode HT_CreateIndex(const char *filename, int depth) {
  int id;
  CALL_BF(BF_CreateFile(filename));
  HashTable *HT;

}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc){
  //insert code here
  int i;
  CALL_BF(BF_OpenFile(fileName,&indexDesc));
  for(i=0 ; i < MAX_OPEN_FILES ; i++){
    if(Open_files[i] != NULL){
      Open_files[i] = indexDesc;
      break;
    }
  }
  return HT_OK;
}
 
HT_ErrorCode HT_CloseFile(int indexDesc) {
  //insert code here
  return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) {
  //insert code here
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
  //insert code here
  return HT_OK;
}

