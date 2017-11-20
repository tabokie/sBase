#ifndef BASIC_MACRO_
#define BASIC_MACRO_

#ifndef
#define Error(note)					({printf(#note);printf("\n");exit(1);})
#endif

#ifndef
#define New(typeName,size)			(typeName*)malloc(sizeof(typeName)*(size))
#endif

#ifndef
#define Initial(array,val,size)		({int i;for(i=0;i<size;i++)array[i]=val;})
#endif

#ifndef
#define PutInts(array,size)			({printf(#array);printf(": ");int i;for(i=0;i<size;i++)printf("%d ",array[i]);printf("\n");})
#endif

#ifndef
#define For(i,a,b)					for(i=a;(a<=b)?(b-i):(i-b)>0;i+=(a<=b)?1:-1)
#endif

#ifndef
#define Log()						({printf("%s",__func__);printf("\n");})
#endif

#ifndef
#define BP(note)					({printf(#note);printf(", In function %s",__func__);printf("\n");})
#endif

#endif	/* BASIC_MACRO_ */