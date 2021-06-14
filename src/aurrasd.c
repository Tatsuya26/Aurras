#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>


int numFiltros;
char* filterID[10];
char* filterFile[10];
int filterLimit[10];

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

int getConf (int fdConf,char *dir_filters) {
    char* buffer = malloc (100);
    for (numFiltros = 0; readln(fdConf,buffer,100) > 0 && numFiltros < 10; numFiltros++) {
        char *tmp = malloc (100);
        strcpy(tmp,dir_filters);
        filterID[numFiltros] = strsep(&buffer," ");
        filterFile[numFiltros] = strcat(tmp,strsep(&buffer," "));
        filterLimit[numFiltros] = atoi(strsep(&buffer," "));
        buffer = malloc(100);
    }
    return numFiltros;
}

int checkFilter (char *filtro) {
    for (int i = 0; i < numFiltros;i++) {
        if (strcmp(filtro,filterID[i]) == 0) return i;
    }
    return -1;
}

void aplicaFiltros (char *filtros[],int numFiltrosClient,char *fichEnt,char* fichSaida) {
    int pipes[numFiltrosClient-1][2];
    for (int i = 0; i  < numFiltrosClient-1;i++) {
        pipe(pipes[i]);
    }
    for (int i = 0;i < numFiltrosClient;i++) {
        if (i == 0) {
            int p = fork();
            if (p == 0) {
                int fdEnt = open (fichEnt,O_RDONLY);
                dup2(fdEnt,0);
                dup2(pipes[i][1],1);
                int findex = checkFilter(filtros[i]);
                execl(filterFile[findex],filterFile[findex],NULL);
                _exit(0);
            }
            else {
                close(pipes[i][1]);
            }
        }
        else if (i == numFiltrosClient-1) {
            int p = fork();
            if (p == 0) {
                int fdSaida = open (fichSaida,O_CREAT | O_WRONLY,0666);
                dup2(fdSaida,1);
                dup2(pipes[i-1][0],0);
                int findex = checkFilter(filtros[i]);
                execl(filterFile[findex],filterFile[findex],NULL);
                _exit(0);
            }
            else {
                close(pipes[i-1][0]);
            }
        }
        else {
            int p = fork();
            if (p == 0) {
                dup2(pipes[i][1],1);
                dup2(pipes[i-1][0],0);
                close (pipes[i-1][0]);
                close (pipes[i][1]);
                int findex = checkFilter(filtros[i]);
                execl(filterFile[findex],filterFile[findex],NULL);
                _exit(0);
            }
            else {
                close(pipes[i][1]);
                close(pipes[i-1][0]);
            }
        }        
    }
 }

int main(int argc, char *argv[]) {
    if (argc == 3) {
        int fdConf,numTarefas;
        if ((fdConf = open(argv[1],O_RDONLY)) == -1) perror("");
        else {
        numFiltros = getConf (fdConf,argv[2]);
        mkfifo("servidor",0666);
        while(1) {
            int pr = open("servidor",O_RDONLY);
            pid_t pid = 0;
            int res = read (pr,&pid,4);
            if (res > 0) {
            char clientPID[12];
            sprintf(clientPID, "%d", pid);
            int pclient = open (clientPID,O_RDWR);
            if (pclient == -1) perror(clientPID);
            else {
                int numFiltrosClient = 0;
                char *filtros[10];
                char *args = malloc(150);
                readln (pclient,args,150);
                char *comando = strsep(&args," ");
                if (strcmp(comando,"transform") == 0) {
                    char *fichEnt = strsep(&args," ");
                    char *fichSaida = strsep(&args," ");
                    char *filtrosLine = strsep(&args,"\n");
                    while (filtrosLine != NULL && *filtrosLine != '\0')  {
                        filtros[numFiltrosClient++] = strsep(&filtrosLine," ");
                    }
                    aplicaFiltros (filtros,numFiltrosClient,fichEnt,fichSaida);
                }
            }
            close(pr);
            }
        }
        }
    }
}