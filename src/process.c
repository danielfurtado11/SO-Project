#include "process.h"

Process new_Process(pid_t pid, char* comando, long timestamp){

        Process p;
        p.pid = pid;
        if (strlcpy(p.comando, comando, sizeof(p.comando)) >= sizeof(p.comando))
            return (Process){0};

        p.timestamp = timestamp;

        return p;
}
