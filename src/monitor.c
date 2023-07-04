// codigo servidor

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#define NumProcessosMáximos 35

typedef struct listaStatus{
    int pid;
    char programa[50];
    long tempoExe;
    struct listaStatus *prox;
} listaStatus;

typedef struct processosExec{
    int pid;
    char programa[50];
    long tempoInic;
    struct processosExec *prox;
} processosExec;

listaStatus* elemNovoLS(listaStatus *Memoria, int pid, char programa[], long tempoExec){
    listaStatus *novoElem = malloc(sizeof(listaStatus));
    novoElem->pid = pid;
    strcpy(novoElem->programa, programa);
    novoElem->tempoExe = tempoExec; 

    novoElem->prox = Memoria;

    return novoElem;
}

processosExec* elemNovoPE(processosExec *Status, int pid, char programa[], long tempoInic){
    processosExec *novoElem = malloc(sizeof(processosExec));
    novoElem->pid = pid;
    strcpy(novoElem->programa, programa);
    novoElem->tempoInic = tempoInic; 

    novoElem->prox = Status;

    return novoElem;
}

void removeElemPE(processosExec **Status, int pid){
    processosExec* aux = *Status;
    processosExec* pt = aux;
    processosExec* anterior = NULL;

    while(pt != NULL && pt->pid != pid){
        anterior = pt;
        pt = pt->prox;
    }
    if(pt == NULL) printf("Algo fudeu esta a remover uma cena que não existe\n");
    if(anterior == NULL) aux = pt->prox;
    else anterior->prox = pt->prox;
    free(pt);
    *Status = aux; 
}

listaStatus* procuraElemMem(listaStatus *Memoria, int pid){
    listaStatus* aux = Memoria;
    listaStatus* elemProcurado;

    if(aux == NULL){
        printf("A lista de processos em exec está vazia\n");
        return NULL;  
    } 

    while (aux != NULL){
        if(aux->pid == pid){
            elemProcurado = aux;
            return elemProcurado;
        }
        aux = aux->prox;
    }

    return NULL;
}

processosExec* procuraElem(processosExec *Status, int pid){
    processosExec* aux = Status;
    processosExec* elemProcurado;

    if(aux == NULL){
        printf("A lista de processos em exec está vazia\n");
        return NULL;  
    } 

    while (aux != NULL){
        if(aux->pid == pid){
            elemProcurado = aux;
            return elemProcurado;
        }
        aux = aux->prox;
    }

    return NULL;
}

int main(int argc,char const *argv[]){

    listaStatus *Memoria = NULL;
    listaStatus *auxM = NULL;
    processosExec *Status = NULL;
    processosExec *aux = NULL;

    char fifoServerEscrita[50];
    char fifoServerLeitura[50];
    char nomeFicheiro[50];

    sprintf (fifoServerLeitura, "fifoCliEscrita");
    sprintf (fifoServerEscrita, "fifoCliLeitura");

    mkfifo(fifoServerLeitura, 0622);
    mkfifo (fifoServerEscrita, 0622);

    int fifoSL = open(fifoServerLeitura, O_RDONLY); 
    int fifoSE = open(fifoServerEscrita, O_WRONLY);
    int ficheiro;

    int terminar = 1;

    while(terminar == 1){
        int comando = 0;  
        read(fifoSL, &comando, sizeof(int));

        if(comando == 1){

            aux = Status;

            long tempoFinal;
            read(fifoSL, &tempoFinal, sizeof(long));

                long tempoFinalStatus;

                char mensagemStatus[150];
                int tam;

                if(aux == NULL){
                    tam = -1;
                    write(fifoSE, &tam, sizeof(int));
                }

                while (aux != NULL){
                    tempoFinalStatus = tempoFinal - aux->tempoInic;
                    sprintf(mensagemStatus, "Pid: %d | Nome do Programa: %s | Tempo em execução: %ld ms. (ainda em execução)",Status->pid, Status->programa, tempoFinalStatus);
                    tam = strlen(mensagemStatus);

                    write(fifoSE, &tam, sizeof(int));
                    write(fifoSE, &mensagemStatus, sizeof(char)*tam);
                    aux = aux->prox;
                    }
                        
                    if(aux == NULL){
                        tam = 0;
                        write(fifoSE, &tam, sizeof(int));
                    }

            comando = 0;
        }
        
        else if(comando == 2){
            int pidClienteAtual;
            read(fifoSL, &pidClienteAtual, sizeof(int));
            
            int tam;
            read(fifoSL, &tam, sizeof(int));
            char programaAtual[tam];
            read(fifoSL, &programaAtual, sizeof(char)*tam);
            
            long tempoInicial;
            read(fifoSL, &tempoInicial, sizeof(long));

            Status = elemNovoPE(Status, pidClienteAtual, programaAtual, tempoInicial);

            comando = 0;
        }

        else if(comando == 3){
            processosExec *elemaRemover;
            int pidClienteAtual;
            read(fifoSL, &pidClienteAtual, sizeof(int));
            
            long tempoFinal, tempoExec;
            read(fifoSL, &tempoFinal, sizeof(long));

            elemaRemover = procuraElem(Status, pidClienteAtual);
            if(elemaRemover == NULL) printf("O elemento a remover da lista não existia\n");
            
            tempoExec = tempoFinal - elemaRemover->tempoInic;

            Memoria = elemNovoLS(Memoria, pidClienteAtual, elemaRemover->programa, tempoExec);

            write(fifoSE, &tempoExec, sizeof(long));

            sprintf(nomeFicheiro,"%s/%d.txt", argv[1], pidClienteAtual);
            ficheiro = open(nomeFicheiro, O_CREAT | O_WRONLY | O_TRUNC, 0644);

            char conteudoFicheiro[150];
            sprintf(conteudoFicheiro, "Pid: %d | Nome do Programa: %s | Tempo final de execução: %ld ms.", pidClienteAtual, elemaRemover->programa, tempoExec);
            int tam = strlen(conteudoFicheiro);
            conteudoFicheiro[tam] = '\0';
            write(ficheiro, &conteudoFicheiro, sizeof(char)*tam);
            close(ficheiro);

            removeElemPE(&Status, pidClienteAtual);

            comando = 0;
        }

        else if(comando == 5){
            int i = 0;
            long somaTempo = 0;
            int numPids;
            read(fifoSL, &numPids, sizeof(int));

            int vazia = 0;
            auxM = Memoria;
            if(auxM == NULL){
                vazia = 1;
                write(fifoSE, &vazia, sizeof(int));
            }
            else{
                write(fifoSE, &vazia, sizeof(int));
                listaStatus *elemProcurado;
                int pidProcurado;
                while(i < numPids){
                    read(fifoSL, &pidProcurado, sizeof(int));
                    elemProcurado = procuraElemMem(Memoria, pidProcurado);
                    somaTempo += elemProcurado->tempoExe;
                    i++;
                }

                write(fifoSE, &somaTempo, sizeof(long));

                comando = 0;
            }
        }

        else if(comando == 6){
            int tam;
            read(fifoSL, &tam, sizeof(int));
            char programaProcurado[tam];
            read(fifoSL, &programaProcurado, sizeof(char)*tam);
            programaProcurado[tam] = '\0';

            int vazia = 0;
            auxM = Memoria;
            if(auxM == NULL){
                vazia = 1;
                write(fifoSE, &vazia, sizeof(int));
            }
            else{
                write(fifoSE, &vazia, sizeof(int));

                int i = 0;
                int numPids;
                read(fifoSL, &numPids, sizeof(int));

                int progUsado = 0;
                listaStatus *elemProcurado;
                int pidProcurado;
                while(i < numPids){
                    read(fifoSL, &pidProcurado, sizeof(int));
                    elemProcurado = procuraElemMem(Memoria, pidProcurado);
                    if(strcmp(programaProcurado, elemProcurado->programa) == 0){
                        progUsado++;
                    }
                    i++;
                }

                write(fifoSE, &progUsado, sizeof(int));
                
                comando = 0;
            }
        }

        else if(comando == 7){
            int i = 0;
            int j;
            int numPids;
            read(fifoSL, &numPids, sizeof(int));

            int vazia = 0;
            auxM = Memoria;
            if(auxM == NULL){
                vazia = 1;
                write(fifoSE, &vazia, sizeof(int));
            }
            else{
                write(fifoSE, &vazia, sizeof(int));
                listaStatus *elemProcurado;

                char progamasUnicos[numPids][50]; /// matriz com o número de programas únicos no máximo existe 1 por cada pid
                int numUnicos = 0;
                
                int pidProcurado;
                while(i < numPids){
                    read(fifoSL, &pidProcurado, sizeof(int));
                    elemProcurado = procuraElemMem(Memoria, pidProcurado);

                    for(j = 0; j < numUnicos; j++){
                        if(strcmp(progamasUnicos[j], elemProcurado->programa) == 0){
                            break;
                        }
                    }

                    if(j == numUnicos){
                        strcpy(progamasUnicos[numUnicos], elemProcurado->programa);
                        numUnicos++;
                    }

                    i++;
                }

                write(fifoSE, &numUnicos, sizeof(int));
                int tam;
                for(i = 0; i < numUnicos; i++){
                    tam = strlen(progamasUnicos[i]);
                    write(fifoSE, &tam, sizeof(int));
                    write(fifoSE, &progamasUnicos[i], sizeof(char)*tam);
                }

                comando = 0;
            }
        }

        else if(comando == 8){
            read(fifoSL, &terminar, sizeof(int));
        }
    }
    
    close(fifoSE);
    close(fifoSL);

    return 0;

}