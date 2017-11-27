#define LIST_OF_BITS \
    BIT_OP(0) 	\
    BIT_OP(1) 	\
    BIT_OP(2)	\
    BIT_OP(3)	\
    BIT_OP(4)	\
    BIT_OP(5)	\
    BIT_OP(6)	\
    BIT_OP(7)	\
    BIT_OP(8)	\
    BIT_OP(9)	\
    BIT_OP(10)	\
    BIT_OP(11)	\
    BIT_OP(12)	\
    BIT_OP(13)	\
    BIT_OP(14)	\
    BIT_OP(15)	\
    BIT_OP(16)	\
    BIT_OP(17)	\
    BIT_OP(18)	\
    BIT_OP(19)	\
    BIT_OP(20)	\
    BIT_OP(21)	\
    BIT_OP(22)	\
    BIT_OP(23)	\
    BIT_OP(24)	\
    BIT_OP(25)	\
    BIT_OP(26)	\
    BIT_OP(27)	\
    BIT_OP(28)	\
    BIT_OP(29)	\
    BIT_OP(30)	\
    BIT_OP(31)	

typedef union COMPLEX_INT{
	unsigned __int64 _ID;
	struct 32_DIGIT_HEAD{
		#define BIT_OP(NO) unsigned BIT##NO;
		LIST_OF_BITS
		#undef BIT_OP
		unsigned int _remainder;
	}_DIGIT;
}cint;

typedef __int64 inid;

// page and addr to inid
// (internal identifier)
inid toINID(int page_no, int addr){
    
}

// inid to page and addr

int BIT(cint n,int i){
	if(i<0||i>31)return -1;
	switch(i){
		#define BIT_OP(NO) case NO:return n._DIGIT.BIT##NO;
		LIST_OF_BITS
		#undef BIT_OP
	}
}