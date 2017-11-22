#ifndef BPLUSTREE_H_
#define BPLUSTREE_H_

/*
about B+tree
1.find corresponding leaf
2.if not full
    2.1 Insert
3.if full and leaves not full->rotate
    3.0 premise: maintain a size element for linked leaves -> premise denied
    >recursive allowed: insertToleaf
        3.1 from pivot to left then to right
        3.1.1 if left...
4.if leaves all full
    force insert
    split full leaf and index

NOTICE: don't allow multiple ref!
*/


/*!
    \def M
    \breif Unit size preset by user.
*/
#ifndef M
#define M               (3)
#endif

/*
    \def NIL
    \brief Sign for tree log node, or NIL node in the tree.
*/
#ifndef NIL
#define NIL             (-1e3)
#endif


/*
    \def NINF
    \brief Pre defined minimum element value.
*/
#ifndef NINF
#define NINF            (-1e5)
#endif


/*
    \def index_type,addr_type
    \brief Storing data type for the tree.
*/
typedef int index_type;
typedef int* addr_type;


/*
    \brief Basic data structure.
    \NOTICE Special alignment: size and father
*/
typedef struct IndexPage{
    int size;
    struct IndexPage* father;
    index_type* key;
    struct IndexPage** pointer;
}* index_page;

typedef struct LeafPage{
    int size;
    index_page father;
    index_type* key;
    addr_type* addr;
    struct LeafPage* next;
    struct LeafPage* pre;
}* leaf_page;

typedef struct IndexTree{
    int size;//recognize code
    index_page root;
    int height;
    leaf_page l_leaf;
    int l_size;
}* index_tree;


#define putIndexPage(page)          ({int in_i_;printf("(");printf("[%d] ",((index_page)page)->size);For(in_i_,0,page->size){if(((index_page)page)->key[in_i_]<0)printf("NINF ");else printf("%d ",((index_page)page)->key[in_i_]);}printf(") ");})
#define putLeafPage(page)           ({int in_i_;printf("(");printf("[%d] ",((leaf_page)page)->size);For(in_i_,0,((leaf_page)page)->size){if(((leaf_page)page)->key[in_i_]<0)printf("NINF ");else printf("%d ",((leaf_page)page)->key[in_i_]);}printf(") ");})


/***********************************************************
    Tree function class
************************************************************/
/*
    \brief Initial new tree.
*/
index_tree initialIndexTree(void);
/*
    \brief Set and get current tree.
*/
extern index_tree GLOBAL_Pointer;
void setCurrentTree(index_tree t);
index_tree getCurrentTree(void);
/*
    \brief Print the tree log to console.
*/
void putTree(index_tree T);
/*
    \brief (index,addr) inserting routine.
*/
int Insert(index_tree T, index_type idx, addr_type addr);




/***********************************************************
    Leaf page function class
************************************************************/
/*
    \brief Construct and destory.
    Two routines to initial a new leaf:
    > initial a null leaf node
    > initial a non-empty node with data copied from designated leaf.
        \param l Designated leaf pointer.
        \param init Starting idx for copying.
        \param size Copy amount.
    To destory a leaf.
        \param l The leaf pointer to destory.
*/
leaf_page newLeafPage(void);
leaf_page newLeafPage_loaded(leaf_page l, int init, int size);
int freeLeafPage(leaf_page l);

/*
    \brief Basic routine for leaf spliting.
    \param l The leaf pointer to split.
*/
int splitLeaf(leaf_page l);

/*
    \brief Basic insersion on leaf
    \param l Current leaf pointer to try insert.
    \param idx Uninserted index.
    \param addr Uninserted address.
*/
int insertToleaf(leaf_page l, index_type idx, addr_type addr);
int insertToleaf_withBuffer(leaf_page l, index_type idx, addr_type addr);

/*
    \brief Rotated insersion on linked leaves
    \param l Current leaf pointer to try insert.
    \param idx Uninserted index.
    \param addr Uninserted address.
*/
int recursiveInsertToLeaf(leaf_page l,index_type idx,addr_type addr);
int leftInsertToLeaf(leaf_page l,index_type idx, addr_type addr);
int rightInsertToLeaf(leaf_page l, index_type idx, addr_type addr);


/***********************************************************
    Index page function class
************************************************************/
/*
    \brief Construct and destory.
    Two routines to initial a new index:
    > initial a null index node
    > initial a special node for tree init routine.
    > initial a non-empty node with data copied from designated index.
        \param l Designated index pointer.
        \param init Starting idx for copying.
        \param size Copy amount.
    To destory a index node.
        \param l The index page pointer to destory.
*/
index_page newIndexPage(void);
index_page firstIndexPage(void);
index_page newIndexPage_loaded(index_page index, int init, int size);
int freeIndexPage(index_page index);

/*
    \brief Manipulating specific index.
    Function changeIndex
        \brief Change an existing index.
        \param index Pointer to the index page this index supposed to be.
        \param init Previous index value.
        \param new New value for this index.
    Function addIndex
        \brief Add an index to a empty pivot.
        \param index Pointer to this index page.
        \param new The new index value.
*/
int changeIndex(index_page index, index_type init, index_type new);
int addIndex(index_page index, index_type new);

/*
    \brief Basic index insersion on index page.
    \param index Current index page pointer to try insert.
    \param p Preset inserting pivot.
    \param new Uninserted index value.
*/
int insertToIndex_withBuffer(index_page index, int p, index_type new);

/*
    \brief Basic pointer manipulation on index page.
    Function replacePointer try replace the previous pointer with new one.
    Function insertPointer_withBuffer try insert one new pointer.
    \param index Current index page pointer to try insert.
    \param p Preset operating pivot.
    \param new Uninserted index value.
*/
int replacePointer(index_page index, int p, leaf_page new);
int insertPointer_withBuffer(index_page index,int p, leaf_page new);

/*
    \brief Basic routine for leaf spliting.
    \param l The leaf pointer to split.
*/
int splitIndex(index_page l);


/***********************************************************
    Additional function class
************************************************************/
/*
    \brief For linear array, find a interval [a,a+1) where idx in.
    Return a as result.
    \param l The index_type array to search.
    \param idx The index value to search for.
    \param size The search scope of array.
*/
int findInterval(index_type* index, index_type idx, int size);



#endif  /* BPLUSTREE_H_ */