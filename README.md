# B+ Tree Indexed DB System - Implementation of Database Systems Course NKUA 2017

A functional database that supports data insertion, read and deletion APIs and queries
B+ Tree Index Implementation with a shared library that wraps a block level IO.

Το Create an Index:
``` C
int AM_CreateIndex(
  char *fileName, 
  char attrType1, 
  int attrLength1,
  char attrType2, 
  int attrLength2 
);
```
To Insert Data into an index:
``` c
int AM_InsertEntry(int fileDesc, void *value1, void *value2)

To Delete an Index:

int AM_DestroyIndex(
  char *fileName
);
```
Upon index creation you can search the index:
```c
int AM_OpenIndexScan(
  int fileDesc, 
  int op, // Multiple operator supported eg. EQUAL, GREATER_THAN, GREATER_THAN_EQUAL
  void *value
);
```
Then use this function in a loop to get all the found items until NULL returned
```c
void *AM_FindNextEntry(
  int scanDesc
);
```
Example of API usage:
```c
eage = 19;

if ((scan1 = AM_OpenIndexScan(eAentry, NOT_EQUAL, (void *) &eage)) < 0) {  //Find all entries with age != 19
		sprintf(errStr, "Error in AM_OpenIndexScan called on %s \n", empAge);
		AM_PrintError(errStr);
}

while ((cvalue = (char*) AM_FindNextEntry(scan1)) != NULL) {
  printf("'%s' \n", cvalue);
}
```
