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
#include <errno.h>
#define BUF_MAX 1024



char **tokenize (char *command, int *N)
{
    if (!command) return NULL;
    int max = 20, i=0;
    char *string;
    char **exec_args = malloc(max * sizeof(char *));


    while ((string = strsep(&command, " "))!=NULL)
    {
        if (strlen(string)<1) continue;
        if (i>=max)
        {
            max += max*0.5;
            exec_args = realloc(exec_args, max * sizeof(char *));
        }
        exec_args[i++] = string;
    }

    exec_args[i] = NULL;
    if (N) *N = i;

    return exec_args;
}


void filho0 (int *filedes, char *command)
{
    close(filedes[0]);

    dup2(filedes[1], 1);
    close(filedes[1]);

    char **args = tokenize(command, NULL);
    execvp(args[0], args);
    free(args);
    _exit(0);
}

void filhoN (int *filedes, char *command)
{
    close(filedes[1]);

    dup2(filedes[0], 0);
    close(filedes[0]);

    char **args = tokenize(command, NULL);
    execvp(args[0], args);
    free(args);
    _exit(errno);
}

void filhoi (int *filedesE, int *filedesS, char *command)
{
    close(filedesE[1]);
    close(filedesS[0]);

    dup2(filedesE[0], 0);
    dup2(filedesS[1], 1);

    close(filedesE[0]);
    close(filedesS[1]);

    char **args = tokenize(command, NULL);
    execvp(args[0], args);
    free(args);
    _exit(errno);
}



int pipeline(char* line, int chan) {   


    int n_processos = 1;
    // conta o nº de comandos a executar
    for (int i = 0; i < strlen(line); i++) {if (line[i] == '|'){n_processos++;}}

    int filedes[n_processos][2];

    char *token;
    char **comandos = malloc(n_processos * sizeof(char *));
    token = strtok(line,"|");

    int i = 0;
    while (token != NULL){
        comandos[i] = malloc(strlen(token) + 1);
        strcpy(comandos[i],token);
        token = strtok(NULL, "|");
        i++;
    }

    //for(int j = 0; j < n_processos; j++){
    //    printf("Comando %d: %s\n", j, comandos[j]);
    //}

    for (int i=0 ; i<n_processos-1 ; i++)
        if (pipe(filedes[i])==-1)
        {
            strerror(errno);
            exit(errno);
        }

    pid_t pid;

    for (int i=0 ; i<n_processos ; i++) {
        pid = fork();

        if (pid==-1) {
            strerror(errno);
            exit(errno);
        }

        if (!pid) {
            if (i==0) 
                filho0(filedes[i], comandos[i]);
            else if (i==n_processos-1)
                filhoN(filedes[i-1], comandos[i]);
            else
                filhoi(filedes[i-1], filedes[i], comandos[i]);
        }

        if (!i)
            close(filedes[i][1]);    // como fica o extremo de escrita do filho aberto, é necessario fechar o extremo de escrita do pai
        else if (i==n_processos-1)
            close(filedes[i-1][0]);  // como fica o extremo de leitura do filho aberto, é necessario fechar o extremo de leitura do pai
        else {
            close(filedes[i-1][0]);  // como fica o extremo de leitura do filho aberto, é necessario fechar o extremo de leitura do pai
            close(filedes[i][1]);    // como fica o extremo de escrita do filho aberto, é necessario fechar o extremo de escrita do pai
        }
    }

    for (int i=0 ; i<n_processos ; i++)
        wait(NULL);

    return 0;
}


int executePIPE(char *argv[], int chan) {

    struct timeval before;
    struct timeval after;

    pid_t pidPai = getpid();

    //Criar FIFO T-M
    char pidPaiStr[20];
    sprintf(pidPaiStr, "tmp/%d", pidPai);

    mkfifo(pidPaiStr,0666);
    
    write(chan, &pidPai, sizeof(pid_t));
    int fdFifo = open(pidPaiStr, O_WRONLY);

    int executeU = 11;
    write(fdFifo, &(executeU),sizeof(int));

    int pFilho; //PID do Processo que executou o comando
    int p[2];
    pipe(p);
    // cria filho
    if ((pFilho = fork()) == 0 ) {

        //Certificar que o filho não executa o comando antes do Pai mandar a informação para o Server
        close(p[1]);
        int f;
        read(p[0],&f,sizeof(int));
        close(p[0]);

        // excuta comando
        pipeline(argv[3],chan);
        //_exit(ret);
        _exit(1);


    }
    // Pai
    else {
        
        close(p[0]);

        char *comando_str = malloc(sizeof(char) * 200);
        comando_str[0] = '\0'; 
        strcpy(comando_str, argv[3]);
        //for (int i = 0; exec_args[i] != NULL; i++) {
        //    strcat(comando_str, exec_args[i]);
        //    if(exec_args[i+1] != NULL){
        //        strcat(comando_str, " ");
        //    }
        //}

        // Medição tempo inicial
        gettimeofday(&before, NULL);

        long beforeMS = before.tv_sec * 1000L + before.tv_usec / 1000L;
        //printf("BEFORE| MiliSeconds: %ld\n", beforeMS);

        Process processo = new_Process(pFilho,comando_str,beforeMS);
        if (memcmp(&processo, &(Process){0}, sizeof(Process)) == 0) {
            perror("Struct Processo não foi inicializado");
            return -1;
        }

        free(comando_str);

        // channel - escrita
        
        write(fdFifo,&processo,sizeof(processo));
        close(fdFifo);
        
        printf("Running PID %d\n", pFilho);

        //Certificar que o filho não executa o comando antes do Pai mandar a informação para o Server
        int m=1;
        write(p[1],&m,sizeof(int));
        close(p[1]);

        int status;
        int pidf = wait(&status);

        
        // WIFEXITED ve se o filho terminou normalmente
        if ( WIFEXITED(status) ) {
            write(chan, &pidPai, sizeof(pid_t));

            fdFifo = open(pidPaiStr, O_WRONLY);
            int executeUF =12;
            write(fdFifo, &executeUF,sizeof(int));
            
        
           
            // Medição do tempo final
            gettimeofday(&after, NULL);
            long afterMS = after.tv_sec * 1000L + after.tv_usec / 1000L;
            //printf("After| MiliSeconds: %ld\n", afterMS);

            long diff = afterMS - beforeMS;
            
            Process processo = new_Process(pidf," ",afterMS);
            /*
            printf("H | PID do programa : %d\n", processo.pid);
            printf("H | Nome do programa : %s\n", processo.comando);
            printf("H | Inicio milisegundos: %ld\n", processo.timestamp);
            */
            write(fdFifo,&processo,sizeof(processo));
            printf("Ended in %ld ms\n", diff);
        }
        else {
            printf("terminou abruptamente");
            return -1;
        }

        close(chan);
        close(fdFifo);
    }
    return 0;
}





int execute(char *argv[], int chan) {

    struct timeval before;
    struct timeval after;

    // parsing do input
    char *token;
    char *exec_args[20];
    int i = 0;
    char *comando = argv[3];

    while ( (token = strsep(&comando, " ")) != NULL ) {
        exec_args[i++] = token;
    }
    exec_args[i] = NULL;

    pid_t pidPai = getpid();

    //Criar FIFO T-M
    char pidPaiStr[20];
    sprintf(pidPaiStr, "tmp/%d", pidPai);

    mkfifo(pidPaiStr,0666);
    
    write(chan, &pidPai, sizeof(pid_t));
    int fdFifo = open(pidPaiStr, O_WRONLY);

    int executeU = 11;
    write(fdFifo, &(executeU),sizeof(int));

    int pFilho; //PID do Processo que executou o comando
    int p[2];
    pipe(p);
    // cria filho
    if ((pFilho = fork()) == 0 ) {

        //Certificar que o filho não executa o comando antes do Pai mandar a informação para o Server
        close(p[1]);
        int f;
        read(p[0],&f,sizeof(int));
        close(p[0]);

        // excuta comando
        int ret = execvp(exec_args[0],exec_args);
        perror("erro a executar comando");
        _exit(ret);


    }
    // Pai
    else {
        
        close(p[0]);

        char *comando_str = malloc(sizeof(char) * 200);
        comando_str[0] = '\0'; 
        for (int i = 0; exec_args[i] != NULL; i++) {
            strcat(comando_str, exec_args[i]);
            if(exec_args[i+1] != NULL){
                strcat(comando_str, " ");
            }
        }

        // Medição tempo inicial
        gettimeofday(&before, NULL);

        long beforeMS = before.tv_sec * 1000L + before.tv_usec / 1000L;
        //printf("BEFORE| MiliSeconds: %ld\n", beforeMS);

        Process processo = new_Process(pFilho,comando_str,beforeMS);
        if (memcmp(&processo, &(Process){0}, sizeof(Process)) == 0) {
            perror("Struct Processo não foi inicializado");
            return -1;
        }

        free(comando_str);

        // channel - escrita
        
        write(fdFifo,&processo,sizeof(processo));
        close(fdFifo);
        
        printf("Running PID: %d\n", pFilho);

        //Certificar que o filho não executa o comando antes do Pai mandar a informação para o Server
        int m=1;
        write(p[1],&m,sizeof(int));
        close(p[1]);

        int status;
        int pidf = wait(&status);

        
        // WIFEXITED ve se o filho terminou normalmente
        if ( WIFEXITED(status) ) {
            write(chan, &pidPai, sizeof(pid_t));

            fdFifo = open(pidPaiStr, O_WRONLY);
            int executeUF =12;
            write(fdFifo, &executeUF,sizeof(int));
            
        
           
            // Medição do tempo final
            gettimeofday(&after, NULL);
            long afterMS = after.tv_sec * 1000L + after.tv_usec / 1000L;
            //printf("After| MiliSeconds: %ld\n", afterMS);

            long diff = afterMS - beforeMS;
            
            Process processo = new_Process(pidf," ",afterMS);
            /*
            printf("H | PID do programa : %d\n", processo.pid);
            printf("H | Nome do programa : %s\n", processo.comando);
            printf("H | Inicio milisegundos: %ld\n", processo.timestamp);
            */
            write(fdFifo,&processo,sizeof(processo));
            printf("Ended in %ld ms\n", diff);
        }
        else {
            perror("terminou abruptamente");
            return -1;
        }

        close(chan);
        close(fdFifo);
    }
    return 0;
}


void status(int chan){
    int bytes_read;
    pid_t pidPai = getpid();
    Process processo;

    //FIFO PARA escrever com o monitor
    char pidPaiStr[40];
    sprintf(pidPaiStr, "tmp/%d", pidPai);
    mkfifo(pidPaiStr,0666);
    write(chan, &pidPai, sizeof(pid_t));
    int fdFifoW = open(pidPaiStr, O_WRONLY);

    int statusInt =14;
    write(fdFifoW, &statusInt,sizeof(int));
    
    //FIFO PARA LER com o monitor
    char pidPaiStrRead[45];
    char pidPaiStrReadNumber[30];
    sprintf(pidPaiStrReadNumber, "%d0057", pidPai);
    sprintf(pidPaiStrRead, "tmp/%d0057", pidPai);
    //printf("Numero: %s\n",pidPaiStrRead);
    if(mkfifo(pidPaiStrRead, 0666) == -1) {
        perror("Error creating fifo");
        exit(EXIT_FAILURE);
    }
    int pidPaiLeitura = atoi(pidPaiStrReadNumber);
    write(fdFifoW, &pidPaiLeitura,sizeof(int));

    int fdFifoL = open(pidPaiStrRead, O_RDONLY);
    if (fdFifoL == -1) {
        perror("Error opening fifo for reading");
        exit(EXIT_FAILURE);
    }
    


    close(fdFifoW);

    while( (bytes_read = read(fdFifoL, &processo, sizeof(Process))) >0){
        printf("%d %s %ld ms\n", processo.pid, processo.comando, processo.timestamp);
    }
    close(fdFifoL);
    unlink(pidPaiStrRead);
}

void statsTime(int chan,int* pids, int size){
    pid_t pid = getpid();
    int time = 0;

    //FIFO para escrever para o monitor
    char pidFifoW[20];
    sprintf(pidFifoW, "tmp/%d", pid);
    mkfifo(pidFifoW,0666);     // cria um FIFO com o PID do processo;
    write(chan,&pid, sizeof(int)); // manda para o FIFO principal do servidor o nome do FIFO criado
    
    int fdFifoW = open(pidFifoW,O_WRONLY);
    int statsTimeInt = 15;  // int pré-definido para o comando stats-time
    

    write(fdFifoW,&statsTimeInt,sizeof(int));   // manda o int para o servidor

    char pidFifoR[40];
    char pidPaiStrReadNumber[30];
    sprintf(pidPaiStrReadNumber, "%d0058", pid);
    sprintf(pidFifoR,"tmp/%d0058",pid);    
    int fifoLeitura = atoi(pidPaiStrReadNumber);


    if(mkfifo(pidFifoR, 0666) == -1) {
        perror("Error creating fifo");
        exit(EXIT_FAILURE);
    }
    
    write(fdFifoW, &fifoLeitura,sizeof(int));

    int fdFifoR = open(pidFifoR, O_RDONLY);
    if (fdFifoR == -1) {
        perror("Error opening fifo for reading");
        exit(EXIT_FAILURE);
    }   

    

    for (int i = 0; i<size; i++){
        write(fdFifoW,&pids[i],sizeof(int));
    }
    int brk = -1;
    write(fdFifoW,&brk,sizeof(int));
    
    read(fdFifoR,&time,sizeof(int));
    printf("Total execution time is %d ms\n", time);

    close(fdFifoR);
    close(fdFifoW);
    unlink(pidFifoR);
    unlink(pidFifoW);
}



void statsCommand(int chan, char * command, int* pids, int size){
    pid_t pid = getpid();
    int total = 0;
    
    char pidFifoW[40];
    sprintf(pidFifoW, "tmp/%d",pid);
    mkfifo(pidFifoW,0666);          // cria um FIFO com o nº do PID
    write(chan,&pid, sizeof(int));  // manda para o FIFO principal o PID
    
    int fdFifoW = open(pidFifoW,O_WRONLY);
    int statsCommandInt = 16;
    write(fdFifoW,&statsCommandInt,sizeof(int));
    
    char pidFifoR[40];
    char pidPaiStrReadNumber[30];
    sprintf(pidPaiStrReadNumber, "%d0059", pid);
    sprintf(pidFifoR,"tmp/%d0059",pid);
    int fifoLeitura = atoi(pidPaiStrReadNumber);
    
    if(mkfifo(pidFifoR, 0666) == -1) {
        perror("Error creating fifo");
        exit(EXIT_FAILURE);
    }

    write(fdFifoW, &fifoLeitura,sizeof(int));       // envia o nome do FIFO para receber do servidor

    int fdFifoR = open(pidFifoR,O_RDONLY);
    if (fdFifoR == -1) {
        perror("Error opening fifo for reading");
        exit(EXIT_FAILURE);
    }  

    int comSize = strlen(command) +1;
    write(fdFifoW, &comSize, sizeof(int));  // envia o tamanho da string para não bloquear
    write(fdFifoW,command,strlen(command)+1);   // envia o nome do programa


    for (int i = 0; i<size; i++){
        write(fdFifoW,&pids[i],sizeof(int));    // envia os PIDS todos
    }
    int brk = -1;
    write(fdFifoW,&brk,sizeof(int));        // envia o commando para terminar de enviar PIDS

    read(fdFifoR,&total,sizeof(int));
    printf("%s was executed %d times\n",command,total);

    close(fdFifoR);
    close(fdFifoW);
    unlink(pidFifoR);
    unlink(pidFifoW);

}






void statsUniq(int chan, int* pids, int size){
    pid_t pid = getpid();

    //FIFO para escrever para o monitor
    char pidFifoW[40];
    sprintf(pidFifoW, "tmp/%d", pid);
    mkfifo(pidFifoW,0666);     // cria um FIFO com o PID do processo;
    write(chan,&pid, sizeof(int)); // manda para o FIFO principal do servidor o nome do FIFO criado


    int fdFifoW = open(pidFifoW,O_WRONLY);
    int statsTimeInt = 17;  // int pré-definido para o comando stats-time
    

    write(fdFifoW,&statsTimeInt,sizeof(int));   // manda o int para o servidor

    char pidFifoR[40];
    char pidPaiStrReadNumber[30];
    sprintf(pidPaiStrReadNumber, "%d0060", pid);
    sprintf(pidFifoR,"tmp/%d0060",pid);    
    int fifoLeitura = atoi(pidPaiStrReadNumber);


    if(mkfifo(pidFifoR, 0666) == -1) {
        perror("Error creating fifo");
        exit(EXIT_FAILURE);
    }
    
    write(fdFifoW, &fifoLeitura,sizeof(int));

    int fdFifoR = open(pidFifoR, O_RDONLY);
    if (fdFifoR == -1) {
        perror("Error opening fifo for reading");
        exit(EXIT_FAILURE);
    }   

    

    for (int i = 0; i<size; i++){
        write(fdFifoW,&pids[i],sizeof(int));
    }
    int brk = -1;
    write(fdFifoW,&brk,sizeof(int));

    char ** array = (char ** ) malloc(10 * sizeof(char *));
    int pos = 0;
    array[0] = NULL;
    int capacity = 10;
    
    int sizeComando = 0;
    while (1){
        read(fdFifoR,&sizeComando,sizeof(int));
        if (sizeComando == -1) break;

        char * comando = malloc(sizeComando);
        read(fdFifoR,comando,sizeComando);
        printf("%s\n",comando);
        if (pos == capacity) {
            capacity = capacity * 2;
            array = realloc(array,capacity * sizeof(char *));
        }
        array[pos] = strdup(comando);
        pos++;
        free(comando);
    }


    close(fdFifoR);
    close(fdFifoW);
    unlink(pidFifoR);
    unlink(pidFifoW);


}

void breakLoop(int chan){
    pid_t pid = getpid();

    //FIFO para escrever para o monitor
    char pidFifoW[40];
    sprintf(pidFifoW, "tmp/%d", pid);
    mkfifo(pidFifoW,0666);     // cria um FIFO com o PID do processo;
    write(chan,&pid, sizeof(int)); // manda para o FIFO principal do servidor o nome do FIFO criado


    int fdFifoW = open(pidFifoW,O_WRONLY);
    int breakInt = 13;  // int pré-definido para o comando break
    

    write(fdFifoW,&breakInt,sizeof(int));
    unlink(pidFifoW);   // manda o int para o servidor

}

int main(int argc, char* argv[] ){
    char *modo, *flag;
    int chan = open("tmp/fifo", O_WRONLY);
    if (chan == -1) perror("erro ao abrir terminal de escrita");


    modo = argv[1];

    if ( strcmp(modo,"execute") == 0) {
        flag = argv[2];

        if ( strcmp(flag,"-u") == 0) {
            //printf("execute com opçao -u\n");
            execute(argv,chan);
        }
        else
        if ( strcmp(flag,"-p") == 0) {
            //printf("execute com opçao -p\n");
            executePIPE(argv,chan);
        }
        else {
            printf("flag inválida\n");
        }

    }
    else
    if ( strcmp(modo,"status") == 0) {
        //printf("Cliente: status\n");
        status(chan);

    }
    else

    if ( strcmp(modo,"stats-time") == 0) {
        int numPids = argc - 2;
        int pids[numPids];
        for (int i = 2; i < argc; i++){
            pids[i-2] = atoi(argv[i]); 
        }
        statsTime(chan,pids, numPids);
    }

    else
    if ( strcmp(modo,"stats-command") == 0) {
        char *command = argv[2];
        int numPids = argc - 3;
        int pids[numPids];
        for (int i = 3; i<argc; i++){
            pids[i-3] = atoi(argv[i]);
        }
        statsCommand(chan,command,pids,numPids);

            
    }

    else
    if ( strcmp(modo,"stats-uniq") == 0) {

        int numPids = argc - 2;
        int pids[numPids];
        for (int i = 2; i < argc; i++){
            pids[i-2] = atoi(argv[i]); 
        } 

        statsUniq(chan,pids,numPids);
    } else if(strcmp(modo,"break")==0){
        breakLoop(chan);
    }
    
    else {
        printf("⚠  Comando inválido ⚠ \n");
    }

    return 0;
}

