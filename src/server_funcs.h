#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "process.h"
#include "hashtable.h"


int executeU (int fdFifo, GHashTable* process_hash);
int executeUF (int fdFifo, GHashTable* process_hash, char* path);
void statusExe(int fdFifoW, GHashTable* process_hash);
long statsTimeExe(int pid);
int statsCommandExe(int pid, char* command);
char** statsUniq(int pid);
int  verificaComandos(char* comando, char ** comFinal);
int sizeArray(char **array);