#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int main(int argc,char *argv[]) {
    if (argc == 1) {
        write (1,"./aurras status\n./aurras transform input-filename output-filename filter-id1 filter-id2 ...\n",92);
    }
    else if (argc == 2 && strcmp(argv[1],"status") == 0) {
        write (1,"estado do servidor\n",19);
    }
    else if (argc >= 4 && strcmp(argv[1],"transform") == 0) {
        write (1,"processar ficheiro\n",19);
    }
    else write (1,"Comando nao reconhecido!\n",25);
}