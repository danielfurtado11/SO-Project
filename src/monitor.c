#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "process.h"
#include <stdlib.h>
#include "hashtable.h"
#include "server_funcs.h"

#define MAXBUFFER 4048

int main(int argc, char* argv[]){
    char *path = argv[1];
    int read_bytes;
    int buffer;
    int status;
    int chan,chanw;
    //int status;
    if((chan = open("tmp/fifo", O_RDONLY)) == -1) {
        // create the fifo
        if(mkfifo("tmp/fifo", 0666) == -1){
            perror("error fifo");
        }
        chan = open("tmp/fifo", O_RDONLY);
        if (chan == -1) perror("erro ao abrir terminal de leitura"); 
    }

    chanw = open("tmp/fifo", O_WRONLY);
    if (chanw == -1) perror("erro ao abrir terminal de escrita"); 


    GHashTable* process_hash = create_process_hash();
    pid_t pidFifo;

    while ((read_bytes = read(chan, &pidFifo, sizeof(pid_t))) > 0){


            //printf("DEBUG: Pid do fifo: %d\n", pidFifo);
            char pidStr[20];
            sprintf(pidStr, "tmp/%d", pidFifo);
        
            int fdFifo = open(pidStr, O_RDONLY);

            read(fdFifo, &buffer, sizeof(int));
            //printf("Debug do buffer (tarefa nº): %d\n",buffer);
            if(buffer==11){
                //EXECUTE U
                executeU(fdFifo, process_hash);
                close(fdFifo);

            } else if(buffer==12){
                //EXECUTE UF
                executeUF(fdFifo, process_hash, path);
                close(fdFifo);
                unlink(pidStr);
               
            }
            else if(buffer==13){
                break;
            }
            else {
            if (fork() == 0 ) {
            
                if(buffer==14){
                    //STATUS
                    
                    int pidEscrita;
                    read(fdFifo, &pidEscrita, sizeof(int));
                   // printf("PID DO FIFO 2: %d\n",pidEscrita);

                    char pidLeitura[40];
                    sprintf(pidLeitura, "tmp/%d", pidEscrita);
                
                    int fdFifoW = open(pidLeitura, O_WRONLY);
                   
                    statusExe(fdFifoW, process_hash);
                    close(fdFifo);
                    close(fdFifoW);
                    unlink(pidStr);
                    
                }

                else if (buffer==15) {
                    //STATS-TIME
                    int time = 0;
                    int pidEscrita, pidFile;

                    read(fdFifo,&pidEscrita,sizeof(int));

                    char fifoEscrita[40];
                    sprintf(fifoEscrita, "tmp/%d", pidEscrita);
                    int fdFifoW = open(fifoEscrita, O_WRONLY);


                    while((read(fdFifo, &pidFile,sizeof(int))) > 0){
                        if (pidFile == -1) break;
                        //printf("Nome do ficheiro: %d\n",pidFile);
                        time += statsTimeExe(pidFile);
                    }



                    
                    write(fdFifoW,&time,sizeof(int));

                    close(fdFifoW);
                    close(fdFifo);
                    unlink(pidStr);
                }

                else if (buffer == 16){
                    //STATS-COMMAND
                    int total = 0;
                    int pidEscrita, comSize, pidFile;
                    
                    read(fdFifo,&pidEscrita,sizeof(int));

                    char fifoEscrita[40];
                    sprintf(fifoEscrita, "tmp/%d", pidEscrita);
                    int fdFifoW = open(fifoEscrita, O_WRONLY);

                    read(fdFifo,&comSize,sizeof(int));
                    char command[comSize];
                    read(fdFifo, command, comSize);
                    //printf("Comando a procurar: %s\n",command);


                    while((read(fdFifo, &pidFile,sizeof(int))) > 0){
                        if (pidFile == -1) break;
                        total += statsCommandExe(pidFile,command);
                    }

                    
                    write(fdFifoW,&total,sizeof(int));

                    close(fdFifoW);
                    close(fdFifo);
                    unlink(pidStr);

                }

                else if (buffer == 17){
                    //STATS-UNIQ
                    int pidEscrita, pidFile;
                    char** comandosFinal = (char **) malloc(1 * sizeof(char *));
                    comandosFinal[0] = NULL;
                    int sizeComFinal = 0;

                    read(fdFifo,&pidEscrita,sizeof(int));

                    char fifoEscrita[40];
                    sprintf(fifoEscrita, "tmp/%d", pidEscrita);
                    int fdFifoW = open(fifoEscrita, O_WRONLY);

                    while((read(fdFifo, &pidFile, sizeof(int))) > 0){
                        if (pidFile == -1) break;
                        char ** comandos = statsUniq(pidFile);
                        int size = sizeArray(comandos);

                        for (int i = 0; i<size; i++){
                            int existe = verificaComandos(comandos[i],comandosFinal);
                            if (existe == 0){
                                comandosFinal = (char **)realloc(comandosFinal, sizeof(char*) * (sizeComFinal + 1));
                                comandosFinal[sizeComFinal] = comandos[i];
                                sizeComFinal++;
                                int sizeComando = strlen(comandos[i])+1;
                                write(fdFifoW, &sizeComando, sizeof(int));
                                write(fdFifoW,comandos[i],sizeComando);
                            }
                        }
                        
                    }
                    int brk = -1;
                    write(fdFifoW,&brk,sizeof(int));
                    free(comandosFinal);

                    close(fdFifoW);
                    close(fdFifo);
                    unlink(pidStr);
                }


                else{
                    perror("Error on reading next command");
                }
                
                _exit(1);
            }
        }
       
        waitpid(-1, &status, WNOHANG);
    }
    unlink("tmp/fifo");
    close(chan);
    close(chanw);
    printf("Servidor desligado\n");

    return 0;
}
