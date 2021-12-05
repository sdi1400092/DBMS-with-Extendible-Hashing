#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hash_file.h"
#define MAX_OPEN_FILES 20
#define MAX_SIZE_OF_BUCKET BF_BLOCK_SIZE

#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return HT_ERROR;        \
  }                         \
}

typedef struct {
  int **HashCode;
  int number_of_block; //με τη παραδοχη οτι 1 block = 1 καδος
  int number_of_registries;
  int local_depth;
  int maxSize;
}buckets;

typedef struct {
  buckets **bucket;
  int global_depth;
}HashTable;

int Open_files[MAX_OPEN_FILES];

void HashFunction(int id, int depth, int **hashing){
  int i, binary[32];
  for(i=0;i<depth;i++){
    binary[i] = id%2;
    id = id/2;
  }
  *hashing = binary;
}

HT_ErrorCode HT_Init() {
  //insert code here 
  int i;
  for(i=0;i<MAX_OPEN_FILES;i++){
    Open_files[i]=-1;
  } 
  return HT_OK;
}

HT_ErrorCode HT_CreateIndex(const char *filename, int depth) {
  int id, i, j, n, file_desc, **binary;
  char *data;

  CALL_BF(BF_CreateFile(filename));

  HashTable *HT;
  HT = (HashTable *) malloc(sizeof(HashTable));
  printf("lala\n");
  // for(i=0;i<depth;i++){ //segmentation fault
  //   HT->bucket[i]->HashCode[i] =malloc(sizeof(int));
  // }
  printf("lala\n");
  for(i=0;i<(2^depth);i++){
    HT->bucket[i] = (buckets *) malloc(sizeof(buckets));
    HT->bucket[i]->local_depth = depth;
    HT->bucket[i]->maxSize = MAX_SIZE_OF_BUCKET/(sizeof(struct Record));
    HT->bucket[i]->number_of_registries = 0;
    n=i;
    for(j=0;j<depth;j++){
      *(HT->bucket[i]->HashCode[j])=n%2;
      n=n/2;
    }
  }
  HT->global_depth = depth;

  CALL_BF(BF_OpenFile(filename, &file_desc));
  BF_Block *temp_block;
  BF_Block_Init(&temp_block);
  CALL_BF(BF_AllocateBlock(file_desc, temp_block));
  data = BF_Block_GetData(temp_block);
  memcpy(data, HT, sizeof(HashTable));
  BF_Block_SetDirty(temp_block);
  CALL_BF(BF_UnpinBlock(temp_block));

  BF_Block *temp_block2;
  BF_Block_Init(&temp_block2);
  for(i=0;i<(2^depth);i++){ 
    CALL_BF(BF_AllocateBlock(file_desc, temp_block2));
    int block_num;
    HT->bucket[i]->number_of_block= BF_GetBlockCounter(file_desc,&block_num) - 1;    
    CALL_BF(BF_UnpinBlock(temp_block2));
  }

  printf("File: %s is created\n", filename);
  CALL_BF(BF_CloseFile(file_desc));
  free(HT);

  return HT_OK;
}

HT_ErrorCode HT_OpenIndex(const char *fileName, int *indexDesc){
  //insert code here
  int i;
  int temp;
  CALL_BF(BF_OpenFile(fileName,&temp));
  *indexDesc=temp;
  for(i=0 ; i < MAX_OPEN_FILES ; i++){
    if(Open_files[i] == -1){
      Open_files[i] = *indexDesc;
      break;
    }
  }
  return HT_OK;
}
 
HT_ErrorCode HT_CloseFile(int indexDesc) {
  //insert code here
  int i;
  for(i=0;i<MAX_OPEN_FILES;i++){
    if (Open_files[i]=indexDesc){
      Open_files[i]=-1;
    }
  }
  CALL_BF(BF_CloseFile(indexDesc));
  return HT_OK;
}

HT_ErrorCode HT_InsertEntry(int indexDesc, Record record) {
  //insert code here
  int *hashing, i, offset;
  char *data;
  Record *temp = &record;
  HashTable *HT;
  HT = (HashTable *) malloc(sizeof(HashTable));
  printf("3\n");
  
  BF_Block *block;
  BF_Block_Init(&block);
  CALL_BF(BF_GetBlock(indexDesc, 0, block));
  data = BF_Block_GetData(block);
  memcpy(HT, data, sizeof(HashTable));
  printf("4\n");

  HashFunction(record.id, HT->global_depth, &hashing);

  for(i=0;i<(2^HT->global_depth);i++){
    if (*(HT->bucket[i]->HashCode)== hashing){
      break;
    }
  }
  printf("5\n");
  // CALL_BF(BF_GetBlock(indexDesc, HT->bucket[i]->number_of_block, block)); //segmentation fault
  // data = BF_Block_GetData(block);
  // offset = HT->bucket[i]->number_of_registries;
  printf("6\n");
  // if (offset < HT->bucket[i]->maxSize){    //segmentation falut
  //   memcpy(data + offset*(sizeof(struct Record)), temp, sizeof(struct Record));
  //   BF_Block_SetDirty(block);
  //   CALL_BF(BF_UnpinBlock(block));
  // }
  printf("7\n");
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
  //insert code here
  return HT_OK;
}

