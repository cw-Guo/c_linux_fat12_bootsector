# c_fat12_bootsector
using c/linux to process a fat12 img 用c语言实现对fat12文件系统的读取
### 一个FAT12 格式的引导程序
##### Chengwei Guo

首先把文档读出来

    vim -b fat12.img
    :%!xxd
 这样可以在vim中读出其2进制表示


----------


FAT12 文件系统将 逻辑盘的空间划分为四个个部分，

·  引导区（$Boot$区）、
·  文件分配表区（$FAT$区）、
·  根目录区（$dir$区）
·  数据区（$data$区）

其中，根目录区域和数据区域都不是固定长度的。


----------
##### 引导区

引导区中，读取其2 进制表示
前三个字节分别是 EB 3C 90  
0xEB是跳转指令，0x58是跳转的地址，而0x90则是空指令
之后8个字节则是厂商名 

从第11个字节的数据叫做 BPB（BIOS Paramter Block）
BPB 的结构可以通过查阅资料得到。
我们通过读取BPB的内容可以得知一些关键信息。

比如在我们实践的这个fat12.img的文件中，

>每扇区字节数:512

>每簇扇区数:4

>Boot记录占用多少扇区:1

>共有多少FAT表: 2

>根目录文件数最大值:512

>一个FAT占用扇区数:3

我们可以知道 boot记录占用了1个扇区，那么也就是$512B$大小。

紧接着是两个FAT表，这两个FAT表是完全相同的， 每个FAT表占用3个扇区

那么我们知道根目录的起始地址是 $512B+2\times 3 \times512B =11\times 512B$


----------


##### 根目录区
该fat.img 根目录区 的编码不是标准编码   多了两行（16*2= 32字节）补充信息
通过添加一些限制我们可以读出根目录区的结构。
> 文件名

> 拓展名

> 对应开始簇号

> 文件大小

通过对应开始簇号，我们需要去fat表处查找。


----------


##### FAT表
fat表位于引导区之后，有两个完全一样的fat表。通过bpb的信息我们知道偏移位置为：512B

fat12中,每个簇号有12位构成（文件大小限制之一）。
每个簇号查找时，找到其偏移位置的两个字节。
由于是小端序，所以我们应该区分是奇数号簇还是偶数号簇，从而对簇号进行读取。

簇号含义：如果簇号大于等于$0x0ff8$ 那么则代表该簇是文件的结束簇,

如果等于$0x0ff7$代表该簇损坏，其余情况都是指下一簇。

其中fat表簇号从2开始，保留了两簇（0、1簇）

----------


##### data区
data 区域在上述所有区域之后，所以计算其偏移时，应如下计算

	base1 = BytsPerSec * (RsvdSecCnt + NumFATs * FATSz) + 32 * RootEntCnt + (FstClus - 2)*SecPerClus*BytsPerSec; 
	//data区域 cluster偏移地址

结合fat表找到一个文件对应的所有簇号并找到所有簇号对应的data空间即可以对齐进行读取。

#### 已知bug 
- 在windows 系统下使用时，输出的pdf文件打开是乱码。在linux下可以正常显示
- txt文件没有文件终止符。


### readme.md is to be finished.
