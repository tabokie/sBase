#include <stdio.h>
#include <stdlib.h>
#include "b_macro.h"
#include "bplus.h"

/*!
    \instrcution
    \build Link b_macro.h bplus.h bplus.c bplustree_plus.c using gcc
    Originally built with gcc 4.9.2
*/

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