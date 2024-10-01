
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdint.h>
#include "parsor.h"
#include "app.h"




int main(int argc, char *argv[]) 
{
	app_init(argv[1]);
	while(1){
		app_process();
	}
	//should not reach here
	app_close();
	return 0;
}






/*

int main(int argc, char *argv[]) {
// argc is the number of arguments passed to the program
// argv is an array of strings representing the arguments
for (int i = 0; i < argc; i++) {
printf("Argument %d: %s\n", i, argv[i]);
}
return 0;
}





	FILE* fd = fopen("floppy.img","r");
	uint8_t temp[33] = {0};
	fseek(fd,0x200,SEEK_SET); //0x2600  0x11 0x4200
	fgets((char*)temp,33,fd); 
	int i = 0;

	for(i = 0;i<32;i++){
		printf("%02X",temp[i]);
	}
	printf("\n");
	for(i = 0;i<32;i++){
		printf("%c",(char)temp[i]);
	}	
	printf("\n");
	return 0;

getenv("HOME")

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

int main(void) {
DIR *d;
struct dirent *dir;
d = opendir(getenv("HOME")); // Replace with your path
if (d) {
while ((dir = readdir(d)) != NULL) {
printf("%s\n", dir->d_name);
}
closedir(d);
} else {
perror("Could not open directory");
return 1;
}
return 0;
}




char * getenv (const char *name)
#include <stdio.h>
#include <dirent.h>

int main(void) {
DIR *d;
struct dirent *dir;
d = opendir("."); // Replace with your path
if (d) {
while ((dir = readdir(d)) != NULL) {
printf("%s\n", dir->d_name);
}
closedir(d);
} else {
perror("Could not open directory");
return 1;
}
return 0;
}


#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
FILE *file;
int c;

if (argc < 2) {
fprintf(stderr, "Usage: %s <file>\n", argv[0]);
return 1;
}

file = fopen(argv[1], "r");
if (file) {
while ((c = fgetc(file)) != EOF) {
putchar(c);
}
fclose(file);
} else {
perror("Error opening file");
return 1;
}

return 0;
}

*/