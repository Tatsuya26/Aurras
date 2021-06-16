#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int leitura = 0;

char * formaLinhaArgs (char *argv[],int argc) {
    char *buffer = malloc (150);
    for (int i = 1;i < argc;i++) {
        strcat(buffer,argv[i]);
        strcat(buffer," ");
    }
    strcat(buffer,"\n");
    return buffer;
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

void sighandler1 (int num) {
    write (1,"pending...\n",11);
}

void sighandler2 (int num) {
    write (1,"processing...\n",14);
}

void sigTerm (int num) {
    pid_t pid = getpid();
    char sPID[12];
    sprintf(sPID, "%d", pid);
    unlink(sPID);
    _exit(0);
}

void sigpipe(int num) {
    leitura = 1;
}

int main(int argc,char *argv[]) {
    if (signal(SIGUSR1,sighandler1) == SIG_ERR) perror("");
    if (signal(SIGUSR2,sighandler2) == SIG_ERR) perror("");
    if (signal(SIGTERM,sigTerm) == SIG_ERR) perror("");
    if (signal(SIGPIPE,sigpipe) == SIG_ERR) perror("");
    if (argc == 1) {
        write (1,"./aurras status\n./aurras transform input-filename output-filename filter-id1 filter-id2 ...\n",92);
    }
    else if (argc == 2 && strcmp(argv[1],"status") == 0) {
        int pw = open ("servidor",O_WRONLY);
        char sPID[12];
        char buffer[500];
        pid_t pid = getpid();
        sprintf(sPID, "%d", pid);
        mkfifo(sPID,0666);
        write(pw,&pid,4);
        close(pw);
        int fw = open (sPID,O_RDWR);
        write (fw,"status\n",7);
        int fr = open (sPID,O_RDONLY);
        while (!leitura) pause();
        close(fw);
        int res = 0;
        while((res = readln (fr,buffer,500)) > 0)
            write (1,buffer,res);
        kill(getpid(),SIGTERM);
    }
    else if (argc >= 4 && strcmp(argv[1],"transform") == 0) {
        int pw = open ("servidor",O_WRONLY);
        char sPID[12];
        pid_t pid = getpid();
        sprintf(sPID, "%d", pid);
        mkfifo(sPID,0666);
        write(pw,&pid,4);
        close(pw);
        char *args = formaLinhaArgs (argv,argc);
        int pr = open (sPID,O_RDWR);
        write (pr,args,150);
        while (1) pause();
    }
    else write (1,"Comando nao reconhecido!\n",25);
}