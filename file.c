//////////////////////////////////////////////////////////////////////
// All rights reserved. This program and the accompanying materials	//
// are made available under the terms of the Public License v1.0	//
// and Distribution License v1.0 which accompany this distribution.	//
// MODULE  	:	file.c 											    //
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
#include "file.h"
//---------------------------------------//

//---------------GLOBAL------------------//
//---------------------------------------//

//--------------PRIVATE------------------//
static folder_t* current = NULL; 
//---------------------------------------//



uint8_t fl_file_list_create(void)
{
	current = (folder_t*)calloc(1,sizeof(folder_t));
	if(!current) return false;
	current->count = ps_get_sfn_entries_number();

	current->files = (file_t**)calloc(current->count,sizeof(void*));
	if(!current->files){
		fl_file_list_delete();
		return false;
	}
	return true;
}

folder_t* fl_file_get_list(void)
{
	return current;
}


void fl_file_list_delete(void)
{
	if(current){
		if(current->files){
			for (uint32_t i = 0; i < current->count;i++){
				if(current->files[i]){
					if(current->files[i]->name)
						free(current->files[i]->name);
				}
				free(current->files[i]);
			}
		}
		free(current->files);
	}
	free(current);
	current = NULL;
}



void fl_file_filtering_process(void)
{
	uint32_t entries = ps_get_all_entries_number();
	uint16_t offset = 0;  	//index data occupied in lfn_buff;
	uint16_t file_cnt = 0;
	uint8_t  lfn_cnt = 0; //number of frame lfn, or lfn entries number per file
	uint8_t  lfn_cnt_max = 0;
	uint8_t* lfn_buff = NULL; //create buffer store temp data
	uint8_t  lfn_flg = false; //has LFN or not
 	for (uint32_t i = 0; i < entries;i++){ //entries
 		entry_t* entry = ps_get_entry_data_by_id(i);
		if(entry->type){ //LFN 
			if(!lfn_flg){
				lfn_cnt_max = entry->data.lname[0] - '@';
				lfn_buff = (uint8_t*)calloc(lfn_cnt_max,sizeof(entry->data.lname));
			}
			//put lname to buffer
			memcpy(lfn_buff+lfn_cnt*sizeof(entry->data.lname),entry->data.lname+1,entry->name_len);
			if(offset < lfn_cnt_max*sizeof(entry->data.lname)) offset += entry->name_len; 
			lfn_flg = true;
			lfn_cnt++;
		}
		else{ //SFN, get file info, name fisrt
			current->files[file_cnt] = (file_t*)calloc(1,sizeof(file_t));
			if(lfn_flg){ //if has LFN 
				current->files[file_cnt]->name = (char*)calloc(offset,1);
				//set name length here
				current->files[file_cnt]->name_len = offset;
				// //the order is ..LFN2 LFN1 SFN, and LFN1 start the name :( 
				for(uint16_t j = lfn_cnt; j != 0;j--){
					if(offset > sizeof(entry->data.lname)){
						memcpy(current->files[file_cnt]->name,lfn_buff+(j-1)*(sizeof(entry->data.lname)),sizeof(entry->data.lname));
						offset -= sizeof(entry->data.lname);
					}
					else{
						memcpy(current->files[file_cnt]->name,lfn_buff+(j-1)*(sizeof(entry->data.lname)),offset);
						offset = 0;
					}
				}
				if(lfn_buff) free(lfn_buff);
			}		
			else{ //get name from SFN
				current->files[file_cnt]->name = (char*)calloc(sizeof(entry->data.sname.name),1);
				current->files[file_cnt]->name_len = entry->name_len;
				memcpy(current->files[file_cnt]->name,entry->data.sname.name,entry->name_len);
			}			
			//after that is ext
			memcpy(current->files[file_cnt]->ext,entry->data.sname.ext,3);
			current->files[file_cnt]->size = entry->data.sname.size;
			current->files[file_cnt]->index = file_cnt;
			current->files[file_cnt]->time = entry->data.sname.time; 
 			//private member
			current->files[file_cnt]->entry_id = i;
			current->files[file_cnt]->lfn = lfn_flg;
			//clear data for the next file
			file_cnt++;
			lfn_flg = false;
			lfn_cnt = 0;
			lfn_cnt_max = 0;
		}
	}
}




void fl_file_fopen(uint16_t file_id)
{
	ps_access_entry_by_id(current->files[file_id]->entry_id);
}





/*





 			for (int i = 0; i < sizeof(entry->data.lname); i++)
 			{
 			printf("%C",entry->data.lname[i] );
 			}
 			printf("\n");



			for (int id = 0; id < 27; id++)
			{
				printf("%c",entry->data.lname[id]);
			}
			printf("\n");
		}


			for (int id = 0; id < 27; id++)
			{
				printf("%c",lfn_buff[id]);
			}
			printf("\n");


			
			for (int id = 0; id < 27; id++)
			{
				printf("%c",current->files[file_cnt]->name[id]);
			}	
			printf("\n");



#define BUFF_SZ 32

void fl_file_filtering_process(void)
{
	uint32_t entries = ps_get_all_entries_number();
	uint8_t* lfn_buff = (uint8_t*)calloc(BUFF_SZ,1); //create buffer store temp data, should we do this ?
	if(!lfn_buff) return;
	uint16_t offset = 0;  	//index data occupied in lfn_buff;
	uint16_t file_cnt = 0;
	uint8_t lfn_cnt = 0;  	//number of frame lfn, or lfn entries number per file, multiple of sizeof(entry->data.lname)
	uint8_t lfn_flg = false; //has LFN or not
 	for (uint32_t i = 0; i < entries;i++){ //entries
 		entry_t* entry = ps_get_entry_data_by_id(i);
		if(entry->type){ //LFN 
			//put lname to buffer
			memcpy(lfn_buff+lfn_cnt*sizeof(entry->data.lname),entry->data.lname+1,entry->name_len);
			//printf("AAAA %d\n",entry->name_len );
			if(offset < BUFF_SZ) offset += entry->name_len; 
			lfn_flg = true;
			lfn_cnt++;
		}
		else{ //SFN, get file info, name fisrt
			current->files[file_cnt] = (file_t*)calloc(1,sizeof(file_t));
			if(lfn_flg){ //if has LFN 
				current->files[file_cnt]->name = (char*)calloc(offset,1);
				//set name length here
				current->files[file_cnt]->name_len = offset;
				// //the order is ..LFN2 LFN1 SFN, and LFN1 start the name :( 
				for(uint16_t j = lfn_cnt; j != 0;j--){
					if(offset > sizeof(entry->data.lname)){
						memcpy(current->files[file_cnt]->name,lfn_buff+(j-1)*(sizeof(entry->data.lname)),sizeof(entry->data.lname));
						offset -= sizeof(entry->data.lname);
					}
					else{
						memcpy(current->files[file_cnt]->name,lfn_buff+(j-1)*(sizeof(entry->data.lname)),offset);
						offset = 0;
					}
				}
			}		
			else{ //get name from SFN
				current->files[file_cnt]->name = (char*)calloc(offset,1);
				current->files[file_cnt]->name_len = entry->name_len;
				memcpy(current->files[file_cnt]->name,entry->data.sname.name,entry->name_len);
			}			
			//after that is ext
			memcpy(current->files[file_cnt]->ext,entry->data.sname.ext,3);
			current->files[file_cnt]->size = entry->data.sname.size;
			current->files[file_cnt]->index = file_cnt;
			current->files[file_cnt]->time = entry->data.sname.time; 
 			//private member
			current->files[file_cnt]->entry_id = i;
			current->files[file_cnt]->lfn = lfn_flg;
			file_cnt++;
			lfn_flg = false;
			lfn_cnt = 0;
			memset(lfn_buff,0,BUFF_SZ);
		}
	}
	free(lfn_buff);
}


// code crash dung struct do ham` create failed ma van chay ham nay
void fl_file_filtering_process(void)
{
	uint32_t entries = ps_get_all_entries_number();
	uint8_t* lfn_buff = (uint8_t*)calloc(BUFF_SZ,1); //create buffer store temp data, should we do this ?
	if(!lfn_buff) return;
	uint16_t offset = 0; //index data occupied in lfn_buff;
	uint16_t file_cnt = 0;
	uint16_t lfn_sz = 0; //lfn size name ~~ 27 ? 
 	for (uint32_t i = 0; i < 2;i++){ //entries
 		entry_t entry = ps_get_entry_data_by_id(i);
		if(entry.type){ //LFN
			lfn_sz = sizeof(entry.data.lname);
			//put lname to buffer
			memcpy(lfn_buff+offset,entry.data.lname,lfn_sz);
			if(offset < BUFF_SZ) offset += lfn_sz; 
		}
		else{ //SFN, get file info, name fisrt
			current->files[file_cnt].name = (char*)calloc(offset,1);
			if(!current->files[file_cnt].name){
				fl_file_list_delete();
				return;
			}
			//the order is ..LFN2 LFN1 SFN, and LFN1 start the name :( 
			for(uint16_t j = offset/lfn_sz; j != 0;j--){
				memcpy(current->files[file_cnt].name,lfn_buff+(j-1)*lfn_sz,lfn_sz);		
			}
			//after that is ext
			memcpy(current->files[file_cnt].ext,entry.data.sname.ext,3);
			current->files[file_cnt].size = entry.data.sname.size;
			current->files[file_cnt].index = file_cnt;
			//current->files[file_cnt].time = entry.data.sname.time; // if assign two struct, CRASH there
			//memcpy(&current->files[file_cnt].time,&entry.data.sname.time,sizeof(date_t)); //Still CRASH
			current->files[file_cnt].time.second = entry.data.sname.time.second;
			current->files[file_cnt].time.min = entry.data.sname.time.min;
			current->files[file_cnt].time.hour = entry.data.sname.time.hour;
			current->files[file_cnt].time.day = entry.data.sname.time.day;
			current->files[file_cnt].time.month = entry.data.sname.time.month;
			current->files[file_cnt].time.year = entry.data.sname.time.year;
 			// //private member
			current->files[file_cnt].entry_id = i;
			file_cnt++;
			offset = 0; //clear lfn buffer's offset 
		}
	}
	free(lfn_buff);
}




//ham nay failed do lfn_cnt khong reset ve 0
void fl_file_filtering_process(void)
{
	uint32_t entries = ps_get_all_entries_number();
	uint8_t* lfn_buff = NULL; //create buffer store temp data, should we do this ?
	uint16_t offset = 0;  	//index data occupied in lfn_buff;
	uint16_t file_cnt = 0;
	uint8_t lfn_cnt = 0;  	//number of frame lfn, or lfn entries number per file, multiple of sizeof(entry->data.lname)
	uint8_t lfn_cnt_max = 0;
	uint8_t lfn_flg = false; //has LFN or not

 	for (uint32_t i = 0; i < 2;i++){ //entries
 		entry_t* entry = ps_get_entry_data_by_id(i);
		if(entry->type){ //LFN 
			//realloc at the first time meet lfn with new size is 'A'-'@'
			if(!lfn_flg){
				lfn_cnt_max = entry->data.lname[0]-'@';
				lfn_buff = (uint8_t*)calloc(lfn_cnt_max,sizeof(entry->data.lname)); 
				if(!lfn_buff) return;
			} 
			//put lname to buffer, remove first character
			memcpy(lfn_buff+lfn_cnt*sizeof(entry->data.lname),entry->data.lname+1,entry->name_len);
			if(offset < lfn_cnt_max*sizeof(entry->data.lname)) offset += entry->name_len; 
			lfn_flg = true;
			lfn_cnt++;
		}
		else{ //SFN, get file info, name fisrt
			current->files[file_cnt] = (file_t*)calloc(1,sizeof(file_t));
			if(!current->files[file_cnt]) goto exit;	
			if(lfn_flg){ //if has LFN 
				current->files[file_cnt]->name = (char*)calloc(offset,1);
				if(!current->files[file_cnt]->name)	goto exit;	
				//set name length here
				current->files[file_cnt]->name_len = offset;
				// //the order is ..LFN2 LFN1 SFN, and LFN1 start the name :( 
				for(uint16_t j = lfn_cnt; j != 0;j--){
					if(offset > sizeof(entry->data.lname)){
						memcpy(current->files[file_cnt]->name,lfn_buff+(j-1)*(sizeof(entry->data.lname)),sizeof(entry->data.lname));
						offset -= sizeof(entry->data.lname);
					}
					else{
						memcpy(current->files[file_cnt]->name,lfn_buff+(j-1)*(sizeof(entry->data.lname)),offset);
						offset = 0;
					}
				}
			}		
			else{ //get name from SFN
				current->files[file_cnt]->name = (char*)calloc(offset,1);
				if(!current->files[file_cnt]->name)	goto exit;
				current->files[file_cnt]->name_len = entry->name_len;
				memcpy(current->files[file_cnt]->name,entry->data.sname.name,entry->name_len);
			}
			//after that is ext
			memcpy(current->files[file_cnt]->ext,entry->data.sname.ext,3);
			current->files[file_cnt]->size = entry->data.sname.size;
			current->files[file_cnt]->index = file_cnt;
			current->files[file_cnt]->time = entry->data.sname.time; 
			//memcpy(&current->files[file_cnt].time,&entry->data.sname.time,sizeof(date_t)); 
			// current->files[file_cnt]->time.second = entry->data.sname.time.second;
			// current->files[file_cnt]->time.min = entry->data.sname.time.min;
			// current->files[file_cnt]->time.hour = entry->data.sname.time.hour;
			// current->files[file_cnt]->time.day = entry->data.sname.time.day;
			// current->files[file_cnt]->time.month = entry->data.sname.time.month;
			// current->files[file_cnt]->time.year = entry->data.sname.time.year;
 			//private member
			current->files[file_cnt]->entry_id = i;
			current->files[file_cnt]->lfn = lfn_flg;
			file_cnt++;
			lfn_flg = false;
			if(lfn_buff){
				free(lfn_buff);
				lfn_buff = NULL;
			} 
		}
	}
	return; //success
exit:	
	fl_file_list_delete();		
	if(lfn_buff) free(lfn_buff);
}




void fl_file_filtering_process(void)
{
	uint32_t entries = ps_get_all_entries_number();
	uint8_t* lfn_buff = (uint8_t*)calloc(BUFF_SZ,1); //create buffer store temp data, should we do this ?
	if(!lfn_buff) return;
	uint16_t offset = 0;  	//index data occupied in lfn_buff;
	uint16_t file_cnt = 0;
	uint8_t lfn_cnt = 0;  	//number of frame lfn, or lfn entries number per file, multiple of sizeof(entry->data.lname)
	uint8_t lfn_flg = false; //has LFN or not
 	for (uint32_t i = 0; i < entries;i++){ //
 		entry_t* entry = ps_get_entry_data_by_id(i);
		if(entry->type){ //LFN 
			//put lname to buffer
			memcpy(lfn_buff+lfn_cnt*sizeof(entry->data.lname),entry->data.lname,entry->name_len);
			if(offset < BUFF_SZ) offset += entry->name_len; 
			lfn_flg = true;
			lfn_cnt++;
		}
		else{ //SFN, get file info, name fisrt
			current->files[file_cnt] = (file_t*)calloc(1,sizeof(file_t));
			if(!current->files[file_cnt]){
				free(lfn_buff);
				return;
			} 
			if(lfn_flg){ //if has LFN 
				current->files[file_cnt]->name = (char*)calloc(offset,1);
				if(!current->files[file_cnt]->name){
					fl_file_list_delete();
					free(lfn_buff);
					return;
				}
				//set name length here
				current->files[file_cnt]->name_len = offset;
				// //the order is ..LFN2 LFN1 SFN, and LFN1 start the name :( 
				for(uint16_t j = lfn_cnt; j != 0;j--){
					if(offset > sizeof(entry->data.lname)){
						memcpy(current->files[file_cnt]->name,lfn_buff+(j-1)*(sizeof(entry->data.lname)),sizeof(entry->data.lname));
						offset -= sizeof(entry->data.lname);
					}
					else{
						memcpy(current->files[file_cnt]->name,lfn_buff+(j-1)*(sizeof(entry->data.lname)),offset);
						offset = 0;
					}
				}
			}		
			else{ //get name from SFN
				current->files[file_cnt]->name = (char*)calloc(offset,1);
				if(!current->files[file_cnt]->name){
					fl_file_list_delete();
					free(lfn_buff);
					return;
				}
				current->files[file_cnt]->name_len = entry->name_len;
				memcpy(current->files[file_cnt]->name,entry->data.sname.name,entry->name_len);
			}
			//after that is ext
			memcpy(current->files[file_cnt]->ext,entry->data.sname.ext,3);
			current->files[file_cnt]->size = entry->data.sname.size;
			current->files[file_cnt]->index = file_cnt;
			current->files[file_cnt]->time = entry->data.sname.time; 
			//memcpy(&current->files[file_cnt].time,&entry->data.sname.time,sizeof(date_t)); 
			// current->files[file_cnt]->time.second = entry->data.sname.time.second;
			// current->files[file_cnt]->time.min = entry->data.sname.time.min;
			// current->files[file_cnt]->time.hour = entry->data.sname.time.hour;
			// current->files[file_cnt]->time.day = entry->data.sname.time.day;
			// current->files[file_cnt]->time.month = entry->data.sname.time.month;
			// current->files[file_cnt]->time.year = entry->data.sname.time.year;
 			//private member
			current->files[file_cnt]->entry_id = i;
			current->files[file_cnt]->lfn = lfn_flg;
			file_cnt++;
			lfn_flg = false;
			memset(lfn_buff,0,BUFF_SZ);
		}
	}
	free(lfn_buff);
}




*/







