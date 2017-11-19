#include <stdio.h>
#include <stdlib.h>

#define Error(note)					({printf(#note);printf("\n");exit(1);})
#define New(typeName,size)			(typeName*)malloc(sizeof(typeName)*(size))
#define Initial(array,val,size)		({int i;for(i=0;i<size;i++)array[i]=val;})
#define PutInts(array,size)			({printf(#array);printf(": ");int i;for(i=0;i<size;i++)printf("%d ",array[i]);printf("\n");})
#define For(i,a,b)					for(i=a;(a<=b)?(b-i):(i-b)>0;i+=(a<=b)?1:-1)
#define Log()						({printf("%s",__func__);printf("\n");})
#define BP(note)					({printf(#note);printf(", In function %s",__func__);printf("\n");})

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
#define Inf 			(1e3)
typedef int index_type;
typedef int* addr_type;


int findInterval(index_type* index, index_type idx, int size){
	int lo=-1,hi=size,mid;
	while(hi-lo>=2){
		mid=lo+(hi-lo)/2;
		if(index[mid]<=idx)lo=mid;
		else hi=mid;
	}
	return lo;
}
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
	int size;//recognize code
	index_page root;
	int height;
	leaf_page l_leaf;
	int l_size;
}* index_tree;

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

index_page newIndexPage(void){
	Log();
	index_page newPage=New(struct IndexPage,1);
	newPage->size=0;
	newPage->pointer=New(index_page,M+2);
	Initial(newPage->pointer,NULL,M+2);
	newPage->key=New(index_type,M+1);
	Initial(newPage->key,(index_type)0,M+1);
	return newPage;
}

int freeLeafPage(leaf_page l){
	Log();
	free(l->key);
	free(l->addr);
	free(l);
	return 1;
}

int freeIndexPage(index_page index){
	Log();
	free(index->key);
	free(index->pointer);
	free(index);
	return 1;
}

index_page firstIndexPage(void){
	Log();
	index_page first=newIndexPage();
	int i;

	leaf_page cur=newLeafPage();
	first->pointer[0]=cur;//[0]'s pre is NULL
	cur->father=first;
	For(i,1,M+1){
		cur->next=newLeafPage();
		cur->next->pre=cur;
		cur=cur->next;
		first->pointer[i]=cur;
		cur->father=first;
	}
	cur->next=first->pointer[0];
	cur->next=NULL;//symmetricity
	return first;
}

index_tree initialIndexTree(void){
	Log();
	index_tree newTree=New(struct IndexTree,1);
	newTree->size=Inf;
	newTree->root=firstIndexPage();
	newTree->root->father=newTree;

	newTree->height=1;
	newTree->l_leaf=newTree->root->pointer[0];

	newTree->l_size=0;
	return newTree;
}

int changeIndex(index_page index, index_type init, index_type new){
	Log();
	if(index==NULL||index->size==Inf)return 1;
	int i;
	For(i,0,index->size)if(index->key[i]==init)break;
	if(i==M)return 0;
	else index->key[i]=new;
	if(i==0)return changeIndex(index->father, init, new);
	return 1;
}

int insertToleaf(leaf_page l, index_type idx, addr_type addr){
	Log();
	if(l->size>=M)return 0;
	int i;
	int lo=findInterval(l->key,idx,l->size);
	if(lo==-1&&!changeIndex(l->father,l->key[0],idx))Error(Fail to change upper index!);
	For(i,l->size,lo+1){
		l->key[i]=l->key[i-1];
		l->addr[i]=l->addr[i-1];
	}
	l->key[lo+1]=idx;
	l->addr[lo+1]=addr;

	l->size++;
	return 1;
}
int insertToleaf_withBuffer(leaf_page l, index_type idx, addr_type addr){
	Log();
	if(l->size!=M)return 0;
	int i;
	//l->size=M
	int lo=findInterval(l->key,idx,l->size);
	if(lo==-1&&!changeIndex(l->father,l->key[0],idx))Error(Fail to change upper index!);
	For(i,l->size,lo+1){
		l->key[i]=l->key[i-1];
		l->addr[i]=l->addr[i-1];
	}
	l->key[lo+1]=idx;
	l->addr[lo+1]=addr;

	l->size++;
	return 1;
}

int insertToIndex_withBuffer(index_page index, int p, index_type new){
	Log();
	int i;
	For(i,index->size,p){
		index->key[i]=index->key[i-1];
	}
	index->key[p]=new;
	return 1;
}

//deserted
int replacePointer(index_page index, int p, leaf_page new){
	Log();
	if(index->size<p)return 0;
	index->pointer[p]=new;
	return 1;
}

int insertPointer_withBuffer(index_page index,int p,leaf_page new){//insert to idx(p)
	Log();
	int i;
	For(i,index->size+1,p){
		index->pointer[i]=index->pointer[i-1];
	}
	index->pointer[p]=new;
	return 1;
}

leaf_page newLeafPage_loaded(leaf_page l, int init, int size){
	Log();
	leaf_page new=newLeafPage();
	int i;
	For(i,init,init+size){
		new->key[i-init]=l->key[i];
		new->addr[i-init]=l->addr[i];
	}
	new->addr[size]=l->addr[init+size];
	new->size=size;
	//
	new->father=l->father;
	return new;
}

index_page newIndexPage_loaded(index_page index, int init, int size){
	Log();
	index_page new=newIndexPage();
	int i;
	For(i,init,init+size){
		new->key[i-init]=index->key[i];
		new->pointer[i-init]=index->pointer[i];
	}
	new->pointer[size]=index->pointer[init+size];
	new->size=size;
	//
	new->father=index->father;
	return new;
}

int splitLeaf(leaf_page l){//related to father
	Log();
	if(l->size<=M)return 0;
	if(l->father->size>=M+1)return 0;
	//safe to load to father
	int pivot;
	For(pivot,0,l->father->size)if(l->father->key[pivot]==l->key[0])break;
	if(pivot==l->father->size)return 0;
	//determine the new index to insert
	index_type newIdx=l->key[(M+1)/2];
	int size1=(M+1)/2;
	int size2=(M+1-(M+1)/2+1);
	//insert pointers to pivot+1 and pivot+2
	//right part to replace
	leaf_page right=newLeafPage_loaded(l,0,size1);
	l->father->pointer[pivot+1]=right;
	//left part to insert
	leaf_page left=newLeafPage_loaded(l,(M+1)/2,size2);
	insertPointer_withBuffer(l->father,pivot+1,left);
	//insert index to pivot+1
	insertToIndex_withBuffer(l->father,pivot+1,newIdx);
	//construct linkage
	//change index
	if(pivot==-1)changeIndex(l->father->father,l->father->key[1],l->father->key[0]);
	//l->leaf
	l->pre->next=left;
	l->next->pre=right;
	left->pre=l->pre;
	left->next=right;
	right->pre=left;
	right->next=l->next;
	
	if(l->father->size==M+1&&!splitIndex(l->father))Error(split index error!);
	//
	freeLeafPage(l);
	return 1;

}
int splitIndex(index_page l){
	if(l->size<=M)return 0;
	if(l->father->size==Inf){
		index_tree tree = (index_tree)l->father;
		index_page newRoot = newIndexPage();
		tree->root=newRoot;
		tree->height++;
		newRoot->pointer[0]=l;
		l->father=newRoot;
	}
	else if(l->father->size>=M+1)return 0;
	//safe to load to father
	int pivot;
	For(pivot,0,l->father->size)if(l->father->key[pivot]==l->key[0])break;
	if(pivot==l->father->size)return 0;
	//determine the new index to insert
	index_type newIdx=l->key[(M+1)/2];
	int size1=(M+1)/2;
	int size2=(M+1-(M+1)/2);
	//insert pointers to pivot+1 and pivot+2
	//right part to replace
	index_page right=newIndexPage_loaded(l,0,size1);
	l->father->pointer[pivot+1]=right;
	//left part to insert
	index_page left=newIndexPage_loaded(l,(M+1)/2+1,size2);
	insertPointer_withBuffer(l->father,pivot+1,left);
	//insert index to pivot+1
	insertToIndex_withBuffer(l->father,pivot+1,newIdx);
	//construct linkage
	//change index
	if(pivot==-1)changeIndex(l->father->father,l->father->key[1],l->father->key[0]);
	
	if(l->father->size==M+1&&!splitIndex(l->father))Error(split index error!);
	//
	freeLeafPage(l);
	return 1;

}
int leftInsertToLeaf(leaf_page l,index_type idx, addr_type addr){
	Log();
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
	Log();
	if(l->size<M){
		if(!insertToleaf(l,idx,addr))Error(Unexpected Full leaf!);
		return 1;
	}
	else if(l->next==NULL)return 0;
	else{//idx is smallest
		index_type lastIdx=l->key[l->size-1];
		addr_type lastAddr=l->addr[l->size-1];
		l->size--;
		insertToleaf(l,idx,addr);
		return rightInsertToLeaf(l->next,lastIdx,lastAddr);
	}
}

int recursiveInsertToLeaf(leaf_page l,index_type idx,addr_type addr){
	Log();
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
	Log();
	if(T->root==NULL)Error(NIL tree!);
	int i;
	int treeHeight=T->height;//search depth
	index_page cur=T->root;//search init
	For(i,0,treeHeight){
		//idx in [lo,hi)
		int lo=findInterval(cur->key,idx,cur->size);
		lo++;
		if(cur->pointer[lo]==NULL&&i<treeHeight-1)Error(NIL index page!);
		else if(cur->pointer[lo]==NULL)Error(NIL leaf page!);
		else cur=cur->pointer[lo];
	}
	//cur is the leaf
	if(cur->size<M){
		if(insertToleaf(cur,idx,addr)){//vacant leaf
			T->l_size++;
			return 1;
		}
	}
	else if(T->l_size<M*pow(M+1,treeHeight)){//rotate
		if(recursiveInsertToLeaf(cur,idx,addr)){
			T->l_size++;
			return 1;
		}
		else Error(Cannot find the vacant leaf!);
	}
	else{//use preset buffer
		if(!insertToleaf_withBuffer(cur,idx,addr))Error(Fail to split leaf!);
		else if(!splitLeaf(cur))Error(Fail to split leaf!);
		else{
			T->l_size++;
			return 1;
		}
	}
	return 1;
}

void putTree(index_tree T){
	printf("Tree height: %d\nLinked leaves number: %d\n",T->height,T->l_size);
	return;
}

int main(void){

	index_tree MainTree=initialIndexTree();
	//test sample
	int sampleSize=100;
	int data[100]={0};
	Initial(data,i,100);
	int i;
	For(i,0,sampleSize)Insert(MainTree,data[i],data+i);
	putTree(MainTree);
	return 0;
}