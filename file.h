#ifndef _FILE_H_
#define _FILE_H_

//////////////////////////////////////////////////////////////////////
// All rights reserved. This program and the accompanying materials	//
// are made available under the terms of the Public License v1.0	//
// and Distribution License v1.0 which accompany this distribution.	//
// MODULE  	:	file.h 												//
// AUTHOR 	: 	DonP - donp172748@gmail.com							//
// DATE   	: 	--/--/----											//
//////////////////////////////////////////////////////////////////////
#pragma pack (1)
//--------------INCLUDE------------------//
#include "parsor.h"
//---------------------------------------//

//---------------MACRO-------------------//




//---------------------------------------//

//--------------DECLARE------------------//


typedef struct file_t{
	char* name;
	char ext[4];
	uint32_t size;
	uint32_t index;
	date_t time;
	//private
	uint32_t entry_id;
	uint32_t name_len;
	uint8_t lfn;
}file_t;

typedef struct folder_t{
	file_t** files;
	uint32_t count;
}folder_t;

uint8_t fl_file_list_create(void);
folder_t* fl_file_get_list(void);
void fl_file_list_delete(void);
void fl_file_filtering_process(void);
void fl_file_fopen(uint16_t file_id);
//---------------------------------------//

#endif//_FILE_H_
