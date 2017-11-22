#ifndef BASIC_MACRO_
#define BASIC_MACRO_

#ifndef Error
#define Error(note)                 ({printf(#note);printf("\n");exit(1);})
#endif

#ifndef New
#define New(typeName,size)          (typeName*)malloc(sizeof(typeName)*(size))
#endif

#ifndef Initial
#define Initial(array,val,size)     ({int in_i_;for(in_i_=0;in_i_<size;in_i_++)array[in_i_]=val;}) 
#endif

#ifndef PutInts
#define PutInts(array,size)         ({printf(#array);printf(": ");int in_i_;for(in_i_=0;in_i_<size;in_i_++)printf("%d ",array[in_i_]);printf("\n");}) 
#endif

#ifndef For
#define For(i,a,b)                  for(i=a;(a<=b)?(b-i):(i-b)>0;i+=(a<=b)?1:-1)
#endif

#ifndef Log
#define Log()                       ({printf("%s",__func__);printf("\n");})
#endif

#ifndef BP
#define BP(note)                    ({printf(#note);printf(", In function %s",__func__);printf("\n");})
#endif

#endif  /* BASIC_MACRO_ */