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
  buckets *bucket;
  int global_depth;
  int next_block;
}HashTable;

int Open_files[MAX_OPEN_FILES];

void HashFunction(int id, int depth, int **hashing){
  int i, *binary;
  binary = (int *) malloc(sizeof(int));
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

int power(int base, int exp){
  int result = 1;
  while(exp!=0){
    result *= base;
    --exp;
  }
  return result;
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
  HT->bucket = (buckets *) malloc(power(2,depth)*sizeof(buckets));
  for(i=0;i<power(2,depth);i++){
    HT->bucket[i].local_depth = depth;
    HT->bucket[i].maxSize = MAX_SIZE_OF_BUCKET/(sizeof(struct Record) + 1);
    HT->bucket[i].number_of_registries = 0;
    n=i;
    HT->bucket[i].HashCode =malloc(depth*sizeof(int));
    for(j=0;j<depth;j++){
      HT->bucket[i].HashCode[j]=n%2;
      n=n/2;
    }
  }
  HT->global_depth = depth;

  CALL_BF(BF_OpenFile(filename, &file_desc));
  BF_Block *temp_block;
  BF_Block_Init(&temp_block);
  CALL_BF(BF_AllocateBlock(file_desc, temp_block));
  int block_num;
  data = BF_Block_GetData(temp_block);

  BF_Block *temp_block2;
  BF_Block_Init(&temp_block2);
  for(i=0;i<power(2,depth);i++){ 
    CALL_BF(BF_AllocateBlock(file_desc, temp_block2));
    CALL_BF(BF_GetBlockCounter(file_desc,&block_num));
    HT->bucket[i].number_of_block= block_num -1;
    BF_Block_SetDirty(temp_block2);
    CALL_BF(BF_UnpinBlock(temp_block2));
  }

  memcpy(data, &(HT->global_depth), sizeof(int));
  for( i=0;i<power(2,HT->global_depth);i++){
    memcpy(data+sizeof(int)+(i*sizeof(buckets)),&(HT->bucket[i]),sizeof(buckets));
  }
  BF_Block_SetDirty(temp_block);
  CALL_BF(BF_UnpinBlock(temp_block));

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
  //printf("id: %d\n", record.id);
  
  BF_Block *block, *Dirblock;
  BF_Block_Init(&block);
  BF_Block_Init(&Dirblock);
  CALL_BF(BF_GetBlock(indexDesc, 0, Dirblock));
  data = BF_Block_GetData(Dirblock);
  memcpy(&(HT->global_depth), data, sizeof(int));
  HT->bucket = (buckets *) malloc(power(2,HT->global_depth) * sizeof(buckets));
  for(i=0;i<power(2,HT->global_depth);i++){
    memcpy(&(HT->bucket[i]),data+sizeof(int)+i*sizeof(buckets),sizeof(buckets));
    HashFunction(i,HT->global_depth,&(HT->bucket[i].HashCode));
  }
  HashFunction(record.id, HT->global_depth, &hashing);
  int counter;
  for(i=0;i<(power(2,HT->global_depth));i++){
    counter=0;
    for(int j=(HT->bucket[i].local_depth)-1;j>=0;j--){
      if (HT->bucket[i].HashCode[j]== hashing[j]){
        counter++;
      }
    }
    if(counter == HT->bucket[i].local_depth){
      break;
    }
  }
  CALL_BF(BF_GetBlock(indexDesc, HT->bucket[i].number_of_block, block));
  data2 = BF_Block_GetData(block);
  offset = HT->bucket[i].number_of_registries;
  if (offset < HT->bucket[i].maxSize){
    memcpy(data2 + offset*sizeof(struct Record), temp, sizeof(struct Record));
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
    HT->bucket[i].number_of_registries++;
  }
  else{
    if(HT->bucket[i].local_depth == HT->global_depth){
      //expand
      HT->global_depth += 1;
      buckets *temp;
      temp = (buckets *) malloc((power(2,HT->global_depth-1)) * sizeof(buckets));
      for(int j=0 ; j<power(2,HT->global_depth-1) ; j++){
        temp[j].HashCode=HT->bucket[j].HashCode;
        temp[j].local_depth=HT->bucket[j].local_depth;
        temp[j].maxSize=HT->bucket[j].maxSize;
        temp[j].number_of_block=HT->bucket[j].number_of_block;
        temp[j].number_of_registries=HT->bucket[j].number_of_registries;
      }
      HT->bucket=(buckets *) realloc(HT->bucket,power(2,HT->global_depth)*sizeof(buckets));
      int z=0;
      for(int j=0;j<(power(2,HT->global_depth));j+=2){
        HT->bucket[j]=temp[z];
        HT->bucket[j+1]=temp[z];
        z+=1;
      }
      //Hashing
      for(int j=0;j<power(2,HT->global_depth);j++){
        HT->bucket[j].HashCode = (int *) malloc(HT->global_depth*sizeof(int));
        int n = j;
        for(z=0;z<HT->global_depth;z++){
          HT->bucket[j].HashCode[z]=n%2;
          n=n/2;
        }
      }
    }
    //split
    BF_Block *temp_block;
    BF_Block_Init(&temp_block);
    CALL_BF(BF_AllocateBlock(indexDesc, temp_block));
    printf("3\n");
    int k=(HT->global_depth)-(HT->bucket[i].local_depth);
    int z=i;
    for(int j=0;j<power(2,k);j++){
      HT->bucket[z].local_depth+=1;
      z++;
    }
    printf("4\n");
    int blocknum;
    BF_GetBlockCounter(indexDesc, &blocknum);
    for(int j=i+(power(2,k))-1;j>=(i+(power(2,k)/2));j--){
      HT->bucket[j].number_of_block = blocknum - 1;
    }
    if(record.id>=32){
      for(int a=0;a<power(2, HT->global_depth);a++){
        printf("bucket: %d depth: %d block: %d\n",a, HT->bucket[a].local_depth , HT->bucket[a].number_of_block);
      }
    }

    BF_Block_SetDirty(temp_block);
    CALL_BF(BF_UnpinBlock(temp_block));

    int x = HT->bucket[i].number_of_registries;
    HT->bucket[i].number_of_registries = 0;

    data = BF_Block_GetData(Dirblock);
    printf("5\n");
    memcpy(data, &(HT->global_depth), sizeof(int));
    for(int j=0;j<power(2,HT->global_depth);j++){
      memcpy(data+sizeof(int)+(j*sizeof(buckets)),&(HT->bucket[j]),sizeof(buckets));
    }
    BF_Block_SetDirty(Dirblock);
    CALL_BF(BF_UnpinBlock(Dirblock));
    printf("6\n");

    Record *temp_record;
    temp_record = (Record *) malloc(sizeof(Record));
    for(int j=0 ; j<x ; j++){
      memcpy(temp_record, data2 + j*sizeof(struct Record), sizeof(struct Record));
      HT_InsertEntry(indexDesc, *temp_record);
    }
    printf("7\n");
    free(temp_record);
    HT_InsertEntry(indexDesc, record);  

  }

  data = BF_Block_GetData(Dirblock);
  memcpy(data, &(HT->global_depth), sizeof(int));
  for(i=0;i<power(2,HT->global_depth);i++){
    memcpy(data+sizeof(int)+(i*sizeof(buckets)),&(HT->bucket[i]),sizeof(buckets));
  }

  free(HT->bucket);
  free(HT);
  BF_Block_SetDirty(Dirblock);
  CALL_BF(BF_UnpinBlock(Dirblock));
}

HT_ErrorCode HT_PrintAllEntries(int indexDesc, int *id) {
  //insert code here
  int *id_hashing, i;
  Record *record;
  record = (Record *) malloc(sizeof(Record));
  BF_Block *block;
  BF_Block_Init(&block);
  char *data, *records_data;
  HashTable *HT;
  HT = (HashTable *)malloc(sizeof(HashTable));

  //get directory
  CALL_BF(BF_GetBlock(indexDesc, 0, block));
  data = BF_Block_GetData(block);
  memcpy(&(HT->global_depth), data, sizeof(int));
  HT->bucket = (buckets *) malloc(sizeof(buckets));
  for(int i=0;i<power(2,HT->global_depth);i++){
    memcpy(&(HT->bucket[i]),data+sizeof(int)+i*sizeof(buckets),sizeof(buckets));
    HashFunction(i,HT->global_depth,&(HT->bucket[i].HashCode));
  }


  if(id==NULL){
    int i;
    for(i=0;i<(power(2,HT->global_depth));i++){
      BF_GetBlock(indexDesc,HT->bucket[i].number_of_block,block);
      data= BF_Block_GetData(block);
      //print records of block data
      for(int j=0 ; j<HT->bucket[i].number_of_registries ; j++){
        memcpy(record, data + j*sizeof(Record), sizeof(Record));
        printRecord(*record);
      }
    }
  }
  else{
    printf("id to be found = %d\n", *id);
    HashFunction(*id, HT->global_depth, &id_hashing);
    int counter, block_num;
    for(i=0;i<(power(2,HT->global_depth));i++){
      counter=0;
      for(int j=(HT->bucket[i].local_depth)-1;j>=0;j--){
        if (HT->bucket[i].HashCode[j]== id_hashing[j]){
          counter++;
        }
      }
      if(counter == HT->bucket[i].local_depth){
        break;
      }
    }
    block_num = HT->bucket[i].number_of_block;
    CALL_BF(BF_GetBlock(indexDesc, block_num, block));
    data = BF_Block_GetData(block);
    for(int j=0 ; j<HT->bucket[i].number_of_registries ; j++){
      memcpy(record, data + j*sizeof(Record), sizeof(Record));
      if(record->id == *id){
        printRecord(*record);
      }
    }
  }

  free(HT->bucket);
  free(HT);
  free(record);

  return HT_OK;
}

