#include <stdio.h>
#include <stdlib.h>

#define Error(note)					({printf(#note);printf("\n");exit(1);})
#define New(typeName,size)			(typeName*)malloc(sizeof(typeName)*(size))
#define Initial(array,val,size)		({int i;for(i=0;i<size;i++)array[i]=val;})
#define PutInts(array,size)			({printf(#array);printf(": ");int i;for(i=0;i<size;i++)printf("%d ",array[i]);printf("\n");})
#define For(i,a,b)					for(i=a;(a<=b)?(b-a):(a-b)>0;i+=(a<=b)?1:-1)

/*
about B+tree
1.find corresponding leaf
2.if not full
	2.1 Insert
3.if full and leaves not full->rotate
	3.0 premise: maintain a size element for linked leaves
	>recursive allowed: insertToleaf
		3.1 from pivot to left then to right
		3.1.1 if left...
4.if leaves full
	insertToIndex(median index)
*/


#define M 				(3)
typedef int index_type;
typedef int* addr_type;

//maybe round array?
typedef struct IndexPage{
	int size;
	struct IndexPage** pointer;
	index_type* key;
	struct IndexPage* father;
}* index_page;

typedef struct LeafPage{
	int size;
	index_type* key;
	addr_type* addr;
	struct LeafPage* next;
	struct LeafPage* pre;
	index_page father;
}* leaf_page;

typedef struct IndexTree{
	index_page root;
	unsigned int height;
	leaf_page l_leaf;
	int l_size;
}* index_tree;

leaf_page newLeafPage(void){
	leaf_page newPage=New(struct LeafPage,1);
	newPage->size=0;
	newPage->key=New(index_type,M);
	Initial(newPage->key,(index_type)0,M+1);
	newPage->addr=New(addr_type,M);
	Initial(newPage->addr,(addr_type)NULL,M+1);
	newPage->next=NULL;
	newPage->pre=NULL;
	return newPage;
}

index_page newIndexPage(void){
	index_page newPage=New(struct IndexPage,1);
	newPage->size=0;
	newPage->pointer=New(index_page,M+2);
	Initial(newPage->pointer,NULL,M+2);
	newPage->key=New(index_type,M+1);
	Initial(newPage->key,(index_type)0,M+1);
	return newPage;
}

int freeLeafPage(leaf_page l){
	free(l->key);
	free(l->addr);
	free(l);
	return 1;
}

int freeIndexPage(index_page index){
	free(index->key);
	free(index->addr);
	free(index);
	return 1;
}

index_page firstIndexPage(void){
	index_page first=newIndexPage();
	int i;

	leaf_page cur=newLeafPage();
	first->pointer[0]=cur;//[0]'s pre is NULL
	For(i,1,M+1){
		first->pointer[i]=newLeafPage();
		cur->next=first->pointer[i];
		first->pointer[i]->pre=cur;
		cur=cur->next;
	}
	//cur->next=first->pointer[0];
	cur->next=NULL;//symmetricity
	return first;
}

index_tree initialIndexTree(void){
	index_tree newTree=New(struct IndexTree,1);
	newTree->root=firstIndexPage();
	newTree->height=1;
	newTree->l_leaf=newTree->root->pointer[0];
	newTree->l_size=0;
	return newTree;
}

int changeIndex(index_page index, index_type init, index_type new){
	int i;
	For(i,0,M)if(index->key[i]==init)break;
	if(i==M)return 0;
	else index->key[i]=new;
	if(i==0)return changeIndex(index->father, init, new);
	return 1;
}

int insertToleaf(leaf_page l, index_type idx, addr_type addr){
	if(l->size>=M)return 0;
	int i;
	int lo=-1,hi=l->size,mid;
	while(hi-lo>=2){
		mid=lo+(hi-lo)/2;
		if(l->idx[mid]<=idx)lo=mid;
		else hi=mid;
	}
	if(lo==-1&&!changeIndex(l->father,l->key[0],idx))Error(Fail to change upper index!);
	For(i,l->size,hi){
		l->key[i]=l->key[i-1];
		l->addr[i]=l->addr[i-1];
	}
	l->key[hi]=idx;
	l->addr[hi]=addr;

	l->size++;
	return 1;
}
int insertToleaf_withBuffer(leaf_page l, index_type idx, addr_type addr){
	if(l->size!=M)return 0;
	int i;
	//l->size=M
	int lo=-1,hi=l->size,mid;
	while(hi-lo>=2){
		mid=lo+(hi-lo)/2;
		if(l->idx[mid]<=idx)lo=mid;
		else hi=mid;
	}
	if(lo==-1&&!changeIndex(l->father,l->key[0],idx))Error(Fail to change upper index!);
	For(i,l->size,hi){
		l->key[i]=l->key[i-1];
		l->addr[i]=l->addr[i-1];
	}
	l->key[hi]=idx;
	l->addr[hi]=addr;

	l->size++;
	return 1;
}

int insertToIndex_withBuffer(index_page index,index_type idx, addr_type addr){

}

int insertPointer_withBuffer(index_page index,int p,leaf_page new){//insert to idx(p)
	int i;
	For(i,index->size+1,p){
		index->pointer[i]=index->pointer[i-1];
	}
	index->pointer[p]=new;
	return 1;
}

int splitLeaf(leaf_page l){//related to father
	if(l->size<=M)return 0;
	if(l->father->size>=M+1)return 0;
	//safe to load to father
	int i,pivot;
	For(i,0,l->father->size)if(l->father->key[i]==l->key[0])break;
	if(i==l->father->size)return 0;
	l->father->pointer[pivot+1]=newLeafPage_loaded();
	insertPointer_withBuffer(l->father,pivot+1,newLeafPage_loaded());
	insertToIndex();
}
int splitIndex(index_page index){

}
int leftInsertToLeaf(leaf_page l,index_type idx, addr_type addr){
	if(l->size<M){
		if(!insertToleaf(l,idx,addr))Error(Unexpected Full leaf!);
		return 1;
	}
	else if(l->pre==NULL)return 0;
	else{//idx is biggest
		index_type firstIdx=l->key[0];
		addr_type firstAddr=l->addr[0];
		int i;
		For(i,0,l->size-1){
			l->key[i]=l->key[i+1];
			l->addr[i]=l->addr[i+1];
		}
		l->key[l->size-1]=idx;
		l->addr[l->size-1]=addr;
		changeIndex(l->father,firstIdx,l->key[0]);
		return leftInsertToLeaf(l->pre,firstIdx,firstAddr);
	}
}

int rightInsertToLeaf(leaf_page l, index_type idx, addr_type addr){
	if(l->size<M){
		if(!insertToleaf(l,idx,addr))Error(Unexpected Full leaf!);
		return 1;
	}
	else if(l->pre==NULL)return 0;
	else{//idx is smallest
		index_type lastIdx=l->key[l->size-1];
		addr_type lastAddr=l->addr[l->size-1];
		l->size--;
		insertToleaf(l,idx,addr);
		return rightInsertToLeaf(l->next,lastIdx,lastAddr);
	}
}

int recursiveInsertToLeaf(leaf_page l,index_type idx,addr_type addr){
	leaf_page cur=l;
	int isVacant=0;
	while(cur!=NULL){
		if(cur->size<M){
			isVacant=1;
			break;
		}
		cur=cur->pre;
	}
	if(isVacant)return leftInsertToLeaf(l,idx,addr);
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

int Insert(index_tree T, index_type idx, addr_type addr){
	if(T->root==NULL)Error(NIL tree!);
	int i;
	int treeHeight=T->height;//search depth
	index_page cur=T->root;//search init
	For(i,0,treeHeight){
		//idx in [lo,hi)
		int lo=-1,hi=cur->size,mid;
		while(hi-lo>=2){
			mid=(hi-lo)/2+lo;
			if(cur->key[mid]<=idx)lo=mid;
			else hi=mid;
		}
		lo++;
		if(cur->pointer[lo]==NULL&&i<treeHeight-1)Error(NIL index page!);
		else if(cur->pointer[lo]==NULL)Error(NIL leaf page!);
		else cur=cur->pointer[lo];
	}
	//cur is the leaf
	if(cur->size<M)insertToleaf(cur,idx,addr);//vacant leaf
	else if(T->l_size<M*pow(M+1,treeHeight)){//rotate
		if(recursiveInsertToLeaf(cur,idx,addr)){
			T->l_size++;
			return 1;
		}
		else return Error(Cannot find the vacant leaf!);
	}
	else{//use preset buffer
		if(insertToleaf_withBuffer(cur,idx,addr))Erro(Fail to split leaf!);
		if(splitLeaf(cur))Error(Fail to split leaf!);
	}

}

int main(void){

	index_tree MainTree=initialIndexTree();
	int data=7;
	Insert(MainTree,1,&data);
	return 0;
}