#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>


ssize_t readln (int fd, char *buffer,size_t size) {
	ssize_t res = 0;
	ssize_t j = 0;
	char local[1];

	while ((res = read (fd,local,1)) > 0) {
		if (local[0] != '\n') {
				buffer[j] = local[0];
				j++;
			}
			else {
				buffer[j] = local[0];
				j++;
				return j;
			}
		}
	return j;
}

int getConf (int fdConf,char *filterID[],char *filterFile[],int filterLimit[]) {
    char* buffer = malloc (100);
    int numFiltros;
    for (numFiltros = 0; readln(fdConf,buffer,100) > 0 && numFiltros < 10; numFiltros++) {
        filterID[numFiltros] = strsep(&buffer," ");
        filterFile[numFiltros] = strsep(&buffer," ");
        filterLimit[numFiltros] = atoi(strsep(&buffer," "));
        buffer = malloc(100);
    }
    return numFiltros;
}

int main(int argc, char *argv[]) {
    if (argc == 3) {
        int fdConf,numTarefas;
        if ((fdConf = open(argv[1],O_RDONLY)) == -1) perror("");
        else {
        int numFiltros;
        char* filterID[10];
        char* filterFile[10];
        int filterLimit[10];
        numFiltros = getConf (fdConf,filterID,filterFile,filterLimit);
        mkfifo("servidor",0666);
        //while(1) {
            //int pr = open("servidor",O_RDONLY);
        //}
        }
    }
}