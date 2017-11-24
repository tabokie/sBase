#include <stdio.h>
#include <stdlib.h> // 
#include <time.h> // time()
#include "b_macro.h" // For(i,a,b)
#include "bplus.h"
// initialIndexTree(), setCurrentTree(), getCurrentTree(), putTree()
// Insert(), ChangeRef(), ChangeName(), FetchAddr(),

/*!
    \instrcution
    \build Link b_macro.h bplus.h bplus.c bplustree_plus.c using gcc
    Originally built with gcc 4.9.2
*/

int GetRandom(int ceiling);

int main(void){
    srand(time(NULL));

    index_tree MainTree=initialIndexTree();
    setCurrentTree(MainTree);

    // sorted sequence inserting test
    /*
    int sampleSize=100;
    printf("Enter test sample size:\n");
    scanf("%d",&sampleSize);
    // test sample: sample[n]=n
    int* data=New(int,sampleSize);
    Initial(data,in_i_,sampleSize);
    // test routine
    int i;
    For(i,0,sampleSize){
        Insert(MainTree,data[i],data+i);
        putTree(MainTree);
    }
    putTree(MainTree);
    */

    // manual inserting test
    /*
    int value=0;
    int *addr=&value;
    while(1){
        printf(">");
        scanf("%d",&value);
        if(value==-1)break;
        Insert(MainTree,value,addr);
        putTree(MainTree);
    }
    */

    // random inserting test
    int sampleSize=0;    
    printf("Enter test sample size:\n");
    scanf("%d",&sampleSize);
    int bound=sampleSize*5;
    int* addr=&bound;
    int idx=0;
    int i;
    For(i,0,sampleSize){// use same addr to avoid SAME IDX conflict
        idx=GetRandom(bound);
        printf("Inserting: %d\n",idx);        
        Insert(MainTree,idx,addr);
        putTree(MainTree);
    }

    // random fetching test
    printf("Starting Fetching:\n");
    addr=NIL_ADDR;
    For(i,0,sampleSize){
        idx=GetRandom(bound);
        addr=FetchAddr(MainTree, idx);
        printf("idx:%d ref:%d\n",idx,(int)addr);
    }
    return 0;
}

//generate random number in [0,ceiling)
int GetRandom(int ceiling){

    int random = 0;
    if (ceiling - 1 < RAND_MAX){
        random = (int)rand() % ceiling;
    }
    else{
        random = (int)rand()*(ceiling - 1) / RAND_MAX;
    }
    return random;
}