#include <stdio.h>
#include <syslog.h>

int main(int argc, char* argv[]) {
    FILE *fp;
    openlog(NULL, 0, LOG_USER);

    if (argc != 3){
	syslog(LOG_ERR, "Invalid number of arguements : %d", argc);
	return 1;
    }else{
	if(argc == 1){	
	    syslog(LOG_ERR, "Invalid number of arguements : %d", argc);
	    return 1;
	}
        fp = fopen(argv[1], "w");
	if(fp == NULL){
	    printf("File Doesn't exist\n"); 
	    syslog(LOG_ERR, "File doesn't exist");
	    return 1;
	}
	syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);
        fprintf(fp, "%s", argv[2]);
        fclose(fp);
    }
    return 0;
}
