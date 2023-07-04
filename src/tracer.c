#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <stddef.h>
int main(int argc,char const *argv[]){
    int comando = 0;
    int mypid = getpid();
    
    char fifoescrita [50];
    char fifoleitura [50];

    sprintf (fifoescrita, "fifoCliEscrita");
    sprintf (fifoleitura, "fifoCliLeitura");
    
    /// Abertura dos fifos
    int fifoCE = open(fifoescrita, O_WRONLY);
    int fifoCL = open(fifoleitura, O_RDONLY);

    if(strcmp(argv[1], "status") == 0){
        comando = 1;
        int tam = 1;

        write (fifoCE, &comando, sizeof(int));

        struct timeval tempoStatus;
        gettimeofday(&tempoStatus, NULL);
        long tempoFinal = tempoStatus.tv_sec*1e3 + tempoStatus.tv_usec/ 1e3;

        write (fifoCE, &tempoFinal, sizeof(long));

        while(tam != 0){
            read(fifoCL, &tam, sizeof(int));
            //close(fifoCL);
            if(tam == -1) {
                printf("Não foi executado nenhum programa até ao momento.\n"); 
                break;
            }

            if(tam == 0) {
                break;
            }

            char mensagemStatus[tam+1];

            read(fifoCL, &mensagemStatus, sizeof(char)*tam);
            
            mensagemStatus[tam] = '\0';

            printf("%s\n", mensagemStatus);
        }
        return 0;
    }

    if(strcmp(argv[1], "stats-time") == 0){
        comando = 5;
        write(fifoCE, &comando, sizeof(int));

        int numPids = argc - 2;
        write(fifoCE, &numPids, sizeof(int));

        int vazia;
        read(fifoCL, &vazia, sizeof(int));
        if(vazia == 1){
            printf("Não existe nenhum processo na memória\n");
            return 0;
        }

        char stringPid[20];
        int pidProcurado;
        for(int i = 2; i < argc; i++){
            strcpy(stringPid, argv[i]);
            pidProcurado = atoi(stringPid);
            write(fifoCE, &pidProcurado, sizeof(int));
        }

        long tempoTotal;
        read(fifoCL, &tempoTotal, sizeof(long));

        printf("Total execution time is %ld ms\n", tempoTotal);

        return 0;
    }

     if(strcmp(argv[1], "stats-command") == 0){
        comando = 6;
        write(fifoCE, &comando, sizeof(int));

        int tam = strlen(argv[2]);
        write (fifoCE, &tam, sizeof(int));
        char programa[tam];
        strcpy(programa, argv[2]);
        programa[tam] = '\0';
        write(fifoCE, &programa, sizeof(char)*tam);

        int vazia;
        read(fifoCL, &vazia, sizeof(int));
        if(vazia == 1){
            printf("Não existe nenhum processo na memória\n");
            return 0;
        }

        int numPids = argc - 3;
        write (fifoCE, &numPids, sizeof(int));

        char stringPid[20];
        int pidProcurado;
        for(int i = 3; i < argc; i++){
            strcpy(stringPid, argv[i]);
            pidProcurado = atoi(stringPid);
            write(fifoCE, &pidProcurado, sizeof(int));
        }

        int progUsado;
        read(fifoCL, &progUsado, sizeof(int));

        printf("%s was executed %d times.\n", programa, progUsado);

        return 0;
    }

    if(strcmp(argv[1], "stats-uniq") == 0){
        comando = 7;
        write(fifoCE, &comando, sizeof(int));

        int numPids = argc - 2;
        write(fifoCE, &numPids, sizeof(int));

        int vazia;
        read(fifoCL, &vazia, sizeof(int));
        if(vazia == 1){
            printf("Não existe nenhum processo na memória\n");
            return 0;
        }

        char stringPid[20];
        int pidProcurado;
        for(int i = 2; i < argc; i++){
            strcpy(stringPid, argv[i]);
            pidProcurado = atoi(stringPid);
            write(fifoCE, &pidProcurado, sizeof(int));
        }

        int i = 0;
        int tam;
        int numUnicos;
        read(fifoCL, &numUnicos, sizeof(int));
        while(i < numUnicos){
            read(fifoCL, &tam, sizeof(int));
            char programaUnico[tam];
            read(fifoCL, &programaUnico, sizeof(char)*tam);
            programaUnico[tam] = '\0';

            printf("%s\n", programaUnico);
            i++;
        }
        return 0;
    }

    if(strcmp(argv[1], "terminar") == 0){
        comando = 8;
        write(fifoCE, &comando, sizeof(int));

        int terminar = -1;
        write(fifoCE, &terminar, sizeof(int));

        return 0;
    }

    if(argc < 3){
        printf("Argumentos insuficientes para realizar qualquer ação\n");
        return -1;
    }

    if(argc < 4){
        if(strcmp(argv[2], "-u") == 0) printf("Argumentos insuficientes para executar o <execute -u>\n");
        else printf("Argumentos insuficientes para executar o <execute -p?>\n");
        return -1;
    }
    
    if(strcmp(argv[2], "-u") == 0){
        comando = 2;
        write (fifoCE, &comando, sizeof(int));
   
        write (fifoCE, &mypid, sizeof(int));
 
        int tam = strlen(argv[3]) +1;
        write (fifoCE, &tam, sizeof(int));
        char programa[tam];
        strcpy(programa, argv[3]);
        write (fifoCE, &programa, sizeof(char)*tam);
        printf("Running PID %d\n\n", mypid);
        printf("Output do programa:\n\n");

        struct timeval start_time, end_time;
        /// recolha da timestamp em que a execução começa
        gettimeofday(&start_time, NULL);
        long tempoInicio = start_time.tv_sec*1e3 + start_time.tv_usec/1e3;
        write (fifoCE, &tempoInicio, sizeof(long));
        int i; 
        for(i = 4; i < argc; i++){
            pid_t pid = fork();
            if(pid == 0){ // filho
                execlp(argv[3], argv[3], argv[i], NULL);
                _exit(0);
            }
            else{//pai
                int status;
                waitpid(pid, &status, 0);
            }
            printf("\n");
        }
        /// recolha da timestamp em que a execução acaba
        comando = 3;
        write (fifoCE, &comando, sizeof(int));

        gettimeofday(&end_time, NULL);
        long tempoFinal = end_time.tv_sec*1e3 + end_time.tv_usec/ 1e3;

        write (fifoCE, &mypid, sizeof(int));

        write(fifoCE, &tempoFinal, sizeof(long));

        long tempoExec;
        read(fifoCL, &tempoExec, sizeof(long));

        printf("Ended in %ld ms\n", tempoExec);
        return 0;
    }

return 0;
}

/*
    if(strcmp(argv[2], "-p") == 0){
        int comando = 2;
        write(fifoCE, &comando, sizeof(int));

        write (fifoCE, &mypid, sizeof(int));

        int tamString = 0;
        for(int i = 3; i <= argc; i++){
            tamString += strlen(argv[i]) + 1;
        }
        char listaComandos[tamString];
        for(int i = 3; i < argc; i++){
            strcpy(listaComandos, argv[i]);
            if(i < argc) strcat(listaComandos, " ");
        }
        listaComandos[tamString] = '\0';

        printf("%s\n", listaComandos);

        write(fifoCE, &tamString, sizeof(int));
        write(fifoCE, &listaComandos, sizeof(char)*tamString);

        int numComandos = contaComandos(listaComandos);

        struct timeval start_time, end_time;
        /// recolha da timestamp em que a execução começa
        gettimeofday(&start_time, NULL);
        long tempoInicio = start_time.tv_sec*1e3 + start_time.tv_usec/1e3;
        write (fifoCE, &tempoInicio, sizeof(long));

        printf("Running PID %d\n\n", mypid);

        printf("Output do programa:\n\n");



        int i; 
        for(i = 4; i < argc; i++){
            pid_t pid = fork();
            if(pid == 0){ // filho
                execlp(argv[3], argv[3], argv[i], NULL);
                _exit(0);
            }
            else{//pai
                int status;
                waitpid(pid, &status, 0);
            }
            printf("\n");
        }

    }
*/

/*
int contaComandos(char *listaComandos){
    int com = 0;
    int i = 1;

    while(listaComandos[i] != '\0'){
        if(listaComandos[i] == '|') com++;
        
        i++;
    }

    return com;
}
*/