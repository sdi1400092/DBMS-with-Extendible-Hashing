#include <stdio.h>
#include <stdlib.h>

typedef struct{
    int x;
} number;

typedef struct {
    number *z;
} arrayofnumber;

void HashFunction(int id, int depth, int **hashing){
  int i, *binary;
  binary = (int *) malloc(sizeof(int));
  for(i=0;i<depth;i++){
    binary[i] = id%2;
    id = id/2;
  }
  *hashing = binary;
}

int main(){
    int *q, *hashing;
    arrayofnumber *a;
    a = (arrayofnumber *) malloc(sizeof(arrayofnumber));
    a->z = (number *) malloc(10 * sizeof(number));
    for (int i=0 ; i<10 ; i++){
        a->z[i].x = i;
    }
    for (int i=0 ; i<10 ; i++){
        printf("%d\n", a->z[i].x);
        HashFunction(a->z[i].x, 3, &hashing);
        for(int z=0;z<3;z++){
            printf("%d", hashing[z]);
        }
        printf("\n");
    }
}