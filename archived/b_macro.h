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
#define For(i,a,b)                  for(i=(a);(((a)<=(b))?((b)-(i)):((i)-(b)))>0;i+=(((a)<=(b))?1:-1))
#endif

#ifndef Log
#define Log()                       ({printf("%s",__func__);printf("\n");})
#endif

#ifndef BP
#define BP(note)                    ({printf(#note);printf(", In function %s",__func__);printf("\n");})
#endif

#ifndef BOOL_TYPE_
#define BOOL_TYPE_
typedef enum{false, true} bool;
#endif

#define CLOCK_INIT					struct timespec start,end;
#define CLOCK_START					clock_gettime(CLOCK_MONOTONIC,&start);
#define CLOCK_END					clock_gettime(CLOCK_MONOTONIC,&end);
#define CLOCK_SHOW(note)			printf("Elapsed time(%s): %.6f ms\n",#note,\
									1000*(end.tv_sec-start.tv_sec)+(double)(end.tv_nsec-start.tv_nsec)/1000000);

#endif  /* BASIC_MACRO_ */