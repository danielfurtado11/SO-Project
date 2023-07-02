#ifndef HASHTABLE_H
#define HASHTABLE_H
#include <glib.h>
#include "process.h"


typedef struct process_hash{
  GHashTable* process_hash;
}process_hash;

guint g_pid_t_hash(gconstpointer key);

gboolean g_pid_t_equal(gconstpointer a, gconstpointer b);

GHashTable* create_process_hash ();

void insert_process_hash (GHashTable* process_hash, Process *process);

void remove_process_hash (GHashTable* process_hash, pid_t* pid);

Process* get_process_hash (GHashTable* process_hash, pid_t pid);

void destroy_process_hash (GHashTable* process_hash);

void print_process(gpointer key, gpointer value, gpointer user_data);

#endif
