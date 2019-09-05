#include "data.h"

int RMDIR(char* path) {
  DIR *directory = opendir(path);
  if (directory == NULL) {
    printf("Could not open directory\n");
    return 0;
  }
  struct dirent *data;
  while ((data = readdir(directory)) != NULL) {
    if (data->d_type == 4 && strcmp(data->d_name, ".") != 0 && strcmp(data->d_name, "..") != 0) {
      // Directory
      char temp_path[255];
      strcpy(temp_path, path);
      strcat(temp_path, "/");
      strcat(temp_path, data->d_name);
      RMDIR(temp_path);
      rmdir(temp_path);

    } else if (data->d_type != 4) {
      // File

      char new_path[250];
      strcpy(new_path, path);
      strcat(new_path, "/");
      strcat(new_path, data->d_name);
      remove(new_path);
    }
  }
  return 1;
}

node* linked_list_insert(node *head, node *to_be_inserted, int updates){
    if(updates == 1){
      return to_be_inserted;
    }
    node *ptr;
    node *prev = ptr;
    for(ptr = head; ptr != NULL; ptr = ptr->next){
      prev = ptr;
    }
    prev->next = to_be_inserted;
    return head;
}

node* parse_update_file(char* update_path){
  //char* manifest_path = ".Manifest";
  int updates = 0;
  int fd = open(update_path, O_RDONLY);
  if (fd < 0) {
    printf("Can't open the manifest file\n");
    close(fd);
    return 0;
  }

  //create linked list out of all the U in update.
  node* head = malloc (sizeof(node));
  head = NULL;
  //this will store the server's .Manifest file
  //now to parse the file and actually store it inside

//read line by line
int version_manifest;
int not_end_of_file = 1; //if 1, end of file;
while(not_end_of_file){
  char c;
  int num = 1000;
  char* line_buffer = malloc(num);
  int count = 0;
  while((not_end_of_file = read(fd, &c, 1)) > 0 && c != '\n'){
    line_buffer[count] = c;
    count++;
    if (count > num - 5) {
      num*=2;
      line_buffer = realloc(line_buffer, num);
    }
  }
  line_buffer[count] = '\t';
  if (num == count) line_buffer = realloc(line_buffer, num + 2);
  line_buffer[++count] = '\0';
  if (strlen(line_buffer) < 1){
    break;
  }
  node* hashnode = malloc(sizeof(node));
  //hashnode = NULL;

  //now we parse through each line.
  //reminder: .Update file structure?
  //<M/A/D> <tab> <version_num> <tab> <filename> <tab> <filepath> <tab> <hashcode>

  if (strlen(line_buffer) < 2) continue;
  int i, j = 0, tabs_seen = 0;

  char token[PATH_MAX];
  for (i = 0; i < strlen(line_buffer); i++) {
    if (line_buffer[i] == '\t') {
      token[j] = '\0';
      tabs_seen++;
      if (tabs_seen == 1) {
        hashnode->update_id = token[0];
        updates++;
      } else if (tabs_seen == 2) {
        hashnode->version = atoi(token);
      } else if (tabs_seen == 3) {
        hashnode->filename = malloc(strlen(token) + 1);
        strcpy(hashnode->filename, token);
      } else if(tabs_seen == 4){
        hashnode->filepath = malloc(strlen(token) + 1);
        strcpy(hashnode->filepath, token);
      } else if(tabs_seen == 5){
        hashnode->code = malloc(strlen(token) + 1);
        strcpy(hashnode->code, token);
      } else break;
      j = 0;
      bzero(token, PATH_MAX);
      continue;
    }
    token[j] = line_buffer[i];
    j++;
  }

  //nodeInsert(hashnode, HashTable);
  head = linked_list_insert(head, hashnode, updates);

}

return head;
}

node* parse_commit_file(char* update_path){
  //char* manifest_path = ".Manifest";
  int updates = 0;
  int fd = open(update_path, O_RDONLY);
  if (fd < 0) {
    printf("Can't open the manifest file\n");
    close(fd);
    return 0;
  }

  //create linked list out of all the U in update.
  node* head = NULL;
  //head = NULL;
  //this will store the server's .Manifest file
  //now to parse the file and actually store it inside

//read line by line
int version_manifest;
int not_end_of_file = 1; //if 1, end of file;
while(not_end_of_file){
  char c;
  int num = 1000;
  char* line_buffer = malloc(num);
  int count = 0;
  while((not_end_of_file = read(fd, &c, 1)) > 0 && c != '\n'){
    line_buffer[count] = c;
    count++;
    if (count > num - 5) {
      num*=2;
      line_buffer = realloc(line_buffer, num);
    }
  }
  line_buffer[count] = '\t';
  if (num == count) line_buffer = realloc(line_buffer, num + 2);
  line_buffer[++count] = '\0';
  if (strlen(line_buffer) < 1){
    break;
  }
  node* hashnode = NULL;
  //bzero(hashnode, sizeof(node));

  //now we parse through each line.
  //reminder: update path looks like U tab version_num tab filepath

  if (strlen(line_buffer) < 2) continue;
  int i, j = 0, tabs_seen = 0;
  int update_detected = 0;
  char token[PATH_MAX];
  for (i = 0; i < strlen(line_buffer); i++) {
    if (line_buffer[i] == '\t') {
      token[j] = '\0';
      tabs_seen++;
      if (tabs_seen == 1) {
        if(token[0] == 'U'){
          update_detected = 1;
          hashnode = malloc(sizeof(node));
          updates++;
        }
      } else if (tabs_seen == 2) {
        if(update_detected){
          hashnode->version = atoi(token);
        }
      } else if (tabs_seen == 3) {
        if(update_detected){
          hashnode->filepath = malloc(strlen(token) + 1);
          strcpy(hashnode->filepath, token);
          hashnode->filename = malloc(strlen(token)+ 1);
          strcpy(hashnode->filename, get_name(token));
          break;
        }
      } else break;
      j = 0;
      bzero(token, PATH_MAX);
      continue;
    }
    token[j] = line_buffer[i];
    j++;
  }

  //nodeInsert(hashnode, HashTable);
  //head->filepath
  if (hashnode != NULL) head = linked_list_insert(head, hashnode, updates);

}
close(fd);
return head;
}

char* gethash(char* filepath){
    int fd = open(filepath, O_RDONLY);
  if (fd < 0) {
    printf("Incorrect Path Given... exiting ...\n");
    close(fd);
    return NULL;
  }

  struct stat stats;
  fstat(fd, &stats);
  int size = stats.st_size;
  char *buf = malloc(size+1);
  int n = read(fd, buf, size);

  unsigned char tmphash[SHA256_DIGEST_LENGTH];
  SHA256(buf, size, tmphash);
  unsigned char *hash = malloc (SHA256_DIGEST_LENGTH*2);

  int i;
  for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
   sprintf((char*)(hash+(i*2)), "%02x", tmphash[i]);
  }

  close(fd);
  return hash;
}


void make_list(char* filePath, node** hash_client) {
 DIR *directory = opendir(filePath);
 struct dirent* temp;
 if (directory == NULL) {
   fprintf(stderr, "Could not find the directory!\n");
   return;
 }

 while ((temp = readdir(directory)) != NULL) {
   if (temp->d_type == 4 && temp->d_name[0] != '.') {
     char* buffer = malloc(2+temp->d_reclen+strlen(filePath));
     strcpy(buffer, filePath);
     strcat(buffer, "/");
     strcat(buffer, temp->d_name);
     make_list(buffer, hash_client);
   }
   else if (temp->d_name[0] != '.') {

     int count = 3+strlen(filePath)+temp->d_reclen;
       char* buffer = malloc(count);
       strcpy(buffer, filePath);
       strcat(buffer, "/");
       strcat(buffer, temp->d_name);
       node *temp_node = malloc(sizeof(node));
       temp_node->version = 1;
       temp_node->code = malloc(SHA256_DIGEST_LENGTH*2 + 1);
       strcpy(temp_node->code, gethash(buffer));                   //hashcode
       temp_node->filepath = malloc(strlen(buffer) + 1);
       temp_node->next = NULL;
       strcpy(temp_node->filepath, buffer);                        //filepath
       temp_node->filename = malloc(temp->d_reclen + 1);
       strcpy(temp_node->filename, temp->d_name);                  //filename
       //linked_list_ptr->next = temp_node;
       //linked_list_ptr = temp_node;
       nodeInsert(temp_node, hash_client);
     }
   }
 }


node** createTable(){
   node** hash_client = malloc (150 * sizeof(node*));
   bzero(hash_client, sizeof(node*)*150);
   return hash_client;
}

int getkey(char *filename){
 int len = strlen(filename);
   int i = 0;
   int sum = 0;
   while(i < len){
       sum += (int)filename[i];
       i++;
   }
   int key = sum % 150;
   return key;
}

void nodeInsert(node* hashnode, node **table){
   int key = getkey(hashnode->filename);
   if(table[key] == NULL){
       table[key] = hashnode;
       return;
   }
   node* ptr = table[key];
   node* prev = table[key];
   while (ptr != NULL){
       prev = ptr;
       ptr = ptr->next;
   }
   prev->next = hashnode;
   return;

}

node** parse_manifest(char* manifest_path, int* version_num){

 //server-client interaction will happen here.
 //assumes that the .Manfiest was given.

 //char* manifest_path = ".Manifest";
 int fd = open(manifest_path, O_RDONLY);
 if (fd < 0) {
   printf("Can't open the manifest file\n");
   close(fd);
   return 0;
 }

 //next step is to create a hash_client (size 100), hashed to the name of the file
 node** hash_client = createTable();
 //this will store the server's .Manifest file
 //now to parse the file and actually store it inside

//read line by line
int line_number = 0;
int version_manifest;
int not_end_of_file = 1; //if 1, end of file;
while(not_end_of_file){
 char c;
 int num = 1000;
 char* line_buffer = malloc(num);
 int count = 0;
 while((not_end_of_file = read(fd, &c, 1)) > 0 && c != '\n'){
   line_buffer[count] = c;
   count++;
   if (count > num - 5) {
     num*=2;
     line_buffer = realloc(line_buffer, num);
   }
 }
 line_buffer[count] = '\t';
 if (num == count) line_buffer = realloc(line_buffer, num + 2);
 line_buffer[++count] = '\0';
 if (strlen(line_buffer) < 1){
   break;
 }
 if (line_number == 0){
   version_manifest = atoi(line_buffer);
   *version_num = version_manifest;
   line_number ++;
   continue;
 }

 node* hashnode = malloc(sizeof(node));

 //now we parse through each line.

 if (strlen(line_buffer) < 2) continue;
 int i, j = 0, tabs_seen = 0;
 char token[PATH_MAX];
 for (i = 0; i < strlen(line_buffer); i++) {
   if (line_buffer[i] == '\t') {
     token[j] = '\0';
     tabs_seen++;
     if (tabs_seen == 1) {
       hashnode->version = atoi(token);
     } else if (tabs_seen == 2) {
       hashnode->filename = malloc(strlen(token) + 1);
       strcpy(hashnode->filename, token);
     } else if (tabs_seen == 3) {
       hashnode->filepath = malloc(strlen(token) + 1);
       strcpy(hashnode->filepath, token);
     } else if (tabs_seen == 4) {
       hashnode->code = malloc(strlen(token) + 1);
       strcpy(hashnode->code, token);
       break;
     } else break;
     j = 0;
     bzero(token, PATH_MAX);
     continue;
   }
   token[j] = line_buffer[i];
   j++;
 }

 nodeInsert(hashnode, hash_client);

}
close(fd);
return hash_client;
}

void printHash(node** HashTable){
  int i;
  for (i = 0; i < 150; i++){
    if(HashTable[i] == NULL) continue;
    node* ptr = HashTable[i];
    for(;ptr != NULL; ptr = ptr->next){
      printf("%d\t%s\t%s\t%s\n", ptr->version, ptr->filename, ptr->filepath, ptr->code);
    }
  }
}

int make_manifest(node** HashTable, char* path, int version) {
  int fd = open(path, O_WRONLY | O_CREAT, 0700);
  if (fd < 0) {
    printf("Could not create Manifest\n");
    close(fd);
    return 0;
  }
  char v_num[250];
  sprintf(v_num, "%d\n", version);
  write(fd, v_num, strlen(v_num));
  int i;
  for (i = 0; i < 150; i++){
    if(HashTable[i] == NULL) continue;
    node* ptr = HashTable[i];
    for(;ptr != NULL; ptr = ptr->next){
      char v[250];
      sprintf(v, "%d\t", ptr->version);
      write(fd, v, strlen(v));
      write(fd, ptr->filename, strlen(ptr->filename));
      write(fd, "\t", 1);
      write(fd, ptr->filepath, strlen(ptr->filepath));
      write(fd, "\t", 1);
      write(fd, ptr->code, strlen(ptr->code));
      write(fd, "\n", 1);
    }
  }
  close(fd);
  return 1;
}

char* get_name(char* file_path) {
  int index = -1, i;
  for (i = 0; i < strlen(file_path); i++) {
    if (file_path[i] == '/') index = i;
  }
  return file_path+index+1;
}


node* search_node(node *live_file, node** hash_client){
 int key = getkey(live_file->filename);
 node *pointer = hash_client[key]; //of hash_client
 while(pointer != NULL){
   if (strcmp(live_file->filename, pointer->filename)==0){
     if(strcmp(live_file->filepath, pointer->filepath)==0){
       //file exists
       return pointer;
     }
   }
   pointer = pointer->next;
 }
 node *temp = malloc(sizeof(node));
 temp->filename = "does not exist";
 return temp;
}

node* delete(char* filepath, node *front){
   // int size = front->size;
    node *head = front;
    node *ptr;
    node *prev = front;

    if (front == NULL){
        return NULL;
    }

    if (strcmp(filepath, front->filepath)==0 && front->next == NULL){
        head = NULL;
        return NULL;
    }

    if (strcmp(filepath, front->filepath)==0){
        head = front->next;
        return head;
    }
    for (ptr = front; ptr != NULL; ptr = ptr->next){
        if (strcmp(filepath, ptr->filepath)!=0){
            prev = ptr;
            continue;
        } else if (strcmp(filepath, front->filepath)==0){
            prev->next = ptr->next;
            return head;
        }
    }
    return head;
}
