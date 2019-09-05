#ifndef DATA_ST
#define DATA_ST

typedef struct node{
  int version;
  unsigned char *code;
  char *filepath;
  char *filename;
  char update_id;
  struct node* next;
}node;

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <dirent.h>
#include <netdb.h>
#include <signal.h>
#include <openssl/sha.h>

int RMDIR(char* path);
node* linked_list_insert(node *head, node *to_be_inserted, int updates);
node* parse_update_file(char* update_path);
node* parse_commit_file(char* update_path);
node** parse_manifest(char* manifest_path, int* version_num);
char* gethash(char* filepath);
void make_list(char* filePath, node** hash_client);
node** createTable();
void nodeInsert(node* hashnode, node **table);
void printHash(node** HashTable);
int make_manifest(node** HashTable, char* path, int version);
char* get_name(char* file_path);
node* search_node(node *live_file, node** hash_client);
node* delete(char* filepath, node *front);

#endif
