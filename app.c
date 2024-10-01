//////////////////////////////////////////////////////////////////////
// All rights reserved. This program and the accompanying materials	//
// are made available under the terms of the Public License v1.0	//
// and Distribution License v1.0 which accompany this distribution.	//
// MODULE  	:	app.c 											    //
// AUTHOR 	: 	DonP - donp172748@gmail.com							//
// DATE   	: 	--/--/----											//
//////////////////////////////////////////////////////////////////////


//--------------INCLUDE------------------//
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "parsor.h"
#include "file.h"
#include "app.h"
//---------------------------------------//

//---------------GLOBAL------------------//
//---------------------------------------//

//--------------PRIVATE------------------//
static char* remove_null_in_name(char* data,uint16_t len);
static void flush_array(char* data,uint16_t len);
static void show_file_in_dir(void);
static void wait_usr_command(void);
//---------------------------------------//



void app_init(char* name)
{
	uint8_t success = true;
	success = ps_fat_file_parsing_init(name);
	if(!success) goto failed;
	success = fl_file_list_create();
	if(!success) goto failed;
	return;
failed:
	printf("APP INIT FAILED\n");
}

void app_process(void)
{
	show_file_in_dir();
	wait_usr_command();
}

void app_close(void)
{
	fl_file_list_delete();
	ps_fat_file_release();
}

#define FIELD     	6
#define SZ_ID   	5
#define SZ_NAME   	25
#define SZ_TYPE 	10
#define SZ_TIME   	20
#define SZ_DATE  	20
#define SZ_SIZE   	20

static void show_file_in_dir(void)
{
	fl_file_list_delete();
	fl_file_list_create();
	fl_file_filtering_process();
	folder_t* dir = fl_file_get_list();

	uint8_t offset = 0;
	char screen_sz[200] = {0};
	//start label
	sprintf(screen_sz+offset,"ID");
	offset += SZ_ID;	
	sprintf(screen_sz+offset,"NAME");
	offset += SZ_NAME;	
	sprintf(screen_sz+offset,"TYPE");
	offset += SZ_TYPE;	
	sprintf(screen_sz+offset,"TIME");
	offset += SZ_TIME;	
	sprintf(screen_sz+offset,"DATE");
	offset += SZ_DATE;	
	sprintf(screen_sz+offset,"SIZE");
	offset += SZ_SIZE;	

	flush_array(screen_sz,offset);
	memset(screen_sz,0,sizeof(screen_sz));
	offset = 0;
	printf("\n");
	//end label
	for(uint32_t id = 0; id < dir->count;id++){
		sprintf(screen_sz+offset,"%d",dir->files[id]->index);
		offset += SZ_ID;
		uint8_t len = (dir->files[id]->name_len > SZ_NAME) ? SZ_NAME:dir->files[id]->name_len;
		memcpy(screen_sz+offset,remove_null_in_name(dir->files[id]->name,len),len);
		offset += SZ_NAME;	
		sprintf(screen_sz+offset,"%s",dir->files[id]->ext);
		offset += SZ_TYPE;	
		sprintf(screen_sz+offset,"%02d:%02d",dir->files[id]->time.hour, dir->files[id]->time.min);
		offset += SZ_TIME;	
		sprintf(screen_sz+offset,"%02d/%02d/%04d",dir->files[id]->time.day,dir->files[id]->time.month,dir->files[id]->time.year);
		offset += SZ_DATE;	
		sprintf(screen_sz+offset,"%d",dir->files[id]->size);
		offset += SZ_SIZE;	


		flush_array(screen_sz,offset);
		memset(screen_sz,0,sizeof(screen_sz));
		offset = 0;
		printf("\n");
	}

}


static char* remove_null_in_name(char* data,uint16_t len)
{
	char* buff = (char*)calloc(len,1);
	if(!buff) return data;
	uint8_t count = 0;
	for (uint16_t i = 0; i < len; i++){
		if(data[i]){
			buff[count] = data[i];
			count++;
		} 
	}
	memset(data,0,len);
	memcpy(data,buff,count);
	free(buff);
	return data;
}


static void flush_array(char* data,uint16_t len)
{
	for (uint16_t i = 0; i < len; i++){
		printf("%c",data[i] ? data[i] : ' ');
	}
}


static void wait_usr_command(void)
{
	char cmd[10] = {0};
	printf("User Input:");
	scanf("%s",cmd);
	printf("\n");
	uint32_t id = atoi(cmd);
	folder_t* dir = fl_file_get_list();
	if(id >= dir->count) return;
	fl_file_fopen(id);
}



/*


	// for(uint32_t id = 0; id < dir->count;id++){
	// 	printf("%d",dir->files[id]->index);
	// 	if(dir->files[id]->lfn){
	// 		printf("%*s",4,"");
	// 		flush_array(dir->files[id]->name,(dir->files[id]->name_len > SZ_NAME) ? SZ_NAME:dir->files[id]->name_len);
	// 	}
	// 	else printf("%12s",dir->files[id]->name);
	// 	printf("%20s",dir->files[id]->ext);
	// 	// printf("%*d-%d",SZ_TIME, dir->files[id]->time.hour, dir->files[id]->time.min);
	// 	// printf("%*d/%d/%d",SZ_DATE,dir->files[id]->time.day,dir->files[id]->time.month,dir->files[id]->time.year);
	// 	// printf("%*d",SZ_SIZE,dir->files[id]->size);
	// 	printf("\n");
	// }




	for(uint32_t id = 0; id < dir->count;id++){
		printf("%d",dir->files[id]->index);
		if(dir->files[id]->lfn){
			printf("%*s",4,"");
			flush_array(dir->files[id]->name,(dir->files[id]->name_len > SZ_NAME) ? SZ_NAME:dir->files[id]->name_len);
		}
		else printf("%*s",12,dir->files[id]->name);
		printf("%*s",SZ_TYPE,dir->files[id]->ext);
		printf("%*d-%d",SZ_TIME, dir->files[id]->time.hour, dir->files[id]->time.min);
		printf("%*d/%d/%d",SZ_DATE,dir->files[id]->time.day,dir->files[id]->time.month,dir->files[id]->time.year);
		printf("%*d",SZ_SIZE,dir->files[id]->size);
		printf("\n");
	}



	for(uint32_t id = 0; id < dir->count;id++){
		sprintf(screen_sz+offset,"%d",dir->files[id]->index);
//		memset(screen_sz+offset,0x20,offset + SZ_ID - strlen(screen_sz+offset));
		offset += SZ_ID;	
//		sprintf(screen_sz+offset,"%d",dir->files[id]->index);
//		memset(screen_sz+offset,0x20,offset + SZ_NAME - strlen(screen_sz+offset));
		offset += SZ_NAME;	
		sprintf(screen_sz+offset,"%s",dir->files[id]->ext);
//		memset(screen_sz+offset,0x20,offset + SZ_TYPE - strlen(screen_sz+offset));
		offset += SZ_TYPE;	
		sprintf(screen_sz+offset,"%d-%d",dir->files[id]->time.hour, dir->files[id]->time.min);
//		memset(screen_sz+offset,0x20,offset + SZ_TIME - strlen(screen_sz+offset));
		offset += SZ_TIME;	
		sprintf(screen_sz+offset,"%d/%d/%d",dir->files[id]->time.day,dir->files[id]->time.month,dir->files[id]->time.year);
//		memset(screen_sz+offset,0x20,offset + SZ_DATE - strlen(screen_sz+offset));
		offset += SZ_DATE;	
		sprintf(screen_sz+offset,"%d",dir->files[id]->size);
//		memset(screen_sz+offset,0x20,offset + SZ_SIZE - strlen(screen_sz+offset));
		offset += SZ_SIZE;	


		flush_array(screen_sz,offset);
		memset(screen_sz,0,sizeof(screen_sz));
		offset = 0;

		// if(dir->files[id]->lfn){

		// }
		// else{

		// }
		printf("\n");
	}


*/