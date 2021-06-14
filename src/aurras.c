#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

char * formaLinhaArgs (char *argv[],int argc) {
    char *buffer = malloc (150);
    for (int i = 1;i < argc;i++) {
        strcat(buffer,argv[i]);
        strcat(buffer," ");
    }
    strcat(buffer,"\n");
    return buffer;
}


int main(int argc,char *argv[]) {
    if (argc == 1) {
        write (1,"./aurras status\n./aurras transform input-filename output-filename filter-id1 filter-id2 ...\n",92);
    }
    else if (argc == 2 && strcmp(argv[1],"status") == 0) {
        write (1,"estado do servidor\n",19);
    }
    else if (argc >= 4 && strcmp(argv[1],"transform") == 0) {
        int pw = open ("servidor",O_WRONLY);
        char sPID[12];
        pid_t pid = getpid();
        sprintf(sPID, "%d", pid);
        mkfifo(sPID,0666);
        write(pw,&pid,4);
        char *args = formaLinhaArgs (argv,argc);
        int pr = open (sPID,O_RDWR);
        write (pr,args,150);
        unlink(sPID);
        close(pw);
    }
    else write (1,"Comando nao reconhecido!\n",25);
}