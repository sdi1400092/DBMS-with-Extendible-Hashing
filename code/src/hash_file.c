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
  int *HashCode;
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

void printRecord(Record record){
  printf("Id: %d\n",record.id);
  int i;
  printf("Name: ");
  for(i=0;i<15;i++){
    printf("%c",record.name[i]);
    //if(record.name[i+1]==NULL) break;
  }
  printf("\n");
  printf("Surname: ");
  for(i=0;i<20;i++){
    printf("%c",record.surname[i]);
    //if(record.surname[i+1]==NULL) break;
  }
  printf("\n");
  printf("City: ");
  for(i=0;i<20;i++){
    printf("%c",record.city[i]);
    //if(record.city[i+1]==NULL) break;
  }
  printf("\n");
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
  for(i=0;i<(2^depth);i++){
    HT->bucket[i] = (buckets *) malloc(sizeof(buckets));
    HT->bucket[i]->local_depth = depth;
    HT->bucket[i]->maxSize = MAX_SIZE_OF_BUCKET/(sizeof(struct Record));
    HT->bucket[i]->number_of_registries = 0;
    n=i;
    HT->bucket[i]->HashCode =malloc(depth*sizeof(int));
    for(j=0;j<depth;j++){
      HT->bucket[i]->HashCode[j]=n%2;
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
  char *data, *data2;
  Record *temp = &record;
  HashTable *HT;
  HT = (HashTable *) malloc(sizeof(HashTable));
  
  BF_Block *block;
  BF_Block_Init(&block);
  CALL_BF(BF_GetBlock(indexDesc, 0, block));
  data = BF_Block_GetData(block);
  memcpy(HT, data, sizeof(HashTable)); //memcpy(data, HT, sizeof(HashTable));?????

  HashFunction(record.id, HT->global_depth, &hashing);

  for(i=0;i<(2^HT->global_depth);i++){
    for(int j=(HT->bucket[i]->local_depth -1);j>0;j--){
      if (HT->bucket[i]->HashCode[j]== hashing[j]){ //problhma
        break;                                     
      }
    }
  }
  CALL_BF(BF_GetBlock(indexDesc, HT->bucket[i]->number_of_block, block));
  data = BF_Block_GetData(block);
  offset = HT->bucket[i]->number_of_registries;
  if (offset < HT->bucket[i]->maxSize){
    memcpy(data + offset*(sizeof(struct Record)), temp, sizeof(struct Record));
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
    HT->bucket[i]->number_of_registries++;
  }
  else{
    jump: //temporary
    if(HT->bucket[i]->local_depth<HT->global_depth){
      //split
      BF_Block *temp_block;
      BF_Block_Init(&temp_block);
      CALL_BF(BF_AllocateBlock(indexDesc, temp_block));
      data2 = BF_Block_GetData(temp_block);
      //HT->bucket[i]->local_depth+=1;
      int k=(HT->global_depth)-(HT->bucket[i]->local_depth);
      int z=i;
      for(int j=0;j<2^k;j++){
        HT->bucket[z]->local_depth+=1;
        z++;
      }
      int blocknum;
      for(int j=i+(2^k)-1;j>(i+((2^k)/2));j--){
        HT->bucket[j]->number_of_block=BF_GetBlockCounter(indexDesc, &blocknum) - 1;
      }
      int x = HT->bucket[i]->number_of_registries;
      HT->bucket[i]->number_of_registries = 0; //??mesa sto for apo panw den eprepe na htan auto?
      for(int j=0 ; j<x ; j++){
        Record *temp_record;
        memcpy(temp_record, data + j*sizeof(struct Record), sizeof(struct Record));
        HT_InsertEntry(indexDesc, *temp_record);
      }
    }
    else{
      //expand
      // int j;
      // for(j=2^HT->global_depth;j<(2^(HT->global_depth + 1));j++){
      //   HT->bucket[j] = (buckets *) malloc(sizeof(buckets));
      //   HT->bucket[j]->local_depth = HT->bucket[j-(2^HT->global_depth)]->local_depth;
      //   HT->bucket[j]->maxSize = MAX_SIZE_OF_BUCKET/(sizeof(struct Record));
      //   HT->bucket[j]->number_of_registries = HT->bucket[j-(2^HT->global_depth)]->number_of_registries;
      //   HT->bucket[j]->number_of_block = HT->bucket[j-(2^HT->global_depth)]->number_of_block;
      // }
      // //sort by number of block
      // buckets *temp;
      // int z;
      // for(z=0;z<(2^HT->global_depth);z++){
      //   for(j=z+1;j>(2^HT->global_depth);j--){
      //     if (HT->bucket[j]->number_of_block <= HT->bucket[z]->number_of_block){
      //       temp = HT->bucket[j];
      //       HT->bucket[j] = HT->bucket[z];
      //       HT->bucket[z]=temp;
      //     }
      //   }
      // }
      HT->global_depth += 1;
      buckets **temp;
      for(int j=0;i<(2^(HT->global_depth));j++){
        temp[j]=(buckets *)malloc(sizeof(buckets));
      }
      temp=HT->bucket;
      *(HT->bucket)=(buckets *) realloc(HT->bucket,2^(HT->global_depth)*sizeof(buckets));
      int z=0;
      for(int j=0;j<(2^(HT->global_depth));j+=2){
        HT->bucket[j]=temp[z];
        HT->bucket[j+1]=temp[z];
        z+=1;
      }
      free(temp);
      //Hashing
      for(int j=0;j<2^HT->global_depth;j++){
        HT->bucket[j]->HashCode = (int *) malloc(HT->global_depth*sizeof(int));
        int n = j;
        for(z=0;z<HT->global_depth;z++){
          HT->bucket[j]->HashCode[z]=n%2;
          n=n/2;
        }
      }
    }
    goto jump;  //temporary
  }
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
  //insert code here
  BF_Block *block;
  BF_Block_Init(&block);
  char *data;
  HashTable *HT;
  CALL_BF(BF_GetBlock(indexDesc, 0, block));
  data = BF_Block_GetData(block);
  memcpy(HT, data, sizeof(HashTable));
  if(id!=NULL){
    int i;
    for(i=0;i<(2^HT->global_depth);i++){
      BF_GetBlock(indexDesc,HT->bucket[i]->number_of_block,block);
      data= BF_Block_GetData(block);
      //print records of block data
    }
  }
  else{

  }
  return HT_OK;
}

