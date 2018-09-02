#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <defn.h>
#include <bf.h>
#include "AM.h"
#include "Bpluslib.h"

extern fileStruct openFiles[MAX_OPEN_FILES];
extern scanStruct openScans[MAX_OPEN_SCANS];

/* Compares 2 void* values given their type. Returns 1 if val1 > val2 */
int genericCompare(void * val1,void * val2 ,char attr){
	if (attr == INTEGER){
		if(*(int*)val1 > *(int*)val2)
			return 1;
		else if(*(int*)val1 == *(int*)val2)
			return 0;
		else
			return -1;
      }
    else if(attr == FLOAT){
        if(*(float*)val1 > *(float*)val2)
          return 1;
        else if(*(float*)val1 == *(float*)val2)
          return 0;
        else
          return -1;
      }
    else if(attr == STRING){
        if(strcmp((char*)val1, (char*)val2) > 0)
          	return 1;
        else if(strcmp((char*)val1, (char*)val2) == 0)
     		return 0;
        else
        	return -1;
  	}
}

/* Prints 2 void* values given their type */
int genericPrint(void* value1,void* value2,char attrType1,char attrType2){
    if(attrType1 == FLOAT && attrType2 == FLOAT)
	    printf("%f %f \n",*(float*) value1,*(float*) value2);
	if(attrType1 == FLOAT && attrType2 == INTEGER)
	    printf("%f %d \n",*(float*) value1,*(int*) value2);
	if(attrType1 == FLOAT && attrType2 == STRING)
		printf("%f %s \n",*(float*) value1,(char*) value2);

	if(attrType1 == INTEGER && attrType2 == INTEGER)
		printf("%d %d \n",*(int*) value1,*(int*) value2);
	if(attrType1 == INTEGER && attrType2 == STRING)
		printf("%d %s \n",*(int*) value1,(char*) value2);
	if(attrType1 == INTEGER && attrType2 == FLOAT)
		printf("%d %f \n",*(int*) value1,*(float*) value2);

	if(attrType1 == STRING && attrType2 == STRING)
		printf("%s %s \n",(char*) value1,(char*) value2);
	if(attrType1 == STRING && attrType2 == FLOAT)
		printf("%s %f \n",(char*) value1,*(float*) value2);
	if(attrType1 == STRING && attrType2 == INTEGER)
		printf("%s %d \n",(char*) value1,*(int*) value2);
    return AME_OK;
}

/* Creates a B+ tree with a root and 2 data blocks */
int createBplusTree(int fileDesc,void* value1,void* value2){
    /*Se aftin tin sinartisi dimiourgoume to B+ dentro me ton ekshs tropo.
    Prota dhmiourgoume ena block, tin riza, kai meta 2 data blocks.
    Ystera, prosthetoume to kleidi tis protis eggrafhs san to proto kleidi
    ths rizas, kai meta prosthetoume tin prwth eggrafh sto deksi paidi tou
    prwtou kleidiou tis rizas. Etsi ousiastika h arxikh domh tou B+ dentrou
    einai h riza, me ena kleidi, kai 2 paidia, ena apo ta opoia einai keno
    kai to allo periexei tin prwth eggrafh.
    */
    char attrType1 = openFiles[fileDesc].attrType1;
    int attrLength1 = openFiles[fileDesc].attrLength1;
    char attrType2 = openFiles[fileDesc].attrType2;
    int attrLength2 = openFiles[fileDesc].attrLength2;
	char* data;
	int index = 2;
	BF_Block* block;
	BF_Block_Init(&block);
	BF_AllocateBlock(fileDesc,block);//allocate 1st B+ tree block (root)
	BF_GetBlock(fileDesc,1,block);
	data = BF_Block_GetData(block);
	data[0] = 0;  //eurethriou
	data[1] = 0;  //posa kleidia eixei h riza
	//index block : |isDataByte|keyCounter|leftChild(byte)|key|rightChild|...
	memcpy(data+index+1,value1,attrLength1);//bale to kleidi sthn riza
	data[index] = 2; //points to 3rd b+ tree block (leftchild)
	data[index+attrLength1+1] = 3;//points to 2nd B+ tree block (right child)
	data[1] = 1;//exei ena klidi i riza
	BF_Block_SetDirty(block);//set_dirty root
	BF_UnpinBlock(block);//unpin root

    BF_AllocateBlock(fileDesc,block);//allocate 2nd B+ tree block(left child)(it is empty)1st data block
    BF_GetBlock(fileDesc,2,block);
    data = BF_Block_GetData(block);
    data[0] = 1;
    data[1] = 0;
    data[BF_BLOCK_SIZE-1] = 3;//deixnei sto epomeno data block
    BF_Block_SetDirty(block);
	BF_UnpinBlock(block);

    BF_GetBlock(fileDesc,0,block);
    data = BF_Block_GetData(block);
    data[BF_BLOCK_SIZE-3] = 1;//store root's block num
    BF_Block_SetDirty(block);
	BF_UnpinBlock(block);

	BF_AllocateBlock(fileDesc,block);//allocate 3rd b+ tree block (right child)
	BF_GetBlock(fileDesc,3,block);//3rd file block =  2nd data block
	data = BF_Block_GetData(block);
	data[0] = 1;
	data[1] = 0;
	data[BF_BLOCK_SIZE-1] = -10;//-10 vazoume otan den deixnei pouthena akoma
	//data block : |isDataByte|dataCounter|pedioa1|pedioa2|pediob1|pediob2|...
	memcpy(data+index,value1,attrLength1);//pedio kleidi
	memcpy(data+index+attrLength1,value2,attrLength2);//2o peidio
	data[1] = 1;
	BF_Block_SetDirty(block);
	BF_UnpinBlock(block);
	BF_Block_Destroy(&block);
	return AME_OK;
}

/* Searches in the tree for the block the value should be */
int bPlusSearch(int fileDesc,void* value1,int* destBlockNum,Stack* parentStack){
    /*Skopos afths tis sinartisis einai na epistrepsei stin metavliti destBlockNum
    thn ton arithmo tou block mesa sto opoio prepei na mpei h eggrafh.
    Xrhsimopoioume stoiva gia na kratame to path apo  thn riza mexri to datablock.
    */
	BF_Block* block;
	BF_Block_Init(&block);

    char attrType1 = openFiles[fileDesc].attrType1;
    int attrLength1 = openFiles[fileDesc].attrLength1;

    BF_GetBlock(fileDesc,0,block);
    char* data = BF_Block_GetData(block);
    int numberOfLayers = data[BF_BLOCK_SIZE-2];
	int rootsBlockNumber = data[BF_BLOCK_SIZE-3];
	BF_UnpinBlock(block);

	int nextBlockNum = rootsBlockNumber;
	int index = 2;
	BF_GetBlock(fileDesc,rootsBlockNumber,block);//get root
	data = BF_Block_GetData(block);

    void* key = malloc(attrLength1);
	int positionOfNextChild = -1;
	/*arxikopoioume to positionOfNextChild se -1
	gia tin periptosi pou xriastoume na valoume tin egrafh
	stin pio deksia thesi*/
	int j = 0;
	int k = 0;
    parentStack->top = -1;
	while(data[0] == 0){//psaxnoume mexri na vroume data block(sta data block isxuei: data[0]==1)
		for(j=1;j<=data[1];j++){//search all keys in block
			memcpy(key,data + index + (j-1)*attrLength1 +j,attrLength1); //get key
			if(genericCompare(key,value1,attrType1) == 1){
				printf("Get left child\n");
				positionOfNextChild = index + (j-1)*attrLength1 +j-1;
				printf("Getting block : %d\n",data[positionOfNextChild]);
				break;//bre8hke key>value1
			}
		}
		if(positionOfNextChild == -1){ //if all keys < value1
			positionOfNextChild = index + data[1]*(attrLength1+1);//get right child
			printf("Get right child\n");
			printf("Getting block : %d\n",data[positionOfNextChild]);
		}
        parentStack->top++;
        parentStack->arr[parentStack->top] = nextBlockNum;
		nextBlockNum = data[positionOfNextChild];
		BF_UnpinBlock(block);
		BF_GetBlock(fileDesc,nextBlockNum,block);
		data = BF_Block_GetData(block);
	}
	BF_UnpinBlock(block);
	BF_Block_Destroy(&block);
	*destBlockNum = nextBlockNum;
	free(key);
	return AME_OK;
}

/* Shifts data in block to insert new in order to be sorted */
int shiftingData(char* data,int index,int entryPos,int entryNum,void* value1,int attrLength1,void* value2,int attrLength2){
    /*Einai mia voithitiki sunarthsh pou skopo exei na kanei shift deksia
    kapoia dedomena eite kleidia, wste to neo dedomeno h kleidi antistoixa,
    na mpei sthn swsth thesh mesa sto block gia to opoio proorizeitai.*/
    int sum = attrLength1 + attrLength2;
    void* shiftData = malloc(BF_BLOCK_SIZE);
    //move data to shiftData buffer
    memcpy(shiftData,data+entryPos,(data[1]-entryNum)*sum);
    //shift data
    memcpy(data+index+(entryNum+1)*sum,shiftData,(data[1]-entryNum)*sum);//shifting done
    memcpy(data+index+entryNum*sum,value1,attrLength1);
    memcpy(data+index+entryNum*sum+attrLength1,value2,attrLength2);
    free(shiftData);
    return AME_OK;
}

/* Prints all entries from all blocks */
int printAllEntries(int fileDesc){
    /* H synarthsh ektypwnei oles tis eggrafes arxizontas apo to prwto block dedomenwn
    akolou8wntas tous deiktes sta epomena block. Exei voithiko rolo, kai voithaei sthn
    epalithefsi oti topothetoume ta dedomena swsta(kai se swsth seira) mesa sta blocks
    (kai sta data blocks kai sta index blocks). */
    BF_Block* block;
	BF_Block_Init(&block);
    int attrLength1;
    int attrLength2;
    char attrType1;
    char attrType2;
    char* data;
    int nextBlock;
    int totalEntries=0;

    printf("\nPrinting All Entries for fileDesc: %d\n",fileDesc);

    BF_GetBlock(fileDesc,0,block);//get file's first block
	data = BF_Block_GetData(block);
	if(data[0] != 'A'){
        printf("not b+ tree\n");
		return -1;
	}

	memcpy(&attrType1,data+sizeof(char),sizeof(char));
	memcpy(&attrLength1,data+2*sizeof(char),sizeof(int));
	memcpy(&attrType2,data+2*sizeof(char)+sizeof(int),sizeof(char));
	memcpy(&attrLength2,data+3*sizeof(char)+sizeof(int),sizeof(int));
	BF_UnpinBlock(block);//unpin data_block
	int sum = attrLength1+attrLength2;

	BF_GetBlock(fileDesc,2,block);//arxiko data block
	data = BF_Block_GetData(block);
    void* val1 = malloc(attrLength1);
    void* val2 = malloc(attrLength2);
    while(1){//tha kanei break otan ftasoume to deksiotero fullo
        for(int entryPos=2,entryNum=0;entryNum<data[1];entryPos+=sum,entryNum++){
            memcpy(val1,data +2 + entryNum*sum,attrLength1);
            memcpy(val2,data +2 + entryNum*sum + attrLength1,attrLength2);
            genericPrint(val1,val2,attrType1,attrType2);
            totalEntries++;
        }
		if(data[BF_BLOCK_SIZE-1] == -10)
			break;
        nextBlock = data[BF_BLOCK_SIZE-1];

        BF_UnpinBlock(block);
        printf("\nBlock Number : %d\n",nextBlock);
        BF_GetBlock(fileDesc,nextBlock,block);//next data block
    	data = BF_Block_GetData(block);
    }
    free(val1);
    free(val2);
    printf("TotalEntries : %d\n\n",totalEntries);
    BF_UnpinBlock(block);
    BF_Block_Destroy(&block);
    return AME_OK;
}

int insertDataSorted(int fileDesc,int destBlockNum,void* value1,void* value2){
    /*Rolos afths ths sunarthshs einai na vrei tin katallhlh thesh pou prepei
    na mpei mia eggrafh sto data block gia to opoio proorizetai. Afou vrei h sunarthsh thn
    swsth thesh, kanei ta aparaithta shifts, kai vazei to dedomeno sthn swsth tou thesh.
    Me afth thn sunarthsh petuxenoume tin swsth diataksh twn eggrafwn mesa sto block.
    */
    BF_Block* block;
    BF_Block_Init(&block);
    BF_GetBlock(fileDesc,destBlockNum,block);//next data block
    char* data = BF_Block_GetData(block);

    char attrType1 = openFiles[fileDesc].attrType1;
    int attrLength1 = openFiles[fileDesc].attrLength1;
    char attrType2 = openFiles[fileDesc].attrType2;
    int attrLength2 = openFiles[fileDesc].attrLength2;
    int sum = attrLength1+ attrLength2;
    int index = 2;

	void* curkey = malloc(attrLength1);
	int entryPos;
	int entryNum = 0;

    /*Se afto to loop, psaxnoume thn katallhlh thesh ths neas eggrafhs. Otan tin vrei, kanoume to katallhlo
    shift dedomenwn, wste na kanoume xwro gia na mpei h nea eggrafh sthn thesh pou prepei.*/
	for(int entryPos=index;entryNum<data[1];entryPos+=sum){//entryNum dixnei se pia egrafh vriskomaste
		memcpy(curkey,data+entryPos,attrLength1);
		if(genericCompare(curkey,value1,attrType1) == 1 ){ //1 if value1 < curkey
			//vrethike i thesi opote ksekiname to shift twn epomenwn eggrafwn
			shiftingData(data,index,entryPos,entryNum,value1,attrLength1,value2, attrLength2);
			break;
		}
		entryNum++;
	}
    /*An isxuei afth h sinthiki, simenei pos h eggrafh tha mpei sto telos tou block.*/
	if(entryNum == data[1]){//apla vazoume tin egrafi sto telos
		int deksia = data[1]*(attrLength1+attrLength2);
		memcpy(data+index+deksia,value1,attrLength1);
		memcpy(data+index+deksia+attrLength1,value2,attrLength2);
	}
    free(curkey);
	data[1]++;
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);
    BF_Block_Destroy(&block);
	return AME_OK;
}

int insertIndexSorted(int fileDesc,void* value1,void* newKey,int rightPointer,Stack* parentStack)
{
    /*Skopos afths ths sunarthshs einai paromoios me afton ths insertDataSorted, mono pou edw
    prosthetoume kleidi ,me tous katallhlous deiktes tou deksia aristera, se index blocks. O
    kwdikas edw einai arketa pio sinthetos dioti prepei na kanei set kai tous deiktes tou kleidiou
    kai episis se merikes periptoseis prepei na kaleitai anadromika wste na ulopoieitai swsta
    h leitourgia tou B+ dentrou.
    */
	BF_Block* block;
	BF_Block_Init(&block);

    char attrType1 = openFiles[fileDesc].attrType1;
    int attrLength1 = openFiles[fileDesc].attrLength1;
    int index = 2;

	char* data;
    int destBlockNum = parentStack->arr[parentStack->top];
    parentStack->top--;
    int newBlockNum;
    void* midKey = malloc(attrLength1);
	BF_GetBlock(fileDesc,destBlockNum,block);
	data = BF_Block_GetData(block);
    /*Edw tha mpei efoson to index block pou theloume na prostethei to neo kleidi
    me tous deiktes tou, den exei allo xwro.
    */
    if((data[1]+1) * (attrLength1+1) + 3 > BF_BLOCK_SIZE){
        BF_UnpinBlock(block);
        /*kanoume split to index block, epistrefontas stin metavliti newBlockNum
        ton arithmo tou neou block pou dhmiourgisame.*/
        splitIndexBlock(fileDesc,destBlockNum,&newBlockNum,midKey);
        /*Edw tha mpei ean xreiazetai na mpei an h parapanw splitIndexBlock
        espase thn riza tou B+ dentrou mas.
        */
        if(destBlockNum == parentStack->arr[0]){//spaei h riza
            //kainouria riza k ananewnoume ton ari8mo rizas k bazoume to key
            int newRootNum;
            BF_GetBlockCounter(fileDesc,&newRootNum);
            BF_AllocateBlock(fileDesc,block);
            BF_GetBlock(fileDesc,newRootNum,block);//get new block
            data = BF_Block_GetData(block);
            data[0] = 0;
            data[1] = 1;
            data[2]=destBlockNum;
            /*Pairnoume to prwto kleidi ths neas rizas, etsi wste na kseroume
            se se poio paidi proketai na mpei to neo kleidi mazi me ton deikth tou.*/
            memcpy(data+3,midKey,attrLength1);
            data[3+attrLength1] = newBlockNum;

            BF_Block_SetDirty(block);
            BF_UnpinBlock(block);

            BF_GetBlock(fileDesc,0,block);
            data = BF_Block_GetData(block);
            data[BF_BLOCK_SIZE-3] = newRootNum;
            BF_Block_SetDirty(block);
            BF_UnpinBlock(block);

            if(genericCompare(newKey,midKey,attrType1) == 1)//newkey > midkey deksi block
                BF_GetBlock(fileDesc,newBlockNum,block);
            else
                BF_GetBlock(fileDesc,destBlockNum,block);
            data = BF_Block_GetData(block);

        }
        /*Edw tha mpei an to index block pou spasame, DEN einai h riza tou dentrou mas.*/
        else{
            insertIndexSorted(fileDesc,value1,midKey,newBlockNum,parentStack);//me newkey to midkey
        }
    }
	void* curkey = malloc(attrLength1);
	int entryPos;
	int entryNum = 0;
    void* shiftData = malloc(BF_BLOCK_SIZE);
    /*Auto to loop einai paromoio me afto pou exoume sthn insertDataSorted. Skopos tou einai
    na vrei thn thesh pou prepei na mpei to neo kleidi me to deksi paidi tou. Otan vrei thn thesh
    tote to topothetei kiolas.*/
	for(int entryPos=index+1;entryNum<data[1];entryPos+=1+attrLength1){
		memcpy(curkey,data+entryPos,attrLength1);
		//entryNum leei poses egrafes exoume kseperasei
		if(genericCompare(curkey,value1,attrType1) == 1 ){ //1 if value1 < curkey
			//vrethike i thesi opote ksekiname to shift twn epomenwn kleidiwn
			//move data to shiftData buffer
			memcpy(shiftData,data+entryPos,(data[1]-entryNum)*(attrLength1+1));//pernoume kai to klidi mazi //to +1 gia to telefteo i
			memcpy(data+entryPos+attrLength1+1,shiftData,(data[1]-entryNum)*(attrLength1+1));//shifting done
			memcpy(data+entryPos,newKey,attrLength1);//eisagogi tou neou key
            data[entryPos+attrLength1] = rightPointer;
			break;
		}
		entryNum++;
	}
	if(entryNum == data[1]){//apla vazoume to kleidi me to paidi tou sto telos tou index block.
		int deksia = data[1]*(attrLength1+1);
		memcpy(data+index+1+deksia,newKey,attrLength1);
        data[index+1+deksia+attrLength1] = rightPointer;
	}
    data[1]++;
    free(shiftData);
    free(midKey);
    free(curkey);
	BF_Block_SetDirty(block);
	BF_UnpinBlock(block);
	BF_Block_Destroy(&block);
	return AME_OK;
}

int splitDataBlock(int fileDesc,int destBlockNum,int* newBlockNum)//gia na epistrepsoume ton arithmo tou neou block)
{
    /*O rolos afths ths sunarthshs einai na parei to block pou einai gemato (destBlockNum),
    na dhmiourghsei ena neo data block, na isomoirasei ta dedomena sta 2 blocks kai na epistrepsei
    sthn metavlhth newBlockNum ton arithmo tou neou block pou dhmiourghse. Yparxoun arketes
    idiaiteres periptwseis otan  orismena kleidia h ola enos data block einai idia, tis  opoies
    exoume parei upopsin mas ulopoiontas katallhlo kwdika pou lunei tis idiaiterothtes
    aftes sto spasimo enos block.
    */
    BF_Block* block;
    BF_Block_Init(&block);
    BF_GetBlock(fileDesc,destBlockNum,block);//next data block
    char* data = BF_Block_GetData(block);

    char attrType1 = openFiles[fileDesc].attrType1;
    int attrLength1 = openFiles[fileDesc].attrLength1;
    int attrLength2 = openFiles[fileDesc].attrLength2;
	int sum = attrLength1+attrLength2;
    int index = 2;
    int secondHalfDataNumber = data[1]/2;//proorizetai gia to data[1] tou neou block
    int sizeOfMalloc = secondHalfDataNumber*sum; //xreiazetai gia tin malloc

    //pernoume to katw fragma, giati i nea egrafh tha mpei sto neo block opote tha einai balanced
    int middlePosition = index+(data[1]/2)*sum+(data[1]%2)*sum;
    int breakingPosition = middlePosition;//by default spaei sti mesh
    int found = 0;
    int entryPos;
    int entryNum = 0;
    void* midEntry = malloc(attrLength1);
    void* currentKey = malloc(attrLength1);

    memcpy(midEntry,data+middlePosition,attrLength1);

    for(int entryPos= middlePosition;entryPos>=index;entryPos-=sum){//start from mid position
        memcpy(currentKey,data+entryPos,attrLength1);
        if(genericCompare(currentKey,midEntry,attrType1) != 0){//diafora tou midenos
            breakingPosition = entryPos+sum;
            found = 1;
            secondHalfDataNumber = data[1]- breakingPosition/sum;
            break;
        }
    }
    if(found == 0){//an apo ta aristera ola ta kleidia einai idia
        for(int entryPos=middlePosition;entryPos<BF_BLOCK_SIZE;entryPos+=sum){//start from mid position
            memcpy(currentKey,data+entryPos,attrLength1);
            if(genericCompare(currentKey,midEntry,attrType1) != 0){//diafora tou midenos
                breakingPosition = entryPos;
                secondHalfDataNumber = data[1]-entryPos/sum;
                found = 1;
                break;
            }
        }
    }
    sizeOfMalloc = BF_BLOCK_SIZE - breakingPosition;
    void* rightHalfishData = malloc(sizeOfMalloc);

    memcpy(rightHalfishData,data+breakingPosition,sizeOfMalloc);//pernei kai to telefteo
    /************************************************************************************************************/
    //sbinoume ta misa h mporoume na ellatwsoume ton ari8mo twn eggrafwn sto 2o byte wste oi mises na nai "skoupidia"
    data[1] -= secondHalfDataNumber;
    BF_GetBlockCounter(fileDesc,newBlockNum);
    int tempNextBlockNumber = data[BF_BLOCK_SIZE-1];
    data[BF_BLOCK_SIZE-1] = *newBlockNum;//deixnei sto neo block pou ftiaxnoume
    BF_Block_SetDirty(block);//afto pou pirame stin arxh
    BF_UnpinBlock(block);
    //==================================================================twra pame sto neo block
    BF_AllocateBlock(fileDesc,block);
    BF_GetBlock(fileDesc,*newBlockNum,block);//get new block
    data = BF_Block_GetData(block);
    data[0] = 1;
    data[1] = secondHalfDataNumber;
    memcpy(data+index,rightHalfishData,sizeOfMalloc);//o deikths to epomenou exei hdh metafer8e
    data[BF_BLOCK_SIZE-1] = tempNextBlockNumber;
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);
    free(midEntry);
    free(currentKey);
    free(rightHalfishData);
    BF_Block_Destroy(&block);
    return AME_OK;
}

//newBlockNum: gia na epistrepsoume ton arithmo tou neou block
int splitIndexBlock(int fileDesc,int destBlockNum,int* newBlockNum,void* midKey){
    /*H sunarthsh afth xrisimopoieitai gia na spasei ena gemato index block kai
    na isomoirazei ta kleidia kai ta antistoixa paidia tous sta 2 blocks. H leitourgia ths
    einai paromoia me tis splitDataBlock, mono einai arketa pio aplh apo afthn, dioti
    sto spasimo index block, oi idiaiteres periptoseis einai polu pio liges apo aftes
    sto spasimo enos data block, mias kai ta kleidia enos index block einai sigoura
    ola diaforetika metaksu tous.
    */
    BF_Block* block;
	BF_Block_Init(&block);
    BF_GetBlock(fileDesc,destBlockNum,block);
    char* data = BF_Block_GetData(block);

    char attrType1 = openFiles[fileDesc].attrType1;
    int attrLength1 = openFiles[fileDesc].attrLength1;
	int sum = attrLength1+1;

    int index = 2;
    int secondHalfDataNumber;
    if(data[1]%2 == 1)
        secondHalfDataNumber = data[1]/2;
    else if(data[1]%2 == 0)
        secondHalfDataNumber = data[1]/2 - 1;
    int sizeOfMalloc = 1+secondHalfDataNumber*sum; //xreiazetai gia tin malloc
    //pernoume to katw fragma, giati i nea egrafh tha mpei sto neo block opote tha einai balanced af
    int breakingPosition = index+1+(data[1]/2)*sum;//by default spaei sti mesh
    void* rightHalfishData = malloc(sizeOfMalloc);
    memcpy(midKey,data+breakingPosition,attrLength1);
    memcpy(rightHalfishData,data+breakingPosition+attrLength1,sizeOfMalloc);//pernei kai to telefteo
    /************************************************************************************************************/
    //sbinoume ta misa h mporoume na ellatwsoume ton ari8mo twn eggrafwn sto 2o byte wste oi mises na nai "skoupidia"
    data[1] = data[1] - secondHalfDataNumber-1;
    BF_GetBlockCounter(fileDesc,newBlockNum);
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);
    //==================================================================twra pame sto neo block
    BF_AllocateBlock(fileDesc,block);
    BF_GetBlock(fileDesc,*newBlockNum,block);//get new block
    data = BF_Block_GetData(block);
    data[0] = 0;
    data[1] = secondHalfDataNumber;
    memcpy(data+index,rightHalfishData,sizeOfMalloc);//o deikths to epomenou exei hdh metafer8e
    BF_Block_SetDirty(block);
    BF_UnpinBlock(block);

    free(rightHalfishData);
    return AME_OK;
}
