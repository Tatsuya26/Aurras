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
#include <signal.h>


static int *numFiltros;
static char** filterID;
static char** filterFile;
static int* filterLimit;
static int* filterUse;
pid_t processID[1000];
char *processInfo[1000];
int numTarefasAtivas = 0;
int numTarefasTotal = 0;

void sigTermhandler (int num) {
    unlink("servidor");
    int s;
    for (int i = 0; i < numTarefasAtivas;i++)
        wait(&s);
    _exit(0);
}

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
                int fdSaida = open (fichSaida,O_CREAT | O_TRUNC | O_WRONLY,0666);
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
    for (int i = 0;i < numFiltrosClient;i++) 
        wait(&s);
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
    int filterReserve[*numFiltros];
    for (int i = 0;i < *numFiltros;i++) 
        filterReserve[i] = filterUse[i]; 
    for (int i = 0;i < numFiltrosClient;i++) {
        int findex = checkFilter(filtros[i]);
        filterReserve[findex]++;
        if (filterReserve[findex] > filterLimit[findex]) r = 0;
    }
    return r;
}

void useFiltros(char *filtros[],int numFiltrosClient) {
    for (int i = 0;i < numFiltrosClient;i++) {
        int findex = checkFilter(filtros[i]);
        filterUse[findex]++;
    }
}

void preenchePInfo (char *fichEnt,char *fichSaida,char* filtros[],int numFiltrosClient) {
    processInfo[numTarefasAtivas] = malloc(200);
    char numProcesso[7] ; sprintf(numProcesso,"%d",numTarefasTotal + 1);
    strcat(processInfo[numTarefasAtivas],"task #");strcat(processInfo[numTarefasAtivas],numProcesso);strcat(processInfo[numTarefasAtivas],": ");
    strcat(processInfo[numTarefasAtivas],"transform "); strcat(processInfo[numTarefasAtivas]," ");
    strcat(processInfo[numTarefasAtivas],fichEnt);strcat(processInfo[numTarefasAtivas]," ");
    strcat(processInfo[numTarefasAtivas],fichSaida);strcat(processInfo[numTarefasAtivas]," ");
    for (int i = 0;i < numFiltrosClient;i++) {
        strcat(processInfo[numTarefasAtivas],filtros[i]);
        strcat(processInfo[numTarefasAtivas]," ");
    }
    strcat(processInfo[numTarefasAtivas],"\n");
}

void shiftLeftID(int index) {
    for (int i = index; i < numTarefasAtivas - 1;i++) {
        processID[i] = processID[i+1];
        processInfo[i] = processInfo[i+1];
    }
    numTarefasAtivas--;
}

void sendStatusClient (int fdClient,pid_t pidClient,pid_t pidServer)  {
    for (int i = 0; i < numTarefasAtivas;i++) {
        int status;
        pid_t return_pid = waitpid(processID[i], &status, WNOHANG); /* WNOHANG def'd in wait.h */
        if (return_pid == -1) {
            perror("");
        } else if (return_pid == 0) {
            write(fdClient,processInfo[i],200);   
        } else if (return_pid == processID[i]) {
            shiftLeftID(i);i--;
        }
    }
    for (int i = 0; i < *numFiltros;i++) {
        char filtroInfo[150] = "filter ";
        char useLimit[10];
        sprintf(useLimit,"%d/%d ",filterUse[i],filterLimit[i]);
        strcat(filtroInfo,filterID[i]);strcat(filtroInfo,": ");
        strcat(filtroInfo,useLimit);strcat(filtroInfo," (running/max)\n");
        write(fdClient,filtroInfo,150);
    }
    char pid[10];sprintf(pid,"%d",pidServer);
    char pidS[20] = "pid: ";strcat(pidS,pid);strcat(pidS,"\n"); 
    write(fdClient,pidS,20);
    close(fdClient);
    kill(pidClient,SIGPIPE);
}

void transformFile (char *fichEnt,char *fichSaida,char *filtros[],int numFiltrosClient,pid_t pid) {
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
    }
    else kill(pid,SIGTERM);
}

int main(int argc, char *argv[]) {
    if (signal(SIGTERM,sigTermhandler) == SIG_ERR) perror("");
    if (argc == 3) {
        numFiltros = mmap(NULL,sizeof(int),PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);
        filterFile = mmap(NULL,sizeof(char*) * 10,PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);
        filterID = mmap(NULL,sizeof(char*) * 10,PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);
        filterLimit = mmap(NULL,sizeof(int) * 10,PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);
        filterUse = mmap(NULL,sizeof(int) * 10,PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1,0);

        int fdConf;
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
                int numFiltrosClient = 0,p;
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
                    if ((p = fork()) == 0) {
                        transformFile(fichEnt,fichSaida,filtros,numFiltrosClient,pid);
                        close(pr);
                        close (pclient);
                        _exit(1);
                    }
                    else {
                        processID[numTarefasAtivas] = p;
                        preenchePInfo (fichEnt,fichSaida,filtros,numFiltrosClient);
                        numTarefasTotal++;numTarefasAtivas++;
                    }
                }
                if (strcmp(comando,"status\n") == 0) {
                    pid_t pidS  = getpid();
                    sendStatusClient (pclient,pid,pidS);
                }
            }
            close(pclient);
            close(pr);
            }
        }
        }
    }
}