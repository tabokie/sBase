#include <stdio.h>
#include <stdlib.h>
#include "b_macro.h"
#include "bplus.h"

int main(void){

    index_tree MainTree=initialIndexTree();
    setCurrentTree(MainTree);
    //test sample
    int sampleSize=100;
    int data[100]={0};
    Initial(data,i,100);
    int i;
    For(i,0,sampleSize){
        Insert(MainTree,data[i],data+i);
        putTree(MainTree);
    }
    putTree(MainTree);
    return 0;
}