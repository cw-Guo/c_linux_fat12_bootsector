#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;//1字节
typedef unsigned short u16;//2字节
typedef unsigned int u32; //4字节

int  BytsPerSec;	//每扇区字节数
int  SecPerClus;	//每簇扇区数
int  RsvdSecCnt;	//Boot记录占用的扇区数
int  NumFATs;	//FAT表个数
int  RootEntCnt;	//根目录最大文件数
int  FATSz;	//FAT扇区数

int TotSec;//扇区数总数

#pragma pack (1) /*指定按1字节对齐*/

		   //偏移11个字节
struct BPB {
	u16  BPB_BytsPerSec;	//每扇区字节数
	u8   BPB_SecPerClus;	//每簇扇区数
	u16  BPB_RsvdSecCnt;	//Boot记录占用的扇区数
	u8   BPB_NumFATs;	//FAT表个数
	u16  BPB_RootEntCnt;	//根目录最大文件数
	u16  BPB_TotSec16;
	u8   BPB_Media;
	u16  BPB_FATSz16;	//FAT扇区数
	u16  BPB_SecPerTrk;
	u16  BPB_NumHeads;
	u32  BPB_HiddSec;
	u32  BPB_TotSec32;	//如果BPB_FATSz16为0，该值为FAT扇区数
};
//BPB至此结束，长度25字节  
//结构参考百度词条 FAT12

//根目录条目
struct RootEntry {
	char DIR_Name[11];
	u8   DIR_Attr;		//文件属性
	char reserved[10];
	u16  DIR_WrtTime;
	u16  DIR_WrtDate;
	u16  DIR_FstClus;	//开始簇号
	u32  DIR_FileSize;
};
//根目录条目结束，32字节
int FstClus; //开始簇号
int FileSize;//文件大小
int NextCluser;

struct Name { // 文件名结构体
	char name[8];
	char ext[3];
};

void fillBPB(FILE * fat12, struct BPB* bpb_ptr);	//载入BPB
void printFiles(FILE * fat12, struct RootEntry* rootEntry_ptr);	//打印文件名，这个函数在打印目录时会调用下面的printChildren
//void printChildren(FILE * fat12, char * directory, int startClus);	//打印目录及目录下子文件名
int  getFATValue(FILE * fat12, int num);	//读取num号FAT项所在的两个字节，并从这两个连续字节中取出FAT项的值，
void Outputfile(FILE *fat12, int FstClus, int FileSize, struct Name* fileName_ptr); //输出img内的文件

int main()
{
	FILE *fat12;
	fat12 = fopen("fat12.img", "rb");

	struct BPB bpb;
	struct BPB* bpb_ptr = &bpb;

	fillBPB(fat12, bpb_ptr); //读出引导区的结构

	BytsPerSec = bpb_ptr->BPB_BytsPerSec; 
	SecPerClus = bpb_ptr->BPB_SecPerClus;
	RsvdSecCnt = bpb_ptr->BPB_RsvdSecCnt;
	NumFATs = bpb_ptr->BPB_NumFATs;
	RootEntCnt = bpb_ptr->BPB_RootEntCnt;

	FATSz = bpb_ptr->BPB_FATSz16;


	printf("每扇区字节数:%d\n", BytsPerSec);
	printf("每簇扇区数:%d\n", SecPerClus);
	printf("Boot记录占用扇区数:%d\n", RsvdSecCnt);
	printf("共有FAT表数:%d\n", NumFATs);
	printf("根目录文件数最大值:%d\n", RootEntCnt);
	printf("一个FAT占用扇区数:%d\n", FATSz);

	struct RootEntry rootEntry; //根目录
	struct RootEntry* rootEntry_ptr = &rootEntry;

	printFiles(fat12, rootEntry_ptr);

	//system("pause");
	return 0;
}


void fillBPB(FILE* fat12, struct BPB* bpb_ptr) {
	int index;
	index = fseek(fat12, 11, SEEK_SET);
	//如果执行成功，stream将指向以SEEK_SET为基准，
	//偏移offset（指针偏移量）个字节的位置，函数返回0。
	//如果执行失败，则不改变stream指向的位置，函数返回一个非0值。

	if (-1 == index)
	{
		printf("fseek in fillBPB failed!\n");
	}

	index = fread(bpb_ptr, 1, 25, fat12);

	//size_t fread( void *buffer, size_t size, size_t count, FILE *stream )
	//buffer	指向要读取的数组中首个对象的指针
	//size	每个对象的大小（单位是字节）
	//count	要读取的对象个数
	//stream输入流

	if (index != 25) {
		printf("fread in fillBPB failed!");
	}
}

void printFiles(FILE *fat12, struct RootEntry* rootEntry_ptr)
{
	int base = (RsvdSecCnt + NumFATs * FATSz)* BytsPerSec + 32;//目录区的偏移地址字节数目
	int index;
	struct Name fileName;
	struct Name* fileName_ptr = &fileName;//用于存储文件名；

										  //依次处理各文件
	int i;
	for (i = 0; i < RootEntCnt; i++)
	{
		index = fseek(fat12, base, SEEK_SET);//接下来是目录区
											 //按照规则读取相应数据。
		if (-1 == index)
		{
			printf("fseek in printFiles failed!\n");
		}
		index = fread(rootEntry_ptr, 1, 32, fat12); //读根目录区
		if (32 != index)
		{
			printf("fseek in printFiles failed!\n");
		}

		base += 32;//第0个dir读取完毕

		if (rootEntry_ptr->DIR_Name[0] == '\0') continue;

		//接下来读取文件名

		int j = 0;
		int boolval = 0;
		for (j = 0; j < 11; ++j) {//Dos 文件名 编码标准请查 fat12 维基百科
			if (!(((rootEntry_ptr->DIR_Name[j] >= 48) && (rootEntry_ptr->DIR_Name[j] <= 57)) ||
				((rootEntry_ptr->DIR_Name[j] >= 65) && (rootEntry_ptr->DIR_Name[j] <= 90)) ||
				((rootEntry_ptr->DIR_Name[j] >= 97) && (rootEntry_ptr->DIR_Name[j] <= 90)) ||
				(rootEntry_ptr->DIR_Name[j] == ' ') || (rootEntry_ptr->DIR_Name[j] == 33) ||
				(rootEntry_ptr->DIR_Name[j] == 35) ||
				(rootEntry_ptr->DIR_Name[j] == 36) ||
				(rootEntry_ptr->DIR_Name[j] == 37) ||
				(rootEntry_ptr->DIR_Name[j] == 38) ||
				(rootEntry_ptr->DIR_Name[j] == 40) ||
				(rootEntry_ptr->DIR_Name[j] == 41) ||
				(rootEntry_ptr->DIR_Name[j] == 45) ||
				(rootEntry_ptr->DIR_Name[j] == 64) ||
				(rootEntry_ptr->DIR_Name[j] == 94) ||
				(rootEntry_ptr->DIR_Name[j] == 96) ||
				(rootEntry_ptr->DIR_Name[j] == 123) ||
				(rootEntry_ptr->DIR_Name[j] == 125) ||
				(rootEntry_ptr->DIR_Name[j] == 126) ||
				(rootEntry_ptr->DIR_Name[j] == 125)))
			{
				boolval = 1;
				break;
			}
		}

		if (boolval == 1) continue;//排除填充的字符

		int k;
		int tmp = 0;
		if ((rootEntry_ptr->DIR_Attr & 0x10) == 0)//不是子目录
		{
			for (k = 0; k < 8; ++k) { //读取文件名
				if (rootEntry_ptr->DIR_Name[k] != ' ') {

					fileName_ptr->name[tmp] = rootEntry_ptr->DIR_Name[k];
					tmp++;
				}
			}
			fileName_ptr->name[tmp] = '\0';
			tmp = 0;
			for (k = 8; k < 11; ++k) {//读取拓展名
				if (rootEntry_ptr->DIR_Name[k] != ' ') {

					fileName_ptr->ext[tmp] = rootEntry_ptr->DIR_Name[k];
					tmp++;
				}
			}
			if (tmp < 3)
			{
				fileName_ptr->name[tmp] = '\0';
			}

			printf("%s.%s\n", fileName_ptr->name, fileName_ptr->ext);

			FstClus = rootEntry_ptr->DIR_FstClus;
			FileSize = rootEntry_ptr->DIR_FileSize;
		//	printf("the first Clus = %d", FstClus);//测试用
		//	printf("the filesize = %d", FileSize);//测试用
			Outputfile(fat12, FstClus, FileSize, fileName_ptr);
		}
		else {//子目录//涉及嵌套目录，暂不实现
			break;
		}
	}
}

int  getFATValue(FILE * fat12, int num) //查FAT表
{
	//FAT1的偏移字节
	int fatBase = RsvdSecCnt * BytsPerSec;
	//fat 簇号12位
	int fatPos = fatBase + (num * 3 )/ 2;

	int type = 0;

	if (num % 2 == 0) {
		//偶数簇
		type = 0;
	}
	else {//奇数簇
		type = 1;
	}

	//fat 默认保留两簇
	u16 bytes;
	//u16* bytes_ptr =NULL;

	int check;
	check = fseek(fat12, fatPos, SEEK_SET);
	if (check == -1) {
		printf("fseek in getFATValue failed!");
	}

	check = fread(&bytes, 1, 2, fat12);

	if (check != 2) {
		printf("fread in getFATValue failed!");
	}

	//u16为short，结合存储的小尾顺序和FAT项结构可以得到
	if (type == 0) {
		bytes = (bytes & 0x0fff);
		return bytes;
	}
	else {
		bytes = (bytes  >> 4);
		//printf("%d", bytes);
		return bytes;
	}

}

//从内存中输出文件

void Outputfile(FILE *fat12, int FstClus, int FileSize, struct Name* fileName_ptr) {

	/******整理文件名*****/
	char name[24];
	int t = 0;
	int tmp = 0;
	for (t = 0; t < 8; ++t)
	{
		if (fileName_ptr->name[t] != '\0')
		{
			name[tmp] = fileName_ptr->name[t];
			tmp++;
		}
		else { break; }
	}
	name[tmp] = '.';
	tmp++;
	t = 0;
	for (t = 0; t < 3; ++t)
	{
		if (fileName_ptr->ext[t] != '\0')
		{
			name[tmp] = fileName_ptr->ext[t];
			tmp++;
		}
		else { break; }
	}
	name[tmp] = '\0';

	/******整理文件名*****/

	//分配文件存储空间。
	FILE *fp = NULL;
	fp = fopen(name, "a+");

	if (NULL == fp) {
		printf("write failed!\n");
	}


	int i = 0;
	int max =0;
	max =  FileSize / 512 / 4 + 2; //以控制读取文件大小
	printf("max:%d\n", max);
	for (i = 0; i < max; i++)
	{
		int base1;
		base1 = BytsPerSec * (RsvdSecCnt + NumFATs * FATSz) + 32 * RootEntCnt + (FstClus - 2)*SecPerClus*BytsPerSec; //data区域 cluster偏移地址
		if(i==0) printf("%#x\n", base1);

		

		int j =0;

		char *buffer = (char *)malloc(BytsPerSec*SecPerClus);
		int index = 0;

		index = fseek(fat12, base1, SEEK_SET);//按照规则读取相应数据。
		if (-1 == index)
		{
			printf("fseek in outputFiles failed!\n");
		}
	
		index = fread(buffer, 1, (4*512), fat12);
		if ((BytsPerSec*SecPerClus) != index)
		{
			printf("fread in outputFiles failed!\n");
		}
	

		//memcpy(buffer,fat12,4*512);
		fwrite(buffer, 512*4, 1, fp);
		free(buffer);
		buffer = NULL;

		FstClus = getFATValue(fat12, FstClus);//查表，如果是末尾cluster，则跳出循环，不是则更新下一个簇号
		if (FstClus >= 0x0ff8) { printf("i:%d", i);break;  }
		if (FstClus == 0x0ff7) { printf("坏簇!\n"); break; }
	}
	//printf("%d", FileSize);
	fclose(fp);
	fp = NULL;
}


