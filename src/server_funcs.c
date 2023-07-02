#include "server_funcs.h"

int executeU (int fdFifo, GHashTable* process_hash){

    Process processo;

    read(fdFifo, &processo, sizeof(processo));
    insert_process_hash(process_hash, &processo);    
    //g_hash_table_foreach(process_hash, print_process, NULL);
            
    return 1;
}

int executeUF(int fdFifo, GHashTable* process_hash, char *path){

    Process processo;
    int bytes_read;
    bytes_read = read(fdFifo, &processo, sizeof(processo));

    //printf("Bytes Read ExecuteUF: %d\n",bytes_read);
    
                //printf("PID: %d\n", processo.pid);
                //printf("AFTER| MiliSeconds: %ld\n", processo.timestamp);

                Process* p = get_process_hash(process_hash, processo.pid);
                if (p == NULL) {
                perror("Struct not found in hashtable.\n");
                }

                    
                /*printf("HASHTABLE PID do programa a ser executado: %d\n", p->pid);
                printf("HASHTABLE Nome do programa a ser executado: %s\n", p->comando);
                printf("HASHTABLE Inicio milisegundos: %ld\n", p->timestamp);
                */

                char* pidStr = malloc(sizeof(char) *16);
                sprintf(pidStr, "/%d", processo.pid);
                char* pathPid = malloc(sizeof(char) *100);
                pathPid[0] = '\0';
                strcat(pathPid,path);
                strcat(pathPid,pidStr);
                free(pidStr);

                int fd = open (pathPid, O_CREAT | O_TRUNC | O_WRONLY, 0640);
                if (fd < 0)
                    {
                        perror("Erro a abrir ficheiro de Processos");
                        return -1;
                    }

                long tempo = processo.timestamp - p->timestamp;
                p->timestamp = tempo;
                //printf("Tempo Diff: %ld\n",tempo);

                write(fd, p , sizeof(Process));
                remove_process_hash(process_hash, &(p->pid));
                free(pathPid);
                close(fd);


                //DEBUG DO FICHEIRO
                /*Process pTESTE;

                int fdTeste = open (pathPid, O_RDWR,0600);
                    if (fdTeste < 0){
                        perror("Erro a abrir ficheiro de pessoas");
                        return -1;
                    }
                while ((read(fd,&pTESTE,sizeof(pTESTE)))>0){
                        printf("PID: %d\n", processo.pid);
                        printf("Tempo de Execução| MiliSeconds: %ld\n", processo.timestamp);
                        printf("Comando: %s\n",processo.comando);
                    }
                free(pathPid);
                close(fdTeste);
                */
            
        
    return 1;
}



void statusExe(int fdFifoW, GHashTable* process_hash){

    GHashTableIter iter;
    gpointer key, value;
    struct timeval atual;

    gettimeofday (&atual, NULL);
    long atual_ms = atual.tv_sec * 1000L + atual.tv_usec / 1000L;
    //g_hash_table_foreach(process_hash, print_process, NULL);

    g_hash_table_iter_init (&iter, process_hash);

    while (g_hash_table_iter_next (&iter, &key, &value)){
        
        Process* p = (Process*) value;
        /*
        printf("LOOOP PID do programa a ser executado: %d\n", p->pid);
        printf("LOOP Nome do programa a ser executado: %s\n", p->comando);
        printf("LOOP Inicio milisegundos: %ld\n", p->timestamp);
        */

        p->timestamp = atual_ms - p->timestamp;
        write(fdFifoW, p, sizeof(Process));
    }
    
}

long statsTimeExe(int pid){
    double time = 0.0;
    char file[50];
    sprintf(file,"PIDS-folder/%d",pid);
    int fd = open(file,O_RDONLY,0640);
    if (fd<0){
        perror("Erro ao abrir o ficheiro de PIDS");
        return -1;
    }
    Process p;
    while (read(fd,&p,sizeof(Process)) > 0){
        time += p.timestamp;
    }
    close(fd);
    return time;
}



int statsCommandExe(int pid, char* command){
    int total = 0;
    char file[20];
    sprintf(file,"PIDS-folder/%d",pid);
    int fd = open(file,O_RDONLY,0640);
    if (fd<0){
        perror("Erro ao abrir o ficheiro de PIDS");
        return -1;
    }
    Process p;
    while (read(fd,&p,sizeof(Process)) >0){
        //printf("Comando do PID %d: %s\n",pid,p.comando);
        if (strcmp(command,p.comando) == 0){
            total++;
        }
    }
    close(fd);
    return total;
}

int sizeArray(char **array){
    int size = 0;
    while(array[size] != NULL){
        size++;
    }
    return size;
}

char** statsUniq(int pid){
    char file[20];
    sprintf(file,"PIDS-folder/%d",pid);
    int fd = open(file,O_RDONLY,0640);

    char **comandos = (char **) malloc(1 * sizeof(char *));
    int numComandos = 0;


    if (fd<0){
        perror("Erro ao abrir o ficheiro de PIDS");
        return NULL;
    }
    Process p;

    while(read(fd,&p,sizeof(Process)) >0){
        comandos = (char **) realloc(comandos, (numComandos+1) * sizeof(char *));           // armazena espaço no array
        comandos[numComandos] = (char *) malloc((strlen(p.comando) +1) * sizeof(char));     // armazena espaço para a string
        strcpy(comandos[numComandos], p.comando);
        numComandos++;
    }
    return comandos;
}


int  verificaComandos(char* comando, char ** comFinal){
    int sizeComFinal = sizeArray(comFinal);
    int existe = 0;
    for(int j = 0; j < sizeComFinal; j++){
        if(strcmp(comando, comFinal[j]) == 0){
            existe = 1;
            break;
        }
    }


    return existe;

}


