#ifndef BPLUS_H_
#define BPLUS_H_

#define MAX_TREE_DEPTH 20
/* Defined Structs */
typedef struct fileStruct{
	int isAvailable;
    char fname[20];
    char attrType1;
    char attrType2;
    int attrLength1;
    int attrLength2;
}fileStruct;

typedef struct stack{
    int top;
    int arr[MAX_TREE_DEPTH];
}Stack;

typedef struct scanStruct{
	int isAvailable;
    int fileDesc;
	void* value;
    int blockNum;
    int recordNum;
    int operator;
    int entriesFound;
}scanStruct;

int genericCompare(void*, void*, char);
int genericPrint(void*, void*, char, char);
int printAllEntries(int);

int createBplusTree(int ,void* ,void*);
int bPlusSearch(int, void*, int*, Stack*);

int shiftingData(char*, int, int ,int ,void*, int, void*, int);
int insertDataSorted(int, int, void*, void*);
int insertIndexSorted(int, void*, void*, int, Stack*);
int splitDataBlock(int,int,int*);
int splitIndexBlock(int ,int,int*, void* );

#endif
