#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <defn.h>
#include <bf.h>
#include "AM.h"
#include "Bpluslib.h"

int AM_errno = AME_OK;

/* Defined Error Strings */
char* AM_Err[11] = {
    "End of File",
    "Not a Bplus file",
    "Invalid file descriptor",
    "Invalid scan descriptor",
    "Couldn't create file",
    "Invalid attribute length",
    "Unable to delete file",
    "Max open files reached",
    "Max open scans reached",
    "Couldn't create B+ tree",
    "B+ tree search failed"
};

/* Global arrays to hold currently open files and scans */
fileStruct openFiles[MAX_OPEN_FILES];
scanStruct openScans[MAX_OPEN_SCANS];

/* Initializes block level and global arrays */
void AM_Init(){
    BF_Init(LRU);
	for(int index = 0; index < MAX_OPEN_FILES; index++){
        /*an i timh tis metavlitis isAvailable isoute me -1
        tote h thesh afti ston pinaka einai diathesimi giati
        na anoiksoume eite sarwsh/arxeio antistoixa. Arxikopoioume
        oles tis theseis twn pinakwn openScans,openFiles se diathesimes*/
		openFiles[index].isAvailable = -1;
	    openScans[index].isAvailable = -1;
        openScans[index].fileDesc = -1;
        openScans[index].blockNum = -1;
        openScans[index].recordNum = -1;
        openScans[index].operator = -1;
        openScans[index].entriesFound = 0;
	}
	return;
}

/* Creates a B+ file and initializes it */
int AM_CreateIndex(char *fileName,
	               char attrType1,
	               int attrLength1,
	               char attrType2,
	               int attrLength2)
{
	if(BF_CreateFile(fileName) != BF_OK){
        return AM_errno = AME_FILE_ERROR;
	}

    /* Type size mismatch */
    if(attrType1 == INTEGER && attrLength1 != sizeof(int)){
        return AM_errno = AME_INVALID_ATTRL;
    }
    if(attrType2 == FLOAT && attrLength2 != sizeof(float)){
        return AM_errno = AME_INVALID_ATTRL;
    }
    if(attrType1 == FLOAT && attrLength1 != sizeof(float)){
        return AM_errno = AME_INVALID_ATTRL;
    }
    if(attrType2 == INTEGER && attrLength2 != sizeof(int)){
        return AM_errno = AME_INVALID_ATTRL;
    }

    BF_Block* block;
	BF_Block_Init(&block);

	int fileDesc;
	char* data;

    BF_OpenFile(fileName, &fileDesc);  //open file to write metadata
	BF_AllocateBlock(fileDesc,block);
    BF_GetBlock(fileDesc,0,block);//get file's first block
	data = BF_Block_GetData(block);

	/* Setting file's first block meta-data */
	/* Format : |A|attrType1|attrLength1|attrType2|attrLength2|...|B+ tree layers|entry counter| */
    memset(data, 'A', sizeof(char));
    memcpy(data+sizeof(char), &attrType1, sizeof(char));
    memcpy(data+2*sizeof(char), &attrLength1, sizeof(int));
    memcpy(data+2*sizeof(char)+sizeof(int), &attrType2, sizeof(char));
    memcpy(data+3*sizeof(char)+sizeof(int), &attrLength2, sizeof(int));
	data[BF_BLOCK_SIZE-1] = 0; //Entry counter
	data[BF_BLOCK_SIZE-2] = 1; //B+ tree INDEX layers

	BF_Block_SetDirty(block);
	BF_UnpinBlock(block);

    printf("File : %s created succesfully.\n",fileName);
    BF_Block_Destroy(&block);
    BF_CloseFile(fileDesc);    //close file and deallocate memory

	return AME_OK;
}

/* Deletes file from disk */
int AM_DestroyIndex(char *fileName) {
	int status = remove(fileName);
    if( status == 0 ){
  	    printf("%s file deleted successfully.\n",fileName);
  	    return AME_OK;
    }
    else{
  	    return AM_errno = AME_INVALID_DEL;
    }
}

/* Opens file reads metadata and stores the information in the global openFiles array */
int AM_OpenIndex(char *fileName){
	BF_Block* block;
    BF_Block_Init(&block);

    int fileIndex;
    char* data;
	int fileDesc;

    BF_OpenFile(fileName,&fileDesc);
    BF_GetBlock(fileDesc,0,block);//get file's first block
	data = BF_Block_GetData(block);
	if(data[0] != 'A'){
        BF_UnpinBlock(block);
        BF_Block_Destroy(&block);
        return AM_errno = AME_NOT_BPLUS;
    }
    /*Edw pernoume tis times ,pou mas endiaferoun gia to arxeio,
    apo to metadata block(block 0) etsi wste na tis apothikefsoume
    stin global domh openFiles.
    */
    char a1,a2;
    int al1,al2,numOfEntries;
    memcpy(&a1,data+sizeof(char),sizeof(char));
    memcpy(&al1,data+2*sizeof(char),sizeof(int));
    memcpy(&a2,data+2*sizeof(char)+sizeof(int),sizeof(char));
    memcpy(&al2,data+3*sizeof(char)+sizeof(int),sizeof(int));
    numOfEntries = data[BF_BLOCK_SIZE-1];

    /* Store file's metadata to global array for later use */
	for(fileIndex = 0; fileIndex < MAX_OPEN_FILES; fileIndex++){
		if(openFiles[fileIndex].isAvailable == -1){
			openFiles[fileIndex].isAvailable = 0;
            strcpy(openFiles[fileIndex].fname,fileName);
            openFiles[fileIndex].attrType1 = a1;
            openFiles[fileIndex].attrType2 = a2;
            openFiles[fileIndex].attrLength1 = al1;
            openFiles[fileIndex].attrLength2 = al2;
			break;
		}
	}
    BF_UnpinBlock(block);
    BF_Block_Destroy(&block);
	if(fileIndex == MAX_OPEN_FILES)
		return AM_errno = AME_MAX_FILES;

    printf("Open index success file : %s with fileDesc : %d and index : %d\n",fileName,fileDesc,fileIndex);
    return fileIndex;   //returns file's global array index
}

/* Closes file and removes it from the openFiles array */
int AM_CloseIndex(int fileDesc) {
    //To fileDesc einai sto diasthma [0,MAX_OPEN_FILES-1]
	if(fileDesc<0 || fileDesc >= MAX_OPEN_FILES)
        return AM_errno = AME_INVALID_FD;
    //stin openIndex psaxnoume thesi me fileD = -1,dhlwnei oti einai diathesimh h thesh
    openFiles[fileDesc].isAvailable = -1;
	BF_CloseFile(fileDesc);
    return AME_OK;
}

/* Inserts Entry in B+ tree */
int AM_InsertEntry(int fileDesc, void *value1, void *value2) {
    //To fileDesc einai sto diasthma [0,MAX_OPEN_FILES-1]
	if(fileDesc<0 || fileDesc >= MAX_OPEN_FILES)
        return AM_errno = AME_INVALID_FD;

    BF_Block* block;
    BF_Block_Init(&block);

    char* data;
    char attrType1 = openFiles[fileDesc].attrType1;
    int attrLength1 = openFiles[fileDesc].attrLength1;
    char attrType2 =  openFiles[fileDesc].attrType2;
    int attrLength2 =  openFiles[fileDesc].attrLength2;
    int sum = attrLength1+attrLength2;

    /* Pairnoume to metadata block etsi wste na doume
    poses eggrafes exei to arxeio. Apo ton arithmo eggrafwn
    eksartatai h leitourgia tis insertEntry.*/
    BF_GetBlock(fileDesc,0,block);
	data = BF_Block_GetData(block);
	int numOfEntries = data[BF_BLOCK_SIZE-1];
	BF_UnpinBlock(block);


    int block_num;
	int destBlockNum;
    /*Ta prota 2 bytes eite se index block eite se data block, apoteloun
    thn kefalida tou block. Sto byte[0] vazoume ton tupo tou block (1 an einai data block kai
    0 an einai index block). Gia na einai pio efkolh h eisagogi mas, xreiazomaste tin metavlhth
    index, h opoia einai o arithmos bytes tis kefalidas.*/
    int index = 2;
    Stack stack;//xreiazetai gia to spasimo index block

    genericPrint(value1,value2,attrType1,attrType2);    //print entry to be inserted

    if(numOfEntries == 0){//an to arxeio  den exei egrafes ftiaxnoume B+ dentro me riza k 2 data blocks
        if(createBplusTree(fileDesc,value1,value2) != AME_OK){
            return AM_errno = AME_CREATE_BPLUS;
        }
	}

	else if(numOfEntries > 0){
        //pernoume stin metavliti destBlockNum ton arithmo tou block pou tha mpei to entry
		if(bPlusSearch(fileDesc,value1,&destBlockNum,&stack) != AME_OK){
            return AM_errno = AME_SEARCH_BPLUS;
        }
		BF_GetBlock(fileDesc,destBlockNum,block);
		data = BF_Block_GetData(block);
        int currNumOfEntries = data[1];//kratame ton arithmo eggrafwn tou blocks
        BF_UnpinBlock(block);

		if( (currNumOfEntries+1) * sum + 3 <= BF_BLOCK_SIZE){
            /*Edw tha mpei an o proorismos tis eggrafhs exei keno xwro ,
            opote thn xoraei. Sto else tha mpei an DEN thn xoraei.*/
            insertDataSorted(fileDesc,destBlockNum,value1,value2);
		}
		else{
            /*Edw tha mpei giati to block proorismoy ths eggrafhs einai gemato.
            Synepws tha xreiastei to block afto na spasei sta 2 kai na isomoirastoun
            oi eggrafes tou sta 2 blocks.*/
            int newBlockNum;
            //kanoume split to block,kai sthn metavliti newBlockNum pairnoume ton arithmo tou neou block
            splitDataBlock(fileDesc,destBlockNum,&newBlockNum);

            BF_GetBlock(fileDesc,newBlockNum,block);
            data = BF_Block_GetData(block);
            /*ws edw, exoume spasei to data block kai exoume valei taksinomimenes tis eggrafes kai sta duo block
            meta to spasimo kai to data insertion pairnoume to prwto kleidi tou deksiou block gia na
            to anevasoume sto index level, kai episis gia na doume h nea eggrafh an tha mpei sto neo h sto
            palio block.*/
			void* keyForInsertIndex = malloc(attrLength1);
            memcpy(keyForInsertIndex,data+index,attrLength1);
            BF_Block_SetDirty(block);
            BF_UnpinBlock(block);//unpin to prohgoumeno

            if(genericCompare(keyForInsertIndex,value1,attrType1) == 1)//tha paei sto proto
                insertDataSorted(fileDesc,destBlockNum,value1,value2);
            else//tha paei sto neo block
                insertDataSorted(fileDesc,newBlockNum,value1,value2);
            insertIndexSorted(fileDesc,value1,keyForInsertIndex,newBlockNum,&stack);
            free(keyForInsertIndex);
        }
	}
    //afksanoume ton metriti eggrafwn sto metadata block (kata 1)
	BF_GetBlock(fileDesc,0,block);
	data = BF_Block_GetData(block);
	data[BF_BLOCK_SIZE-1]++;
    BF_Block_SetDirty(block);
	BF_UnpinBlock(block);
	printf("Entry number %d inserted in fileDesc : %d\n\n",data[BF_BLOCK_SIZE-1],fileDesc);
	BF_Block_Destroy(&block);
	return AME_OK;
}

/* Opens a scan for a file and stores scan information in the global openScans array */
int AM_OpenIndexScan(int fileDesc, int op, void *value) {
    int scanIndex;
    int destBlockNum;
    int recordNum = -1;//gia na kseroume an uparxei
    Stack stack;
    char* data;

	BF_Block* block;
	BF_Block_Init(&block);
	/*vriskoume apo ta open files to attrType1,attrType2 klp*/
    char at1 = openFiles[fileDesc].attrType1;
    char at2 = openFiles[fileDesc].attrType2;
    int al1 = openFiles[fileDesc].attrLength1;
    int al2 = openFiles[fileDesc].attrLength2;
    int sum = al1+al2;

    /* psaxnoume sto B+ dentro se poio block brisketai to value p dw8hke */
	bPlusSearch(fileDesc,value,&destBlockNum,&stack);

	//psaxnoume mesa sto destBlockNum
    BF_GetBlock(fileDesc,destBlockNum,block);
	data = BF_Block_GetData(block);
    void* key = malloc(al1);
    /*Briskoume mesa sto block to value p dw8hke k apo8hkeuoume thn 8esh tou
    gia telestes not_equal less_than k less_than_or_equal ksekiname apo thn prwth eggrafh
    sto prwto block dedomenwn. Gia to greater_than an dn yparxei akribws h timh p dw8hke
    briskei thn amesws epomenh*/

    for(int entryNum = 0; entryNum < data[1]; entryNum++){
        memcpy(key,data+2 + entryNum*sum,al1);
        //genericPrint(key,value,at1,at1);
        if(op == EQUAL){
            if(genericCompare(key,value,at1) == 0){
                recordNum = entryNum;//recornNum = recordIndex mesa sto block
                break;
            }
        }
        else if(op == GREATER_THAN){
            if(genericCompare(key,value,at1) == 1){ //stamatame ekei p to kleidi einai > value
                recordNum = entryNum;//recornNum = recordIndex mesa sto block
                break;
            }
        }
        else if(op == GREATER_THAN_OR_EQUAL){
            if(genericCompare(key,value,at1) != -1){ //stamatame ekei p to kleidi einai > value
                recordNum = entryNum;//recornNum = recordIndex mesa sto block
                break;
            }
        }
    }
/* Psaxnoume gia keni thesi ston pinaka, an broume apo8hkeuoume ton telesth, to value, to fileDesc
tou arxeiou p kanoume scan, ton ari8mo t block pou brisketai to value k to offset tou mesa sto block*/
    for(scanIndex = 0; scanIndex < MAX_OPEN_SCANS; scanIndex++){
		if(openScans[scanIndex].isAvailable == -1){//psaxei gia keni thesi
			openScans[scanIndex].isAvailable = 0;
            openScans[scanIndex].fileDesc = fileDesc;
            openScans[scanIndex].operator = op;
			if(op == NOT_EQUAL || op == LESS_THAN_OR_EQUAL || op == LESS_THAN){
                openScans[scanIndex].blockNum = 2;  //prepei na arxizoume ap thn arxh
                openScans[scanIndex].recordNum = 0;
            }else{
                openScans[scanIndex].blockNum = destBlockNum;
    			openScans[scanIndex].recordNum = recordNum;
            }
            openScans[scanIndex].value = malloc(al1);//stin close na kanoume free
            memcpy(openScans[scanIndex].value,value,al1);
			break;
		}
	}
    if(scanIndex == MAX_OPEN_SCANS)
        return AM_errno = AME_MAX_SCANS;
    free(key);
    BF_UnpinBlock(block);
    BF_Block_Destroy(&block);
    return scanIndex;
}

/* Finds and returns the next entry of a scan */
void *AM_FindNextEntry(int scanDesc) {
    BF_Block* block;
    BF_Block_Init(&block);

/* Briskoume mesw tou scanDesc to arxeio to opoio sarwnoume wste na paroume
ta attributes tou */
    int fileDesc = openScans[scanDesc].fileDesc;

    char attrType1 = openFiles[fileDesc].attrType1;
    char attrType2 = openFiles[fileDesc].attrType2;
    int attrLength1 = openFiles[fileDesc].attrLength1;
    int attrLength2 = openFiles[fileDesc].attrLength2;
    int sum = attrLength1 + attrLength2;

/*Pairnoume to block sto opoio brisketai h prwth eggrafh */
    int blockNum = openScans[scanDesc].blockNum;
	int recordNum = openScans[scanDesc].recordNum;
    int op = openScans[scanDesc].operator;
    int entriesFound = openScans[scanDesc].entriesFound;
    void *value = malloc(attrLength1);
    memcpy(value,openScans[scanDesc].value,attrLength1);

/*an den yparxei allh eggrafh EOF k epistrefoume NULL */
    if(recordNum == -1){
        free(value);
        AM_errno = AME_EOF;
        BF_Block_Destroy(&block);
		return NULL;
    }
    BF_GetBlock(fileDesc,blockNum,block);
	char* data = BF_Block_GetData(block);
    void* value2 = malloc(attrLength2); //can't free this
    void* key = malloc(attrLength1);
    void* nextKey = malloc(attrLength1);

/* Analoga me ton telesth arxizoume h apo thn arxh h apo thn prwth eggrafh p ikanopoiei ton telesth
pairnoume to value2 ths eggrafhs kai elegxoume an kai h epomenh eggrafh ikanopoiei k ayth ton telesth
an nai auksanoume to offset ths eggrafhs, an xreiastei allazoume k block. an den 8etoume to offset se -1
wste thn epomenh fora p 8a klh8ei h synarthsh na epistrepsei NULL k EOF */
	if(op == EQUAL){
		int entryNum;
        for(entryNum = recordNum; entryNum < data[1]; entryNum++){
            memcpy(key,data+2+entryNum*sum,attrLength1);
            if(genericCompare(key,value,attrType1) == 0){
				memcpy(value2,data+2+entryNum*sum+attrLength1,attrLength2);
                openScans[scanDesc].entriesFound++;
				if(entryNum != data[1]-1){
					memcpy(nextKey,data+2+(entryNum+1)*sum,attrLength1);
					if(genericCompare(nextKey,value,attrType1) == 0)
                    	openScans[scanDesc].recordNum++;
					else
						openScans[scanDesc].recordNum = -1;//den uparxei allh
					break;
				}
                else
                    openScans[scanDesc].recordNum = -1;
				break;
            }
        }
		if(entryNum == data[1]){//teliwse to loop kai den vrike iso klidi
			value2 = NULL;
        }
    }
    else if(op == NOT_EQUAL){
		int entryNum;
        for(entryNum=recordNum;entryNum<data[1];entryNum++){
            memcpy(key,data +2 + entryNum*sum,attrLength1);
	        if(genericCompare(key,value,attrType1) != 0){
	        	memcpy(value2,data+2+entryNum*sum+attrLength1,attrLength2);
                openScans[scanDesc].entriesFound++;
				break;
	        }
        }
		if((entryNum == data[1]-1 || entryNum == data[1]) && data[BF_BLOCK_SIZE-1] != -10){//an exei teleiwsei to panw for kai dn eimaste sto teleytaio block
			openScans[scanDesc].blockNum = data[BF_BLOCK_SIZE-1];//vazoume to epomeno block
			openScans[scanDesc].recordNum = 0;//arxizoume apo tin proti tou egraffh
		}
		else if(data[BF_BLOCK_SIZE-1] == -10 && entryNum == data[1]-1)
			openScans[scanDesc].recordNum = -1;//sinthiki telous arxeiou
		else{
            entryNum++;//start from next entry
			while(entryNum != data[1]){
				memcpy(key,data +2 + entryNum*sum,attrLength1);
	            if(genericCompare(key,value,attrType1) != 0){
	            	openScans[scanDesc].recordNum = entryNum;
					break;
	            }
				entryNum++;
			}
		}
    }
	else if (op == LESS_THAN_OR_EQUAL){
        int entryNum = recordNum;
        memcpy(key,data +2 + recordNum*sum,attrLength1);
        if(genericCompare(key,value,attrType1) != 1){ // 8eloume key <= value
        	memcpy(value2,data+2+entryNum*sum+attrLength1,attrLength2);
            openScans[scanDesc].entriesFound++;
			if(recordNum < data[1]-1){
				memcpy(key,data +2 + (recordNum+1)*sum,attrLength1);
				if(genericCompare(key,value,attrType1) != 1)
					openScans[scanDesc].recordNum++;
				else
					openScans[scanDesc].recordNum = -1;
			}
			else if (data[BF_BLOCK_SIZE-1] != -10 ){//pernoume epomeno block
				int nextBlockNum = data[BF_BLOCK_SIZE-1];
				BF_UnpinBlock(block);
                BF_GetBlock(fileDesc,nextBlockNum,block);
            	data = BF_Block_GetData(block);
				memcpy(key,data +2,attrLength1);
				if(genericCompare(key,value,attrType1) != 1){
					openScans[scanDesc].recordNum = 0;
                    openScans[scanDesc].blockNum = nextBlockNum;
				}
				else
					openScans[scanDesc].recordNum = -1;

			}
        }

	}
    else if (op == LESS_THAN){
        int entryNum = recordNum;
        memcpy(key,data +2 + recordNum*sum,attrLength1);
        if(genericCompare(key,value,attrType1) == -1){ // 8eloume key <= value
        	memcpy(value2,data+2+entryNum*sum+attrLength1,attrLength2);
            openScans[scanDesc].entriesFound++;
			if(recordNum < data[1]-1){
				memcpy(key,data +2 + (recordNum+1)*sum,attrLength1);
				if(genericCompare(key,value,attrType1) == -1)
					openScans[scanDesc].recordNum++;
				else
					openScans[scanDesc].recordNum = -1;
			}
			else if (data[BF_BLOCK_SIZE-1] != -10 ){//pernoume epomeno block
				int nextBlockNum = data[BF_BLOCK_SIZE-1];
				BF_UnpinBlock(block);
                BF_GetBlock(fileDesc,nextBlockNum,block);
            	data = BF_Block_GetData(block);
				memcpy(key,data +2,attrLength1);
				if(genericCompare(key,value,attrType1) == -1){
					openScans[scanDesc].recordNum = 0;
                    openScans[scanDesc].blockNum = nextBlockNum;
				}
				else
					openScans[scanDesc].recordNum = -1;

			}
        }
    }
    else if (op == GREATER_THAN){
        int entryNum = recordNum;
        memcpy(key,data +2 + recordNum*sum,attrLength1);
        if(genericCompare(key,value,attrType1) == 1){ // 8eloume key > value
        	memcpy(value2,data+2+entryNum*sum+attrLength1,attrLength2);
            openScans[scanDesc].entriesFound++;
			if(recordNum < data[1]-1){
				memcpy(key,data +2 + (recordNum+1)*sum,attrLength1);
				if(genericCompare(key,value,attrType1) == 1)
					openScans[scanDesc].recordNum++;
				else
					openScans[scanDesc].recordNum = -1;
			}
			else if (data[BF_BLOCK_SIZE-1] != -10 ){//pernoume epomeno block
				int nextBlockNum = data[BF_BLOCK_SIZE-1];
				BF_UnpinBlock(block);
                BF_GetBlock(fileDesc,nextBlockNum,block);
            	data = BF_Block_GetData(block);
				memcpy(key,data +2,attrLength1);
				if(genericCompare(key,value,attrType1) == 1){
					openScans[scanDesc].recordNum = 0;
                    openScans[scanDesc].blockNum = nextBlockNum;
				}
				else
					openScans[scanDesc].recordNum = -1;

			}
            else if(data[BF_BLOCK_SIZE-1] == -10 && entryNum == data[1]-1)
                openScans[scanDesc].recordNum = -1;//sinthiki telous arxeiou
        }
    }
    else if (op == GREATER_THAN_OR_EQUAL){
        int entryNum = recordNum;
        memcpy(key,data +2 + recordNum*sum,attrLength1);
        if(genericCompare(key,value,attrType1) != -1){ // 8eloume key > value
        	memcpy(value2,data+2+entryNum*sum+attrLength1,attrLength2);
            openScans[scanDesc].entriesFound++;
			if(recordNum < data[1]-1){
				memcpy(key,data +2 + (recordNum+1)*sum,attrLength1);
				if(genericCompare(key,value,attrType1) != -1)
					openScans[scanDesc].recordNum++;
				else
					openScans[scanDesc].recordNum = -1;
			}
			else if (data[BF_BLOCK_SIZE-1] != -10 ){//pernoume epomeno block
				int nextBlockNum = data[BF_BLOCK_SIZE-1];
				BF_UnpinBlock(block);
                BF_GetBlock(fileDesc,nextBlockNum,block);
            	data = BF_Block_GetData(block);
				memcpy(key,data +2,attrLength1);
				if(genericCompare(key,value,attrType1) != -1){
					openScans[scanDesc].recordNum = 0;
                    openScans[scanDesc].blockNum = nextBlockNum;
				}
				else
					openScans[scanDesc].recordNum = -1;
			}
            else if(data[BF_BLOCK_SIZE-1] == -10 && entryNum == data[1]-1)
                openScans[scanDesc].recordNum = -1;//sinthiki telous arxeiou
        }
    }
    free(key);
    free(nextKey);
    free(value);
    BF_UnpinBlock(block);
    BF_Block_Destroy(&block);

    return value2;
}

/* Closes a scan and deletes it from the global array */
int AM_CloseIndexScan(int scanDesc) {
    /* an uparxei to scanDesc apeleu8erwnoume thn 8esh ston pinaka kai 8etoume
    default values sta dedomena */
    if(scanDesc<0 || scanDesc >= MAX_OPEN_FILES)
        return AM_errno = AME_INVALID_SD;

    openScans[scanDesc].isAvailable = -1;
    openScans[scanDesc].fileDesc = -1;
    openScans[scanDesc].blockNum = -1;
    openScans[scanDesc].recordNum = -1;
    openScans[scanDesc].operator = -1;
    openScans[scanDesc].entriesFound = 0;
    free(openScans[scanDesc].value);
    return AME_OK;
}

/* Prints last error code information */
void AM_PrintError(char *errString) {
    printf("%s%s\n",errString,AM_Err[-1*(AM_errno+1)]);
}


void AM_Close() {
    BF_Close();
}
