#ifndef _PARSOR_H_
#define _PARSOR_H_

//////////////////////////////////////////////////////////////////////
// All rights reserved. This program and the accompanying materials	//
// are made available under the terms of the Public License v1.0	//
// and Distribution License v1.0 which accompany this distribution.	//
// MODULE  	:	parsor.h											//
// AUTHOR 	: 	DonP - donp172748@gmail.com							//
// DATE   	: 	--/--/----											//
//////////////////////////////////////////////////////////////////////
#pragma pack (1)
//--------------INCLUDE------------------//
//---------------------------------------//

//---------------MACRO-------------------//
//attributes
#define ATTR_NMFILE 	0x00 
#define ATTR_RDONLY 	0x01
#define ATTR_HIDDEN 	0x02
#define ATTR_SYSTEM 	0x04
#define ATTR_VLABEL 	0x08
#define ATTR_SUBDIR 	0x10
#define ATTR_ARCHVE 	0x20



//Entry Type
#define SFN_ENTRY 	0x00 
#define LFN_ENTRY 	0x01	
//---------------------------------------//

//--------------DECLARE------------------//

//util
typedef struct date_t{
	uint8_t second;
	uint8_t min;
	uint8_t hour;
	uint8_t date;
	uint8_t day;
	uint8_t month;
	uint16_t year;
}date_t;

typedef struct sname_t{
	uint8_t name[8];
	uint8_t ext[3];
	uint8_t attr;		//attribute flag
	uint16_t cluster;  //start cluster content
	uint32_t size;
	date_t time;
}sname_t;

typedef union entry_data_t{
	uint8_t lname[28];
	sname_t sname; 
}entry_data_t;

typedef struct entry_t{
	uint8_t type;
	uint8_t name_len;
	entry_data_t data;
}entry_t;

//FAT API
uint8_t ps_fat_file_parsing_init(char* name);
void ps_fat_file_release(void);
//entry API
uint32_t ps_get_all_entries_number(void); // ??:D??
uint32_t ps_get_sfn_entries_number(void); // stupid code
entry_t* ps_get_entry_data_by_id(uint32_t id);
void ps_access_entry_by_id(uint32_t id);

//---------------------------------------//

#endif//_PARSOR_H_
