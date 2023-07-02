#include "hashtable.h"

guint g_pid_t_hash(gconstpointer key) {
    return (guint) *(pid_t*) key;
}

gboolean g_pid_t_equal(gconstpointer a, gconstpointer b) {
    return (*(pid_t*) a) == (*(pid_t*) b);
}

GHashTable* create_process_hash() {
    return g_hash_table_new_full(g_pid_t_hash, g_pid_t_equal, NULL, g_free);

}

void insert_process_hash (GHashTable* process_hash, Process* process){
    Process* p_ptr = g_malloc(sizeof(Process));
    memcpy(p_ptr, process, sizeof(Process));
    g_hash_table_insert(process_hash, &(p_ptr->pid), p_ptr);
}

void remove_process_hash (GHashTable* process_hash, pid_t* pid){
    g_hash_table_remove(process_hash, pid);
}

Process* get_process_hash (GHashTable* process_hash, pid_t pid){
    return (Process*) g_hash_table_lookup(process_hash, &pid);
}

void destroy_process_hash (GHashTable* process_hash){
    g_hash_table_destroy(process_hash);
}

void print_process(gpointer key, gpointer value, gpointer user_data){
    Process* process = (Process *)value;
    printf("Hash | PID do programa : %d\n", process->pid);
    printf("Hash | Nome do programa : %s\n", process->comando);
    printf("Hash | Inicio milisegundos: %ld\n", process->timestamp);
}
