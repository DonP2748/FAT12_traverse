//////////////////////////////////////////////////////////////////////
// All rights reserved. This program and the accompanying materials	//
// are made available under the terms of the Public License v1.0	//
// and Distribution License v1.0 which accompany this distribution.	//
// MODULE  	:	parsor.c										    //
// AUTHOR 	: 	DonP - donp172748@gmail.com							//
// DATE   	: 	--/--/----											//
//////////////////////////////////////////////////////////////////////

#pragma pack (1)
//--------------INCLUDE------------------//
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "parsor.h"
//---------------------------------------//

//---------------GLOBAL------------------//
//General
#define ENTRY_SIZE  	(32)
#define CLUSTER_OFFSET  (2)
#define SIGNATURE 		0xAA55
#define END_CLUSTER 	0x0FFF
#define ROOT_ENTRY_MAX 	(fat->root_entries)


//address in bytes
#define BLOCK_SIZE 	(fat->blk_bytes)
#define ADDR_BOOT 	(0)
#define ADDR_FAT 	(BLOCK_SIZE*fat->reserved_blk)
#define ADDR_ROOT 	(ADDR_FAT + fat->fat_numb*fat->fat_blks*BLOCK_SIZE)
#define ADDR_DATA 	(ADDR_ROOT + fat->root_entries*ENTRY_SIZE)





#define GET_BPB_DATA_AT(f,a,b,d) \
{ \
	fseek(f,a,SEEK_SET);\
	fgets((char*)b,sizeof(b),f);\
	d = reverse((uint8_t*)&b,sizeof(d));\
}

typedef struct _entry_t{
	uint32_t id; 		//index in dir
	uint16_t cluster; 	//cluster number entry belong
	uint8_t  type;     	//file or LFN 
	uint8_t  root;   	//store at root ?  
}_entry_t;



typedef struct BPB_t{
	uint16_t blk_bytes;
	uint8_t  unit_blk;
	uint16_t reserved_blk;
	uint8_t fat_numb;
	uint16_t root_entries;
	uint16_t total_blk_16;
	uint16_t fat_blks;
	uint32_t total_blk_32;
}BPB_t;


//---------------------------------------//

//--------------PRIVATE------------------//
static uint8_t get_bpb_info_from_file(BPB_t* arg,FILE* fd);
static uint32_t reverse(uint8_t* data,uint8_t len);
static uint16_t fat12_cluster_tracing(uint16_t start);
static uint32_t process_entries_in_dir(uint32_t dir_addr);
static uint8_t check_zero(uint8_t* data,uint8_t len);
static uint16_t sfn_entry_parsing(entry_t* entry,uint8_t* data);
static uint16_t lfn_entry_parsing(entry_t* entry,uint8_t* data);
static void clear_entry_table(void);
static void process_block_data_callback(uint8_t* data,uint16_t len);
static void access_file(uint16_t cluster,uint32_t fsize);
static void access_dir(uint16_t cluster);

static BPB_t* fat = NULL;
static FILE* _file = NULL;
static entry_t* ehandle = NULL;
static _entry_t* entries = NULL;
static uint32_t current_dir = 0;
static uint32_t entries_in_dir = 0;
//---------------------------------------//

uint8_t ps_fat_file_parsing_init(char* name)
{
	_file = fopen(name,"r");
	if(!_file) goto failed;

	fat = (BPB_t*)calloc(1,sizeof(BPB_t));
	if(!fat) goto failed;

	ehandle = (entry_t*)calloc(1,sizeof(entry_t));
	if(!ehandle) goto failed;

	if(!get_bpb_info_from_file(fat,_file)) goto failed;

	current_dir = ADDR_ROOT;
	entries_in_dir = process_entries_in_dir(current_dir);
	return true;

failed:
	ps_fat_file_release();
	return false;
}

void ps_fat_file_release(void)
{
	if(ehandle){
		free(ehandle);
		ehandle = NULL;
	}
	if(fat){
		free(fat);
		fat = NULL;
	}
	if(_file){
		fclose(_file);
		_file = NULL;
	}
}

uint32_t ps_get_all_entries_number(void)
{
	return entries_in_dir;
}

uint32_t ps_get_sfn_entries_number(void)
{
	uint32_t ret = 0;
	for(uint32_t i = 0; i < entries_in_dir;i++){	
		if(!entries[i].type) ret++;
	}
	return ret;
}


entry_t* ps_get_entry_data_by_id(uint32_t id)
{
	uint8_t buff[ENTRY_SIZE] = {0};
	//find entry in fat
	uint32_t addr = 0;
	//check in root or not first
	if(entries[id].root){
		addr = ADDR_ROOT + (entries[id].cluster)*BLOCK_SIZE + id*ENTRY_SIZE;
	}
	else{
		addr = ADDR_DATA + (entries[id].cluster-CLUSTER_OFFSET)*BLOCK_SIZE + id*ENTRY_SIZE;
	} 
	fseek(_file,addr,SEEK_SET);
	fread(buff,1,ENTRY_SIZE,_file);
	ehandle->type = entries[id].type;
	if(ehandle->type) ehandle->name_len = lfn_entry_parsing(ehandle,buff);
	else ehandle->name_len = sfn_entry_parsing(ehandle,buff);	
	return ehandle;
}


void ps_access_entry_by_id(uint32_t id)
{
	entry_t* entry = ps_get_entry_data_by_id(id);
	if(entry->type) return; //return if LFN, this api just use for file - SFN
	//check file or folder
	if(entry->data.sname.attr&ATTR_SUBDIR) access_dir(entry->data.sname.cluster);
	else access_file(entry->data.sname.cluster,entry->data.sname.size); 	
}

static void access_dir(uint16_t cluster)
{
	//cluster 0 is root, so check if arg is cluster 0
	current_dir =  (cluster) ? (ADDR_DATA + (cluster-CLUSTER_OFFSET)*BLOCK_SIZE) : (ADDR_ROOT);
	//update new dir entries table
	entries_in_dir = process_entries_in_dir(current_dir);
}

static void access_file(uint16_t cluster,uint32_t fsize)
{
	//file data cannot storage anywhere else except data region 
	uint32_t addr = 0;
	uint8_t* buff = (uint8_t*)calloc(BLOCK_SIZE,1); 
	if(!buff) return;

	//process data
	uint16_t next = cluster;
	uint32_t size = fsize;
	uint32_t offset = 0; //0 reach to fsize
	while((next != END_CLUSTER)&&(size>0)){
		//determine addr and offset, goto addr and get offset data
		addr = (ADDR_DATA + (next-CLUSTER_OFFSET)*BLOCK_SIZE);
		offset = (size > BLOCK_SIZE) ? BLOCK_SIZE : size;
		fseek(_file,addr,SEEK_SET);
		fread(buff,1,offset,_file);
		//do something with data
		process_block_data_callback(buff,offset);
		//end loop condition
		size-= offset;
		next = fat12_cluster_tracing(next);
//		printf("%d next cluster %02X\n",size, next); 
	}
	free(buff);
}

static void process_block_data_callback(uint8_t* data,uint16_t len)
{
	for (int i = 0; i < len; i++){
		printf("%c",data[i]);
	}
	//should not add new line :(
	printf("\n"); 
}


static uint16_t lfn_entry_parsing(entry_t* entry,uint8_t* data)
{
	uint8_t i = 1;
	uint8_t j = 1;
	uint16_t len = 0;
	//number of lfn 
	entry->data.lname[0] = data[0];
	//
	while(i < ENTRY_SIZE){
		if(((i>0)&&(i<11))||((i>13)&&(i<26))||((i>27)&&(i<32))){
			//Character in LFN is 16 bit ascii and reverse
			uint16_t temp = data[i]|(data[i+1]<<8);
			if((temp != 0xFFFF)&&(temp != 0x0000)){
				entry->data.lname[j] = data[i+1]; 
				entry->data.lname[j+1] = data[i];
				len+=2;
			}
			j+=2;
		}
		else{
			i++;
			continue;
		}
		i+=2;
	}
	return len;
}

static uint16_t sfn_entry_parsing(entry_t* entry,uint8_t* data)
{
	uint16_t temp = 0;
	memcpy(entry->data.sname.name,data,8);
	memcpy(entry->data.sname.ext,data+0x08,3);
	entry->data.sname.size = reverse(data+0x1C,4);
	entry->data.sname.cluster = reverse(data+0x1A,2);
	entry->data.sname.attr = data[0x0B];
	temp = reverse(data+0x16,2) ; //   reverse(data+0x16,2)  data[0x17]|(data[0x16]<<8)
	entry->data.sname.time.second = (0x001F&temp)*2;
	entry->data.sname.time.min = (0x07E0&temp)>>5;
	entry->data.sname.time.hour = (0xF800&temp)>>11;
	temp = reverse(data+0x18,2);
	entry->data.sname.time.day = (0x001F&temp);
	entry->data.sname.time.date = (0);
	entry->data.sname.time.month = (0x01E0&temp)>>5;
	entry->data.sname.time.year = ((0xFE00&temp)>>9) + 1980;
	return 8;
}


//create entries table and count the entries number, for root and non-root dir 
static uint32_t process_entries_in_dir(uint32_t dir_addr)
{

	uint8_t  root = (dir_addr == ADDR_ROOT);
	uint32_t count = 0;
	uint32_t start = (dir_addr - ADDR_DATA)/BLOCK_SIZE;
	uint16_t cluster =  start + CLUSTER_OFFSET;
	uint16_t cluster_numb = 0; //cluster chain length

	clear_entry_table();
	entries = (_entry_t*)calloc(BLOCK_SIZE/ENTRY_SIZE,sizeof(_entry_t));
	if(!entries) return 0;
	//loop for cluster, implement entries loop in cluster loop
	uint16_t offset = 0;
	//check sequently cluster for root dir and trace next cluster for others dir 
	while((root&&(offset < ROOT_ENTRY_MAX))||((!root)&&(cluster != END_CLUSTER))){
		//alloc more size if use more than one cluster entries
		if(cluster_numb){
			entries = (_entry_t*)realloc(entries,sizeof(_entry_t)*((BLOCK_SIZE/ENTRY_SIZE)*(1+cluster_numb))); 
		}
		for (uint8_t id = offset; id < BLOCK_SIZE/ENTRY_SIZE; id++){
			//get fisrt 12 bytes to check if entry exist or not, btw check LFN or SFN too
			uint8_t buff[13] = {0};
			fseek(_file,(root) ? (ADDR_ROOT+id*ENTRY_SIZE) : (ADDR_DATA+(cluster-CLUSTER_OFFSET)*BLOCK_SIZE)+(id-offset)*ENTRY_SIZE,SEEK_SET);
			fgets((char*)buff,sizeof(buff),_file);

			//remove file attr and NULL so minus 2
			if(!check_zero(buff,sizeof(buff)-2)){
				entries[id].id = id;
				entries[id].cluster = (root) ? cluster_numb : cluster; //assume that root region has its own cluster, up to 512 entries ~ 32 cluster
				entries[id].type = (buff[11] != 0x0F) ? SFN_ENTRY : LFN_ENTRY;
				entries[id].root = root;
				count++;
			}
		}
		cluster_numb++; //number of cluster are used increase
		//update entries table's offset,for root here is 224 for root 0-223 so it's smaller than ROOT_ENTRY_MAX which is 00E0 ~ 224
		//for others dir, it's the next cluster in chain 0-31,32-63,...
		offset = cluster_numb*(BLOCK_SIZE/ENTRY_SIZE); 
		if(!root) cluster = fat12_cluster_tracing(cluster);
	}
	return count;
}

static void clear_entry_table(void)
{
	if(entries){
		free(entries);
		entries = NULL;
	}
}

static uint16_t fat12_cluster_tracing(uint16_t start)
{
	uint8_t buffer[4] = {0};
	uint32_t addr = ADDR_FAT + ((start%2 == 0) ? (start*1.5f) : (start*1.5f-1));
	fseek(_file,addr,SEEK_SET);
	fgets((char*)buffer,sizeof(buffer),_file);
	uint16_t first_entry = (uint8_t)(buffer[1]&0x0f)*16*16+buffer[0]; /* get the first entry */
	uint16_t second_entry = (buffer[2]*16+(buffer[1]>>4)); /* get the seccond entry */

	if(start%2 == 0){
	 	return first_entry;
	}
	else return second_entry;
}




static uint8_t check_zero(uint8_t* data,uint8_t len)
{
	uint8_t ret = true;
	for(uint8_t i = 0;i < len;i++){
		if(data[i] != 0x00){
			ret = false;
			break;
		}
	}
	return ret;
}


static uint8_t get_bpb_info_from_file(BPB_t* arg,FILE* fd)
{
	uint8_t buff[5];
	uint16_t sign = 0;
	GET_BPB_DATA_AT(fd,0x0b,buff,BLOCK_SIZE);
	//check sign
	GET_BPB_DATA_AT(fd,BLOCK_SIZE-2,buff,sign);
	if(sign != SIGNATURE) return false;
	//continue
	GET_BPB_DATA_AT(fd,0x0d,buff,arg->unit_blk);
	GET_BPB_DATA_AT(fd,0x0e,buff,arg->reserved_blk);
	GET_BPB_DATA_AT(fd,0x10,buff,arg->fat_numb);
	GET_BPB_DATA_AT(fd,0x11,buff,arg->root_entries);
	GET_BPB_DATA_AT(fd,0x13,buff,arg->total_blk_16);
	GET_BPB_DATA_AT(fd,0x16,buff,arg->fat_blks);
	GET_BPB_DATA_AT(fd,0x20,buff,arg->total_blk_32)
	return true;
}

static uint32_t reverse(uint8_t* data,uint8_t len)
{
	uint32_t ret = 0;
	for(uint8_t i = 0;i < len;i++){
		ret |= data[i] << 8*(i);	
	}
	return ret;
}




/*

	////////////
	printf("BBB %d, %d %d , %d\n", root,offset, (offset < ROOT_ENTRY_MAX),ROOT_ENTRY_MAX);
	////////////

	////////////
	printf("BBB  %s %d type %d\n",buff,count, entries[id].type);
	////////////









static uint32_t get_entries_numb_in_dir(uint32_t dir_addr)
{
	if(dir_addr != ADDR_ROOT){

		uint32_t count = 0;
		uint32_t start = (dir_addr - ADDR_DATA)/BLOCK_SIZE;
		uint16_t cluster =  start + CLUSTER_OFFSET;
		uint16_t cluster_numb = 0; 

		clear_entry_table();
		entries = (_entry_t*)calloc(BLOCK_SIZE/ENTRY_SIZE,sizeof(_entry_t));
		if(!entries) return 0;
		//loop for cluster, implement entries loop in cluster loop 
		while((cluster = fat12_cluster_tracing(cluster)) != END_CLUSTER){
			//alloc more size if use more than one cluster entries
			if(cluster_numb){
				entries = (_entry_t*)realloc(entries,sizeof(_entry_t)*(BLOCK_SIZE/ENTRY_SIZE + cluster_numb)); 
			}
			uint16_t offset = cluster_numb*(BLOCK_SIZE/ENTRY_SIZE); //entries table's offset
			for (uint8_t id = offset; id < BLOCK_SIZE/ENTRY_SIZE; id++){
				//get fisrt 12 bytes to check if entry exist or not, btw check LFN or SFN too
				uint8_t buff[13] = {0};
				fseek(_file,ADDR_DATA + (cluster-CLUSTER_OFFSET)*BLOCK_SIZE,SEEK_SET);
				fgets((char*)buff,sizeof(buff),_file);
				//remove file type and NULL so minus 2
				if(!check_zero(buff,sizeof(buff)-2)){
					entries[id].id = id;
					entries[id].cluster = cluster;
					entries[id].type = (buff[11] != 0x0F) ? SFN_ENTRY : LFN_ENTRY;
					count++;
				}
			}
			cluster_numb++; //number of cluster are used increase
		}
		return count;
	}
	return fat->root_entries;
}




static uint16_t fat12_cluster_tracing(uint16_t start)
{
	uint8_t buffer[4] = {0};
	uint32_t addr = ADDR_FAT + ((start%2==0) ? (start*1.5f) : (start*1.5f-1));
//	fseek(_file,(int)(ADDR_FAT+start*1.5f),SEEK_SET);
	fseek(_file,addr,SEEK_SET);
	fgets((char*)buffer,sizeof(buffer),_file);
	uint16_t first_entry = (uint8_t)(buffer[1]&0x0f)*16*16+buffer[0]; 
	uint16_t second_entry = (buffer[2]*16+(buffer[1]>>4)); 

	printf("cluster 1 :%02X 2 %02X \n",first_entry,second_entry );
	// if(start%2 & start%3 == 1) return first_entry;
	// else return second_entry;
	if(start%2 == 0){
	 	return first_entry;
	}
	else return second_entry;
}


///////////////////

// void move_to_addr(uint32_t addr)
// {
// 	// current_dir = addr;
// }


// uint32_t ps_file_in_dir_parsing()
// {
// 	//create cluster chain

// 	//maybe each entry is a file and all cluster full entries ?
// 	uint32_t file_id = 0;
// 	uint32_t entries = get_possible_entries_numb_in_dir(fat,current_dir);
// 	uint8_t buff[13] = {0}; //file name, ext, type name l/s, null
// 	//create a bucket
// 	table = (_entry_t*)calloc(entries,sizeof(_entry_t));
// 	if(!table) return 0;
// 	//put in bucket 
// 	for(uint32_t entry = 0;entry < entries;entry++){
// 		uint32_t addr = get_entry_addr_in_bytes(current_dir,entry);
// 		fseek(_file,addr,SEEK_SET);
// 		fgets((char*)buff,sizeof(buff),_file);
// 		if(!check_zero(buff,sizeof(buff)-2)){ //remove type name and NULL
// 			if(buff[11] != 0x0F){ //check if not LFN
// 				table[file_id]->file_id = file_id;
// 				table[file_id]->entry   = entry;
// 				file_id++;
// 			}
// 		}
// 	}
// }



// static uint32_t get_entry_addr_in_bytes(uint32_t dir_addr, uint32_t index)
// {
// 	if(dir_addr != ADDR_ROOT){
// 		uint32_t block = index/(BLOCK_SIZE/ENTRY_SIZE); //cluster chain block-th
// 		if(block){
// 			uint32_t new_index = index%(BLOCK_SIZE/ENTRY_SIZE); //index in cluster chain block-th
// 			//calculate start from current dir
// 			uint32_t start = (dir_addr - ADDR_DATA)/(BLOCK_SIZE);
// 			uint16_t cluster =  start + CLUSTER_OFFSET;
// 			while(block--){//times to find next cluster
// 				cluster = fat12_cluster_tracing(cluster);
// 			}
// 			return (ADDR_DATA+(cluster*BLOCK_SIZE)+new_index*ENTRY_SIZE);
// 		}
// 	}
// 	return (dir_addr+index*ENTRY_SIZE);
// }




// static uint32_t get_possible_entries_numb_in_dir(uint32_t dir_addr)
// {
// 	if(dir_addr != ADDR_ROOT){
// 		//calculate start cluster link from dir addr
// 		uint32_t count = 0;
// 		uint32_t start = (dir_addr - ADDR_DATA)/BLOCK_SIZE;
// 		uint16_t cluster =  start + CLUSTER_OFFSET;
// 		while(cluster = fat12_cluster_tracing(cluster) != 0xFFFF){
// 			count++;
// 		}
// 		//1 cluster ~ 1 block 512 bytes ~ 16 entries
// 		return (count*(BLOCK_SIZE/ENTRY_SIZE));
// 	}
// 	return fat->root_entries;
// }


// uint32_t ps_get_file_count(void)
// {	
// 	return file_in_dir;
// }

// void ps_access_file_by_index(uint32_t id)
// {
// 	file_in_dir = 0;
// 	uint32_t addr = (id) ? : current_dir;
// 	uint32_t count = process_entries_in_dir(addr);
// 	for (uint32_t i = 0; i < count; ++i)
// 	{
// 		
// 	}
// 	if(id){

// 	}

// }

// void ps_get_file_date_by_index(uint32_t id,)
// {

// }

// void ps_get_file_time_by_index(uint32_t id,)
// {

// }

// void ps_get_file_name_by_index(uint32_t id,)
// {

// }

// ham nay nen o tren file
// void ps_fat_fopen_by_index(uint8_t attr)
// {
// 	if(attr == ATTR_NMFILE){

// 	}
// 	if else(attr == )
// }

// static uint32_t create_data_cluster_chain_by_dir(uint32_t dir_addr)
// {
// 	uint32_t count = 0;
// 	uint32_t start = (dir_addr - ADDR_DATA)/BLOCK_SIZE;
// 	uint16_t cluster =  start + CLUSTER_OFFSET;
// 	while(cluster = fat12_cluster_tracing(cluster) != 0xFFFF){
// 		count++;
// 	}

// 	cluster_chain.chain = (uint16_t*)calloc(count,sizeof(uint16_t));
// 	if(!cluster_chain.chain) return 0;
// 	cluster_chain.len = count;

// 	for(uint32_t i = 0;i < count;i++){
// 		cluster_chain.chain[i] = fat12_cluster_tracing(cluster_chain.chain[i])
// 	}
// 	while(cluster != 0xFFFF){
// 		cluster = fat12_cluster_tracing(cluster);
// 		cluster_chain.chain[i] = cluster;
// 		count++;
// 	}
// }

*/