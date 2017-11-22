#include <stdio.h>
#include <stdlib.h>
#include "b_macro.h"
#include "bplus.h"

/***********************************************************
    Tree function class
************************************************************/
index_tree initialIndexTree(void){
    Log();
    index_tree newTree=New(struct IndexTree,1);
    // load tree's signal
    newTree->size=NIL;
    // load index and empty leaves
    newTree->root=firstIndexPage();
    newTree->root->father=newTree;
    // update info
    newTree->height=1;
    newTree->l_leaf=newTree->root->pointer[0];
    newTree->l_size=1;//one NINF

    return newTree;
}

// @NOT_SAFE
index_tree GLOBAL_Pointer=NULL;
void setCurrentTree(index_tree t){
    GLOBAL_Pointer=t;
    return;
}
index_tree getCurrentTree(void){
    return GLOBAL_Pointer;
}

// work bad for larger tree
// @BAD_PERF
void putTree(index_tree T){
    printf("===============================================<\n");
    // print log
    printf("Tree height: %d\nLinked leaves number: %d\n",T->height,T->l_size);
    // use round stack to load pages in one level
    int treeHeight=T->height;
    index_page* stack=New(index_page,M*pow(M,treeHeight));
    int size=M*pow(M,treeHeight);
    // first page
    stack[0]=T->root;
    int top=0;
    int head=0;
    // main iteration
    int i,j,k;
    For(i,0,treeHeight){
        //print this level
        printf(">");
        For(j,head,top+1){
            putIndexPage(stack[j%size]);
        }
        printf("\n");
        //load next level
        int newTop=top;
        For(j,head,top+1){
            For(k,0,stack[j%size]->size){
                if(stack[j%size]!=NULL)
                stack[(++newTop)%size]=stack[j%size]->pointer[k];
            }
        }
        head=top+1;
        top=newTop;
    }
    // print leaves
    printf(">");
    For(j,head,top+1){
        putLeafPage((leaf_page)stack[j%size]);
    }
    printf("\n");
    printf("===============================================<\n");
    // destory temporary stack
    free(stack);
    return;
}

// on a index tree
int Insert(index_tree T, index_type idx, addr_type addr){
    Log();
    // no index page to search
    if(T->root==NULL)Error(NIL tree!);
    // search for right leaf
    int i;
    int treeHeight=T->height;// search depth
    index_page cur=T->root;// search init
    For(i,0,treeHeight){
        // idx in [lo,hi)
        int lo=findInterval(cur->key,idx,cur->size);
        if(cur->pointer[lo]==NULL&&i<treeHeight-1)Error(NIL index page!);
        else if(cur->pointer[lo]==NULL)Error(NIL leaf page!);
        else cur=cur->pointer[lo];
    }
    // cur is the leaf
    // vacant leaf
    if(cur->size<M){
        if(insertToleaf((leaf_page)cur,idx,addr)){//vacant leaf
            T->l_size++;
            return 1;
        }
    }
    // try other leaves
    // not efficient enough
    // @BAD_PERF
    else if(recursiveInsertToLeaf(cur,idx,addr)){
        T->l_size++;
        return 1;
    }
    // leaves all full
    else{//use preset buffer, forced insert
        if(!insertToleaf_withBuffer(cur,idx,addr))Error(Fail to split leaf!);
        else if(!splitLeaf(cur))Error(Fail to split leaf!);
        else{
            T->l_size++;
            return 1;
        }
    }
    return 1;
}

/***********************************************************
    Leaf page function class
************************************************************/
leaf_page newLeafPage(void){
    Log();
    leaf_page newPage=New(struct LeafPage,1);

    newPage->size=0;
    newPage->key=New(index_type,M+1);
    Initial(newPage->key,(index_type)0,M+1);
    newPage->addr=New(addr_type,M+1);
    Initial(newPage->addr,(addr_type)NULL,M+1);
    newPage->next=NULL;
    newPage->pre=NULL;
    return newPage;
}

// inherit father pointer as well
leaf_page newLeafPage_loaded(leaf_page l_un, int init, int size){
    Log();
    leaf_page l=(leaf_page)l_un;
    leaf_page new=newLeafPage();
    // copy [size] from init
    int i;
    For(i,init,init+size){
        new->key[i-init]=l->key[i];
        new->addr[i-init]=l->addr[i];
    }
    new->addr[size]=l->addr[init+size];
    new->size=size;
    // inherit father pointer
    new->father=l->father;
    return new;
}

int freeLeafPage(leaf_page l){
    Log();
    free(l->key);
    free(l->addr);
    free(l);
    return 1;
}

// this routine  destory input leaf page
int splitLeaf(leaf_page l_un){//related to father
    Log();
    leaf_page l=(leaf_page)l_un;
    // not full leaf
    if(l->size<=M)return 0;
    // overflow
    if(l->father->size>=M+1)return 0;
    // safe to load to father
    // find pivot that link this leaf
    int pivot;
    For(pivot,0,l->father->size)if(l->father->key[pivot]==l->key[0])break;
    // father has no link
    if(l->father->size<=0)Error(NIL father!);
    // can't find the link
    if(pivot==l->father->size)return 0;
    // start to split
    // determine the new index to insert
    index_type newIdx=l->key[(M+1)/2];
    int size1=(M+1)/2;
    int size2=(M+1-(M+1)/2);
    // insert pointers to pivot+1 and pivot+2
    // right part to replace
    leaf_page right=newLeafPage_loaded(l,(M+1)/2,size2);
    l->father->pointer[pivot]=right;
    // left part to insert
    leaf_page left=newLeafPage_loaded(l,0,size1);
    insertPointer_withBuffer(l->father,pivot,left);
    //insert midian index to pivot+1
    insertToIndex_withBuffer(l->father,pivot+1,newIdx);

    //construct linkage

    //change index
    //if(pivot==-1)changeIndex(l->father->father,l->father->key[1],l->father->key[0]);
    //l->leaf
    if(l->pre==NULL){
        index_tree tree=getCurrentTree();
        tree->l_leaf=left;
        left->pre=NULL;
    }
    else {
        l->pre->next=left;
        left->pre=l->pre;
    }
    if(l->next!=NULL){
        l->next->pre=right;
    }
    left->next=right;
    right->pre=left;
    right->next=l->next;
    // check if needed to continue spliting
    if(l->father->size>=M+1&&!splitIndex(l->father))Error(split index error!);
    // destory initial leaf page
    freeLeafPage(l);
    return 1;
}

int insertToleaf(leaf_page l_un, index_type idx, addr_type addr){
    Log();
    leaf_page l=(leaf_page)l_un;
    // full
    if(l->size>=M)return 0;
    // special case for new index inserted down-to-up
    if(l->size==0&&l->pre!=NULL)addIndex(l->father,idx);
    // find pivot to insert
    int i;
    int lo=findInterval(l->key,idx,l->size);
    // change the first index, need to change upper index
    if(lo==-1&&l->size>0&&!changeIndex(l->father,l->key[0],idx))
        Error(Fail to change upper index!);
    if(l->key[lo]==idx&&l->addr[lo]!=addr)Error(Same index ref to different addr!);
    else if(l->key[lo]==idx)return 1;
    // insert routine
    For(i,l->size,lo+1){
        l->key[i]=l->key[i-1];
        l->addr[i]=l->addr[i-1];
    }
    l->key[lo+1]=idx;
    l->addr[lo+1]=addr;
    // update info
    l->size++;
    return 1;
}

int insertToleaf_withBuffer(leaf_page l_un, index_type idx, addr_type addr){
    Log();
    leaf_page l=(leaf_page)l_un;
    // check size
    if(l->size!=M)return 0;
    int i;
    // assert(l->size=M)
    int lo=findInterval(l->key,idx,l->size);
    if(lo==-1&&!changeIndex(l->father,l->key[0],idx))Error(Fail to change upper index!);
    if(l->key[lo]==idx&&l->addr[lo]!=addr)Error(Same index ref to different addr!);
    else if(l->key[lo]==idx)return 1;

    For(i,l->size,lo+1){
        l->key[i]=l->key[i-1];
        l->addr[i]=l->addr[i-1];
    }
    l->key[lo+1]=idx;
    l->addr[lo+1]=addr;

    l->size++;
    return 1;
}

int recursiveInsertToLeaf(leaf_page l_un,index_type idx,addr_type addr){
    Log();
    leaf_page l=(leaf_page)l_un;
    leaf_page cur=l;
    int isVacant=0;
    // check left leaves first
    while(cur!=NULL){
        if(cur->size<M){
            isVacant=1;
            break;
        }
        cur=cur->pre;
    }
    if(isVacant){
        return leftInsertToLeaf(l,idx,addr);
    }
    // check right leaves
    cur=l->next;
    while(cur!=NULL){
        if(cur->size<M){
            isVacant=1;
            break;
        }
        cur=cur->next;
    }
    if(isVacant)return rightInsertToLeaf(l,idx,addr);
    else return 0;
}

int leftInsertToLeaf(leaf_page l_un,index_type idx, addr_type addr){
    Log();
    leaf_page l=(leaf_page)l_un;
    if(l->size<M){
        if(!insertToleaf(l,idx,addr))Error(Unexpected Full leaf!);
        return 1;
    }
    else if(l->pre==NULL)return 0;
    else{
        index_type firstIdx=l->key[0];
        addr_type firstAddr=l->addr[0];
        // if needed rotating
        if(firstIdx<idx){
            int i;
            For(i,0,l->size-1){
                l->key[i]=l->key[i+1];
                l->addr[i]=l->addr[i+1];
            }
            l->size--;
            insertToleaf(l,idx,addr);
            changeIndex(l->father,firstIdx,l->key[0]);
            return leftInsertToLeaf(l->pre,firstIdx,firstAddr);
        }
        else{
            return leftInsertToLeaf(l->pre,idx,addr);
        }
    }
}

int rightInsertToLeaf(leaf_page l_un, index_type idx, addr_type addr){
    Log();
    leaf_page l=(leaf_page)l_un;
    if(l->size<M){
        if(!insertToleaf(l,idx,addr))Error(Unexpected Full leaf!);
        return 1;
    }
    else if(l->next==NULL)return 0;
    else{
        index_type lastIdx=l->key[l->size-1];
        addr_type lastAddr=l->addr[l->size-1];
        // if needed rotating
        if(lastIdx>idx){
            l->size--;
            insertToleaf(l,idx,addr);
            return rightInsertToLeaf(l->next,lastIdx,lastAddr);
        }
        else{
            return rightInsertToLeaf(l->next,idx,addr);
        }
    }
}

/***********************************************************
    Index page function class
************************************************************/
index_page newIndexPage(void){
    Log();
    index_page newPage=New(struct IndexPage,1);
    newPage->size=0;
    newPage->pointer=New(index_page,M+1);
    Initial(newPage->pointer,NULL,M+1);
    newPage->key=New(index_type,M+1);
    Initial(newPage->key,(index_type)0,M+1);
    return newPage;
}

index_page firstIndexPage(void){
    Log();
    // initial empty index page
    index_page first=newIndexPage();
    // initial empty leaves and link to father
    int i;
    leaf_page cur=newLeafPage();
    first->pointer[0]=cur;//[0]'s pre is NULL
    cur->father=first;
    For(i,1,M){
        cur->next=newLeafPage();
        cur->next->pre=cur;
        cur=cur->next;
        first->pointer[i]=cur;
        cur->father=first;
    }
    cur->next=NULL;//symmetricity
    // preset the NINF index
    first->size=1;
    first->key[0]=NINF;
    ((leaf_page)(first->pointer[0]))->size=1;
    ((leaf_page)(first->pointer[0]))->key[0]=NINF;
    return first;
}

// this routine inherit father pointer
index_page newIndexPage_loaded(index_page index_un, int init, int size){
    Log();
    index_page index=(index_page)index_un;
    index_page new=newIndexPage();
    int i;
    For(i,init,init+size){
        new->key[i-init]=index->key[i];
        new->pointer[i-init]=index->pointer[i];
    }
    new->size=size;
    // inherit father pointer
    new->father=index->father;
    return new;
}

int freeIndexPage(index_page index){
    Log();
    free(index->key);
    free(index->pointer);
    free(index);
    return 1;
}

int changeIndex(index_page index_un, index_type init, index_type new){
    Log();
    index_page index=(index_page)index_un;
    // two boundary conditions:
    // index is empty and init is from first pointer
    // index is empty and init is from second pointer
    // all ruled out
    if(index==NULL)Error(NIL index!);
    // tree node, no need to change
    if(index->size==NIL)return 1;
    // no index
    else if(index->size<=0)return 0;
    // NIL represent tree node, fatal error
    if(init==NIL)Error(A NIL index comming from below!);
    // NINF constantly guard left edge
    else if(init==NINF)Error(Try to replace the NINF index!);
    // start searching
    int lo=findInterval(index->key,init,index->size);
    // can't find index
    if(lo==-1)Error(Unexpected small index!);
    // two conditions for index
    if(index->key[lo]==init)index->key[lo]=new;
    else if(index->key[lo+1]==init)index->key[lo+1]=new;
    else Error(cannot find suitable index to change);
    // check if needed to continue changing
    if(index->key[0]==new)return changeIndex(index->father, init, new);
    return 1;
}

int addIndex(index_page index_un, index_type new){
    Log();
    index_page index=(index_page)index_un;
    if(index->size>=M)Error(Index page already full!);
    index->key[index->size]=new;
    index->size++;
    return 1;
}

int insertToIndex_withBuffer(index_page index_un, int p, index_type new){
    Log();
    index_page index=(index_page)index_un;
    int i;
    index->size++;
    For(i,index->size,p){
        index->key[i]=index->key[i-1];
    }
    index->key[p]=new;
    return 1;
}

//deserted
int replacePointer(index_page index_un, int p, leaf_page new){
    Log();
    index_page index=(index_page)index_un;
    if(index->size<p)return 0;
    index->pointer[p]=new;
    return 1;
}

int insertPointer_withBuffer(index_page index_un,int p,leaf_page new){//insert to idx(p)
    Log();
    index_page index=(index_page)index_un;
    int i;
    For(i,index->size,p){
        index->pointer[i]=index->pointer[i-1];
    }
    index->pointer[p]=new;
    return 1;
}

// this routine destory input index page
int splitIndex(index_page index_un){// this routine destory previous index page
    Log();
    index_page index=(index_page)index_un;
    // no full
    if(index->size<=M)return 0;
    // no index page father
    // need to construct new one
    if(index->father->size==NIL){
        index_tree tree =getCurrentTree();
        if(tree!=index->father)Error(Tree Root Conflict!);
        // new index page
        index_page newRoot = newIndexPage();
        // construct linkage
        newRoot->father=tree;
        tree->root=newRoot;
        tree->height++;
        // load NINF index and info
        newRoot->key[0]=NINF;
        newRoot->pointer[0]=index;
        newRoot->size=1;
        index->father=newRoot;
    }
    // father is already full
    else if(index->father->size>=M+1)return 0;
    // safe to load to father
    int pivot;
    For(pivot,0,index->father->size)if(index->father->key[pivot]==index->key[0])break;
    if(index->father->size<=0)Error(NIL father!);//not strict
    if(pivot==index->father->size)return 0;
    //determine the new index to insert
    index_type newIdx=index->key[(M+1)/2];
    
    //construct new index pages
    int size1=(M+1)/2;
    int size2=(M+1-(M+1)/2);
    //insert pointers to pivot+1 and pivot+2
    //right part to replace
    index_page right=newIndexPage_loaded(index,(M+1)/2,size2);newIndexPage_loaded(index,0,size1);
    index->father->pointer[pivot]=right;
    //left part to insert
    index_page left=newIndexPage_loaded(index,0,size1);
    insertPointer_withBuffer(index->father,pivot,left);
    //insert index to pivot+1
    insertToIndex_withBuffer(index->father,pivot+1,newIdx);

    //construct linkage
    int i;
    For(i,0,size1)((leaf_page)(index->pointer[i]))->father=left;
    For(i,(M+1)/2,(M+1)/2+size2)((leaf_page)(index->pointer[i]))->father=right;
    //change index
    //if index->father->key[1] is null, that means index->father's father is tree, and conflict dismissed
    if(pivot==-1)changeIndex(index->father->father,index->father->key[1],index->father->key[0]);
    // check if needed to split father
    if(index->father->size==M+1&&!splitIndex(index->father))Error(split index error!);
    // destory initial index page
    freeLeafPage(index);
    return 1;

}


/***********************************************************
    Additional function class
************************************************************/
int findInterval(index_type* index, index_type idx, int size){
    //return [a,a+1) that contains idx
    int lo=-1,hi=size,mid;
    while(hi-lo>=2){
        mid=lo+(hi-lo)/2;
        if(index[mid]<=idx)lo=mid;
        else hi=mid;
    }
    return lo;
}
