#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>


static int *numFiltros;
static char** filterID;
static char** filterFile;
static int* filterLimit;
static int* filterUse;

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
    int i;
    for (i = 0; readln(fdConf,buffer,100) > 0 && i < 10; i++) {
        char *tmp = malloc (100);
        strcpy(tmp,dir_filters);
        filterID[i] = strsep(&buffer," ");
        filterFile[i] = strcat(tmp,strsep(&buffer," "));
        filterLimit[i] = atoi(strsep(&buffer," "));
        filterUse[i] = 0;
        buffer = malloc(100);
    }
    *numFiltros = i;
    return *numFiltros;
}

int checkFilter (char *filtro) {
    for (int i = 0; i < *numFiltros;i++) {
        if (strcmp(filtro,filterID[i]) == 0) return i;
    }
    return -1;
}

void aplicaFiltros (char *filtros[],int numFiltrosClient,char *fichEnt,char* fichSaida) {
    int pipes[numFiltrosClient-1][2];
    int s;
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
        wait(&s);        
    }
}

int possivelFiltro (char *filtros[],int numFiltrosClient) {
    int filtroMax[*numFiltros];
    for (int i = 0;i < *numFiltros;i++) 
        filtroMax[i] = 0;
    for (int i = 0;i < numFiltrosClient;i++) {
        int findex = checkFilter(filtros[i]);
        filtroMax[findex]++;
    }
    for (int i = 0; i < *numFiltros;i++)
        if (filtroMax[i] > filterLimit[i]) return 0;
    return 1;
}

void removeFiltros(char *filtros[],int numFiltrosClient) {
    for (int i = 0;i < numFiltrosClient;i++) {
        int findex = checkFilter(filtros[i]);
        filterUse[findex]--;
    }
}

int verificaFiltros (char *filtros[],int numFiltrosClient) {
    int r = 1;
    for (int i = 0;i < numFiltrosClient;i++) {
        int findex = checkFilter(filtros[i]);
        filterUse[findex]++;
        if (filterUse[findex] > filterLimit[findex]) r = 0;
    }
    removeFiltros(filtros,numFiltrosClient);
    return r;
}

void useFiltros(char *filtros[],int numFiltrosClient) {
    for (int i = 0;i < numFiltrosClient;i++) {
        int findex = checkFilter(filtros[i]);
        filterUse[findex]++;
    }
}

int main(int argc, char *argv[]) {
    if (argc == 3) {
        numFiltros = mmap(NULL,sizeof(int),PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);
        filterFile = mmap(NULL,sizeof(char*) * 10,PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);
        filterID = mmap(NULL,sizeof(char*) * 10,PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);
        filterLimit = mmap(NULL,sizeof(int) * 10,PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);
        filterUse = mmap(NULL,sizeof(int) * 10,PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);

        int fdConf,numTarefas;
        if ((fdConf = open(argv[1],O_RDONLY)) == -1) perror("");
        else {
        *numFiltros = getConf (fdConf,argv[2]);
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
                printf("%d\n",filterUse[2]);
                if (strcmp(comando,"transform") == 0) {
                    numTarefas++;
                    if (fork() == 0) {
                    char *fichEnt = strsep(&args," ");
                    char *fichSaida = strsep(&args," ");
                    char *filtrosLine = strsep(&args,"\n");
                    while (filtrosLine != NULL && *filtrosLine != '\0')  {
                        filtros[numFiltrosClient++] = strsep(&filtrosLine," ");
                    }
                    if (possivelFiltro(filtros,numFiltrosClient)) {
                    if (verificaFiltros(filtros,numFiltrosClient)) {
                        kill(pid,SIGUSR2);
                        useFiltros(filtros,numFiltrosClient);
                        aplicaFiltros (filtros,numFiltrosClient,fichEnt,fichSaida);
                        removeFiltros(filtros,numFiltrosClient);
                        kill (pid,SIGTERM);
                    }
                    else {
                        kill(pid,SIGUSR1);
                        while (!verificaFiltros(filtros,numFiltrosClient));
                        kill(pid,SIGUSR2);
                        useFiltros(filtros,numFiltrosClient);
                        aplicaFiltros (filtros,numFiltrosClient,fichEnt,fichSaida);
                        removeFiltros(filtros,numFiltrosClient);
                        kill (pid,SIGTERM);
                    }
                    close(pr);
                    _exit(1);
                    }
                    else kill(pid,SIGTERM);
                }
                    numTarefas--;
                }
            }
            close(pr);
            }
        }
        }
    }
}