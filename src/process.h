#ifndef PROCESS_H
#define PROCESS_H
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h> /* chamadas ao sistema: defs e decls essenciais */
#include <fcntl.h> 
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

typedef struct Process
{   
    pid_t pid;
    char comando[200];
    long timestamp;
}Process;

//API
Process new_Process(pid_t pid, char* comando, long timestamp);

#endif