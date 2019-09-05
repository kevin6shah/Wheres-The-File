#include "data.h"

int client_socket;

void handle() {
  printf("\n\nClient was interrupted... killing process now!\n");
  kill(getpid(), SIGTERM);
}

int dup_dir(char* old_path, char* dup_path, int check) {
  if (check == 0) {
    char path[255];
    strcpy(path, "dup_");
    strcat(path, old_path);
    mkdir(path, 0700);
    return dup_dir(old_path, path, 1);
  }
  DIR *directory = opendir(old_path);
  if (directory == NULL) {
    printf("Could not open directory\n");
    return 0;
  }
  struct dirent *data;
  while ((data = readdir(directory)) != NULL) {
    if (data->d_type == 4 && strcmp(data->d_name, ".") != 0 && strcmp(data->d_name, "..") != 0) {
      // Directory
      char temp_path[255], actual_path[255];
      strcpy(actual_path, old_path);
      strcpy(temp_path, dup_path);
      strcat(actual_path, "/");
      strcat(temp_path, "/");
      strcat(actual_path, data->d_name);
      strcat(temp_path, data->d_name);

      mkdir(temp_path, 0700);
      dup_dir(actual_path, temp_path, 1);

    } else if (data->d_type != 4) {
      // File
      char path[255];
      char path2[255];
      strcpy(path,old_path);
      strcpy(path2,dup_path);
      strcat(path, "/");
      strcat(path2, "/");
      strcat(path, data->d_name);
      strcat(path2, data->d_name);
      int fd = open(path, O_RDONLY);
      int newfd = open(path2, O_WRONLY | O_CREAT, 0700);
      struct stat stats;
      stat(path, &stats);
      char *buffer = malloc(stats.st_size);
      read(fd, buffer, stats.st_size);
      write(newfd, buffer, stats.st_size);
      close(fd);
      close(newfd);
    }
  }
}

int send_file(char* client_path) {
  int fd = open(client_path, O_RDONLY);
  if (fd < 0) {
    printf("Incorrect path entered\n");
    close(fd);
    return 0;
  }

  struct stat stats;
  stat(client_path, &stats);
  int buf_size = stats.st_size;
  char size[255];
  sprintf(size, "%d", buf_size);
  write(client_socket, size, strlen(size));
  write(client_socket, "$TOKEN", strlen("$TOKEN"));

  char *buffer = malloc(buf_size + 1);
  int n = read(fd, buffer, buf_size);
  if (n < 0) {
    printf("Error Reading\n");
    close(fd);
    return 0;
  }
  n = write(client_socket, buffer, buf_size);
  if (n < 0) {
    printf("Error Writing\n");
    close(fd);
    return 0;
  }

  write(client_socket, "$TOKEN", strlen("$TOKEN"));
  close(fd);
  return 1;
}

int configure(char* ip, char* host) {
  int fd;
  if ((fd = open("./.configure", O_RDONLY)) >= 0) {
    // Already Configured
    printf("IP Address and Host are already configured!\n");
    close(fd);
    return 0;
  } else {
    // Not configured
    fd = open("./.configure", O_WRONLY | O_CREAT, 0700);
    if (fd < 0) {
      printf("Could not create a config file!\n");
      close(fd);
      return 0;
    }
    write(fd, ip, strlen(ip));
    write(fd, "\t", 1);
    write(fd, host, strlen(host));
    printf(".configure file was created sucessfully!\n");
  }
  close(fd);
  return 1;
}

int exists(char* path, char* project_name) {
  DIR *directory = opendir(path);
  if (directory == NULL) {
    printf("Could not open directory\n");
    return;
  }
  struct dirent *data;
  while ((data = readdir(directory)) != NULL) {
    if (data->d_type == 4 && strcmp(data->d_name, ".") != 0 && strcmp(data->d_name, "..") != 0) {
      if (strcmp(data->d_name, project_name) == 0) return 1;
    }
  }
  closedir(directory);
  return 0;
}

int recieve() {
  char token[255];
  char c;
  int ready = 0, i = 0, size = 0, fd;
  while (1) {
    int n = read(client_socket, &c, 1);
    if (n <= 0) break;
    token[i] = c;
    if (i > 5 && ready == 0 && token[i] == 'N' && token[i-1] == 'E' &&
    token[i-2] == 'K' && token[i-3] == 'O' && token[i-4] == 'T' && token[i-5] == '$') {
      token[i-5] = '\0';
      ready = 1;
    }
    i++;
    if (ready == 1) {
      // ON SUCCESS SERVER SENDS $***$
      if (strcmp(token, "$***$") == 0) break;

      // ON FAILURE SERVER SENDS $***$
      if (strcmp(token, "$FFF$") == 0) return 0;

      // Token is ready
      if (size == 1) {
        // Size of A File -> We know
        size = atoi(token);
        if (size == 0) {
          printf("An error occured with size\n");
          return 0;
        }
        char *buffer = malloc(size+1);
        char tmp;
        int index = 0;
        do {
          int n = read(client_socket, &tmp, 1);
          if (n < 0) {
            printf("An error occured with read\n");
            return 0;
          }
          buffer[index] = tmp;
          index++;
        } while(index < size);

        char temp[6];
        int n = read(client_socket, temp, 6);
        if (n < 0) {
          printf("An error occured with read\n");
          return 0;
        }
        n = write(fd, buffer, size);
        if (n < 0) {
          printf("An error occured with write\n");
          return 0;
        }
        size = 0;
      } else if (token[0] == '#') {
        // Directory
        int n = mkdir(token+1, 0700);
        if (n < 0) {
          printf("Error trying to make '%s' directory\n", token+1);
          return 0;
        }
      } else if (token[0] == '*') {
        // File
        fd = open(token+1, O_WRONLY | O_CREAT, 0700);
        if (fd < 0) {
          printf("Error trying to make '%s' file\n", token+1);
          close(fd);
          return 0;
        }
        size = 1;
      } else {
        printf("Structural Damage\n");
        return 0;
      }
      bzero(token, 255);
      i = 0;
      ready = 0;
    }
  }

  return 1;
}

int checkout(char* project_name) {
  if (exists("./", project_name) || (!connect_client())) {
    return 0;
  }
  write(client_socket, "checkout:", strlen("checkout:"));
  write(client_socket, project_name, strlen(project_name));
  write(client_socket, "$TOKEN", strlen("$TOKEN"));
  if (!recieve()) {
    printf("There occured an error checking out...\n");
    return 0;
  }
  printf("Checkout was successful...\n");
  return 1;
}

int create(char* project_name) {
  if (exists("./", project_name) || (!connect_client())) {
    return 0;
  }
  write(client_socket, "create:", strlen("create:"));
  write(client_socket, project_name, strlen(project_name));
  write(client_socket, "$TOKEN", strlen("$TOKEN"));
  if (!recieve()) {
    printf("There occured an error creating the project...\n");
    return 0;
  }
  printf("Create was successful...\n");
  return 1;
}

int update_helper(char* project_name){
  node** live_hash = createTable();
  make_list(project_name, live_hash);
  int server_manifest_version, client_manifest_version;
  node **hash_server = parse_manifest(".Manifest", &server_manifest_version);
  char path[255];
  strcpy(path, project_name);
  strcat(path, "/.Manifest");
  node **hash_client = parse_manifest(path, &client_manifest_version);
  char update_path[255];
  strcpy(update_path, project_name);
  strcat(update_path, "/.Update");

  int fd = open(update_path, O_WRONLY | O_CREAT, 0700);
  if (fd < 0) {
    printf("Error creating the manifest file\n");
    close(fd);
    return 0;
  }
  int i;

  int error = 0;
  for (i = 0; i < 150; i++){
    if(hash_client[i] == NULL) continue;
    node* file_client = hash_client[i];
    for(;file_client != NULL; file_client = file_client->next){
      int completed_if_else_command = 0;
      node* file_server = search_node(file_client, hash_server);
      node *ptr = search_node(file_client, live_hash);
      if(strcmp(ptr->filename, "does not exist") ==0){
        printf("file (%s) has been removed from the local project\n", file_client->filepath);
      }

      if (strcmp(file_server->filename, "does not exist") ==0){
        //in client but not in server
        if (server_manifest_version == client_manifest_version){
          //upload
          completed_if_else_command = 1;
          printf("U:\t%s\n", file_client->filepath);
        } else{
          //delete
          completed_if_else_command = 1;
          printf("D:\t%s\n", file_client->filepath);
          write(fd, "D", strlen("D"));
          write(fd, "\t", 1);
          char* version_num_buff = malloc(10);
          int tmp = file_client->version;
          sprintf(version_num_buff,"%d", tmp);
          write(fd, version_num_buff, strlen(version_num_buff));
          write(fd, "\t", 1);
          write(fd, file_client->filename, strlen(file_client->filename));
          write(fd, "\t", 1);
          write(fd, file_client->filepath, strlen(file_client->filepath));
          write(fd, "\t", 1);
          write(fd, file_client->code, strlen(file_client->code));
          write(fd, "\n", 1);

        }
      } else if(strcmp(file_server->filename, "does not exist") !=0){
          //file exists in both the server and client manifests
         if(server_manifest_version == client_manifest_version && strcmp(file_server->code, ptr->code) != 0) {
          //upload
          completed_if_else_command = 1;
          printf("U:\t%s\n", file_client->filepath);
        } else if(server_manifest_version != client_manifest_version && file_client->version != file_server->version){
            //modify
            completed_if_else_command = 1;
            printf("M:\t%s\n", file_client->filepath);
            write(fd, "M", strlen("M"));
            write(fd, "\t", 1);
            char* version_num_buff = malloc(10);
            sprintf(version_num_buff,"%d", file_server->version);
            write(fd, version_num_buff, strlen(version_num_buff));
            write(fd, "\t", 1);
            write(fd, file_client->filename, strlen(file_client->filename));
            write(fd, "\t", 1);
            write(fd, file_client->filepath, strlen(file_client->filepath));
            write(fd, "\t", 1);
            write(fd, file_server->code, strlen(file_server->code));
            write(fd, "\n", 1);
        } else if(server_manifest_version == client_manifest_version && file_client->version == file_server->version && strcmp(file_client->code, file_server->code)==0){
            completed_if_else_command = 1;
        }

      } else if (!completed_if_else_command){
        error = 1;
        printf("conflicting files :\t%s\n", file_client->filepath);


      }
    }
  }


  //now to see what files from the server are not in the client.

    int j;
    for (j = 0; j < 150; j++){
      if(hash_server[j] == NULL) continue;
      node* ptr = hash_server[j];
      for(;ptr != NULL; ptr = ptr->next){
        node* test = search_node(ptr, hash_client);
        if(strcmp(test->filename, "does not exist") == 0){
          printf("A:\t%s\n", ptr->filepath);
          write(fd, "A", strlen("A"));
          write(fd, "\t", 1);
          char* version_num_buff = malloc(10);
          sprintf(version_num_buff,"%d", ptr->version);
          write(fd, version_num_buff, strlen(version_num_buff));
          write(fd, "\t", 1);
          write(fd, ptr->filename, strlen(ptr->filename));
          write(fd, "\t", 1);
          write(fd, ptr->filepath, strlen(ptr->filepath));
          write(fd, "\t", 1);
          write(fd, ptr->code, strlen(ptr->code));
          write(fd, "\n", 1);

        }
      }
    }
    if (error){
      remove(update_path);
      close(fd);
      return 0;
    }
    close(fd);
    return 1;
}

int update(char* project_name) {
  if (!connect_client()) {
    return 0;
  }
  write(client_socket, "update:", strlen("update:"));
  write(client_socket, project_name, strlen(project_name));
  write(client_socket, "$TOKEN", strlen("$TOKEN"));
  if (!recieve()) {
    printf("There occured an error recieving the .Manifest from the server...\n");
    return 0;
  }

  if (!update_helper(project_name)) {
   printf("Update failed...\n");
   return 0;
  }

  remove(".Manifest");

  printf("Update was successful...\n");
  return 1;
}

int upgrade_helper(char* project_name) {
  char *update_path = malloc(1000);
  strcpy(update_path, project_name);
  strcat(update_path, "/.Update");
  //check if there actually exists a .Update file
  int fd = open(update_path, O_RDONLY);
  if (fd < 0) {
    printf(".Update file does not exist dawg... Try updating first!\n");
    close(fd);
    return 0;
  }
  close(fd);

    int pd = open(".Manifest", O_RDONLY);
    if (pd < 0) {
        printf("cannot open server .Manifest file\n");
        close(pd);
        return 0;
    }
    close(pd);




  char manifest_path[255];
  strcpy(manifest_path, project_name);
  strcat(manifest_path, "/.Manifest");
  int j,n;
  //j holds server's manifest version
  node** server_manifest= parse_manifest(".Manifest", &j);
  node **manifest_data = parse_manifest(manifest_path, &n);
  remove(".Manifest");

  //Parse through .Update file, create linked list of operations

  node *head = parse_update_file(update_path);
  remove(update_path);
  //traverse .Update and implement changes
  //get mostrecentversionnum of project
  node *ptr;
  for (ptr = head; ptr != NULL; ptr = ptr->next){
    if(ptr->update_id == 'D'){
        manifest_data[getkey(ptr->filename)] = delete(ptr->filepath,manifest_data[getkey(ptr->filename)]);
        remove(ptr->filepath);
    } else if(ptr->update_id == 'M'){
      //fetch ptr->filepath (specific file) from server
      //servers path : .server_repo/projectname/(most recent version)/filepath
      remove(ptr->filepath);
      write(client_socket, ptr->filepath, strlen(ptr->filepath));
      write(client_socket, "$TOKEN", strlen("$TOKEN"));
      if (!recieve()) {
        printf("There occured an error recieving '%s' from the server...\n", ptr->filepath);
        return 0;
      }
      node *temp = search_node(ptr, manifest_data);
      node *server_node = search_node(ptr, server_manifest);
      bzero(temp->code, strlen(temp->code));
      strcpy(temp->code, gethash(temp->filepath));
      temp->version = server_node->version;


    } else if(ptr->update_id == 'A'){
      write(client_socket, ptr->filepath, strlen(ptr->filepath));
      write(client_socket, "$TOKEN", strlen("$TOKEN"));
      if (!recieve()) {
        printf("There occured an error recieving '%s' from the server...\n", ptr->filepath);
        return 0;
      }
      node *server_node = search_node(ptr, server_manifest);
      ptr->version = server_node->version;
      nodeInsert(ptr, manifest_data);

    }
  }
  write(client_socket, "$***$$TOKEN", strlen("$***$$TOKEN"));
  remove(manifest_path);
  make_manifest(manifest_data, manifest_path, j);
  return 1;
}

int upgrade(char* project_name){
  if (!connect_client()) {
    return 0;
  }
  write(client_socket, "upgrade:", strlen("upgrade:"));
  write(client_socket, project_name, strlen(project_name));
  write(client_socket, "$TOKEN", strlen("$TOKEN"));
  if (!recieve()) {
    printf(".Manifest file was not recieved from the server\n");
    return 0;
  }

  if (upgrade_helper(project_name)) {
    printf("Upgrade was successful...\n");
    return 1;
  }
  return 0;
}

int currentversion(char* project_name){
  if (!connect_client()) {
    return 0;
  }
  write(client_socket, "currentversion:", strlen("currentversion:"));
  write(client_socket, project_name, strlen(project_name));
  write(client_socket, "$TOKEN", strlen("$TOKEN"));

  printf("\nCurrent Version: ");
  char c;
  char token[255];
  int i = 0;
  while (read(client_socket, &c, 1) > 0) token[i++] = c;
  if (i > 4 && token[i] == '$' && token[i-1] == 'F'
  && token[i-2] == 'F' && token[i-3] == 'F' && token[i-4] == '$') return 0;
  else token[i] = '\0';
  printf("%s\n", token);
  return 1;
}

int rollback(char* project_name, char* version_num){
  if (!connect_client()) {
    return 0;
  }
  write(client_socket, "rollback:", strlen("rollback:"));
  write(client_socket, project_name, strlen(project_name));
  write(client_socket, ":", 1);
  write(client_socket, version_num, strlen(version_num));
  write(client_socket, "$TOKEN", strlen("$TOKEN"));

  char status[255];
  int n = read(client_socket, status, 255);
  status[5] = '\0';
  if (strcmp(status, "$FFF$") == 0) return 0;
  printf("Rollback was successful...\n");
  return 1;
}

void senddir_helper(char* client_path, char* server_path) {
  DIR *directory = opendir(client_path);
  if (directory == NULL) {
    printf("Could not open directory\n");
    return;
  }
  struct dirent *data;
  while ((data = readdir(directory)) != NULL) {
    if (data->d_type == 4 && strcmp(data->d_name, ".") != 0 && strcmp(data->d_name, "..") != 0) {
      // Directory
      char temp_path[255];
      strcpy(temp_path, client_path);
      strcat(temp_path, "/");
      strcat(temp_path, data->d_name);

      char new_path[255];
      strcpy(new_path, server_path);
      strcat(new_path, "/");
      strcat(new_path, data->d_name);

      write(client_socket, "#", strlen("#"));
      write(client_socket, new_path, strlen(new_path));
      write(client_socket, "$TOKEN", strlen("$TOKEN"));

      senddir_helper(temp_path, new_path);
    } else if (data->d_type != 4) {
      // File
      write(client_socket, "*", strlen("*"));
      write(client_socket, server_path, strlen(server_path));
      write(client_socket, "/", strlen("/"));
      write(client_socket, data->d_name, strlen(data->d_name));
      write(client_socket, "$TOKEN", strlen("$TOKEN"));

      char new_path[250];
      strcpy(new_path, client_path);
      strcat(new_path, "/");
      strcat(new_path, data->d_name);
      if (!send_file(new_path)) {
        printf("Error: Could not send '%s' to the server\n", new_path);
        write(client_socket, "$FFF$$TOKEN", strlen("$FFF$$TOKEN"));
      }
    }
  }
}

int senddir(char* project_name, char* version) {
  write(client_socket, "#", strlen("#"));
  char server_path[255] = ".server_repo/";
  strcat(server_path, project_name+4);
  strcat(server_path, "/version");
  strcat(server_path, version);
  write(client_socket, server_path, strlen(server_path));
  write(client_socket, "$TOKEN", strlen("$TOKEN"));
  senddir_helper(project_name, server_path);
  write(client_socket, "$***$$TOKEN", strlen("$***$$TOKEN"));
  printf("Directory sent successfully...\n");
  return 1;
}

int push_helper(char* project_name){
  //duplicate the project Directory
  dup_dir(project_name, NULL, 0);
  //go to duplicate project directory and access the .Manifest file
  char *dup_manifest_path = malloc(strlen(project_name) + strlen("/.Manifest") + strlen("dup_") + 1);
  strcpy(dup_manifest_path, "dup_");
  strcat(dup_manifest_path, project_name);
  strcat(dup_manifest_path, "/.Manifest");
  char *dup_project_name = malloc(strlen(project_name) + strlen("dup_") + 1);
  strcpy(dup_project_name, "dup_");
  strcat(dup_project_name, project_name);
  char *commit_file_path = malloc(strlen(project_name) + strlen("/.Commit") + 1);
  strcpy(commit_file_path, project_name);
  strcat(commit_file_path, "/.Commit");

  int fd = open(commit_file_path, O_RDONLY);
  if (fd < 0) {
    printf("Try committing first before you push\n");
    close(fd);
    return 0;
  }
  close(fd);

  //hash that jawn
  int version_num_manifest;
  node **client_push = parse_manifest(dup_manifest_path, &version_num_manifest);
  //compare the files from .Manifest and dup_directory, delete uneccessarry files (make_list)
  node **current_status = createTable();
  make_list(project_name, current_status); //not of the dup file but the actual one
  int i;
  for (i = 0; i < 150; i++){
    if(current_status[i] == NULL) continue;
    node* ptr = current_status[i];
    for(;ptr != NULL; ptr = ptr->next){
      node *temp = search_node(ptr, client_push);
      if (strcmp(temp->filename, "does not exist")==0){ //file does not exist in the Manifest
        char *stoopid = malloc (strlen(ptr->filepath) + 5);
        strcpy(stoopid,"dup_");
        strcat(stoopid, ptr->filepath);
        remove(stoopid);
      }
    }
  }

  //go through the .Commit file, and check for U(pdated) files, and take fileversion and hashcode
  node *head = parse_commit_file(commit_file_path);
  node *list_ptr;
  for(list_ptr = head; list_ptr != NULL; list_ptr = list_ptr->next){
    node *temp = search_node(list_ptr, client_push);
    temp->version = list_ptr->version;
    bzero(temp->code, strlen(temp->code));
    strcpy(temp->code, gethash(list_ptr->filepath));

  }
  //   the .Manifest with it
  remove(dup_manifest_path);
  make_manifest(client_push, dup_manifest_path, ++version_num_manifest);

  //finally update .Manifest version and replace the client one
  return 1;
  //send entire directory to be copied into version(manifest#) project.

}

int destroy(char* project_name) {
  if (!connect_client()) {
    return 0;
  }
  write(client_socket, "destroy:", strlen("destroy:"));
  write(client_socket, project_name, strlen(project_name));
  write(client_socket, "$TOKEN", strlen("$TOKEN"));

  char status[255];
  int n = read(client_socket, status, 255);
  status[5] = '\0';
  if (strcmp(status, "$FFF$") == 0) return 0;
  printf("Destroy was successful...\n");
  return 1;
}

int history(char* project_name) {
  if (!connect_client()) {
    return 0;
  }
  write(client_socket, "destroy:", strlen("destroy:"));
  write(client_socket, project_name, strlen(project_name));
  write(client_socket, "$TOKEN", strlen("$TOKEN"));

  char *buf = malloc(2000);
  int n = read(client_socket, buf, 2000);
  buf[n] = '\0';
  printf("%s\n", buf);
  printf("History was successful...\n");
  return 1;
}

int push(char* project_name) {
  if (!connect_client()) {
    return 0;
  }
  write(client_socket, "push:", strlen("push:"));
  write(client_socket, project_name, strlen(project_name));
  write(client_socket, "$TOKEN", strlen("$TOKEN"));
  char vp[255];
  char c;
  int i = 0;
  while (read(client_socket, &c, 1) > 0 && c != '$') {
    vp[i] = c;
    i++;
  }
  vp[i] = '\0';
  int version = atoi(vp);
  version++;
  bzero(vp, i);
  sprintf(vp, "%d", version);

  push_helper(project_name);
  char path[255] = "dup_";
  strcat(path, project_name);
  senddir(path, vp);
  RMDIR(project_name);
  rmdir(project_name);
  rename(path, project_name);

  bzero(path, 255);
  strcpy(path, project_name);
  strcat(path, "/.Commit");
  remove(path);

  char status[255];
  int n = read(client_socket, status, 255);
  status[5] = '\0';
  if (strcmp(status, "$FFF$") == 0) return 0;

  printf("Push was successful...\n");
  return 1;
}

int commit_helper(char* project_name) {
  int server_manifest_version, client_manifest_version;
  node **hash_server = parse_manifest(".Manifest", &server_manifest_version);
  remove(".Manifest");
  char path[255];
  strcpy(path, project_name);
  strcat(path, "/.Manifest");
  node **hash_client = parse_manifest(path, &client_manifest_version);

  if (server_manifest_version != client_manifest_version){
    printf("Versions do not match, please update local project\n");
    return 0;
  }
  node **live_hash = createTable();
  make_list(project_name, live_hash);
  //create a linked list which stores all the commits needed
  node *commit_list = malloc(sizeof(node));

  char temp_commit_path[255];
  strcpy(temp_commit_path, project_name);
  strcat(temp_commit_path, "/.Commit");
  int i;
  int fd = open(temp_commit_path, O_WRONLY | O_CREAT, 0700);
  if (fd < 0) {
    printf("Error creating the commit file\n");
    close(fd);
    return 0;
  }
  for (i = 0; i < 150; i++){
    if(hash_client[i] == NULL) continue;
    node* ptr = hash_client[i];
    for(;ptr != NULL; ptr = ptr->next){
      //temp holds the node to the .Manifest of live hash
      node* temp = search_node(ptr, live_hash);
      node* server_node = search_node(ptr, hash_server);

      if (strcmp(server_node->filename, "does not exist")==0){
        //do nothing, acceptable
        //file not in server yet, needs to be added
        // |change|<tab>|filepath|<newline>  change is either A=add or D=delete
        write(fd,"A",1);
        write(fd,"\t",1);
        write(fd, ptr->filepath, strlen(ptr->filepath));
        write(fd,"\n",1);

      } else if (strcmp(server_node->code, ptr->code)!=0 && server_node->version >= ptr->version){
        //error lol
        printf("client must synch with the repository before committing changes\n");
        remove(temp_commit_path);
      }

      if(strcmp("does not exist", temp->filename)==0){
        //file does not exist in local project but exists in .Manifest huhuh
        printf("file (%s) does not exist in the local project\n", ptr->filepath);
      } else{
          if(strcmp(ptr->code, temp->code) != 0){
            //local file is different than file referenced in .manifest
            //write  |change <tab> incremented version number <tab> filepath <tab> updated hashedcode <newline> |
            int cversion = ptr->version + 1;
            char* version_buffer = malloc(100);
            sprintf(version_buffer, "%d", cversion);
            write(fd,"U",1);
            write(fd,"\t",1);
            write(fd, version_buffer, strlen(version_buffer));
            write(fd,"\t",1);
            write(fd, ptr->filepath, strlen(ptr->filepath));
            write(fd,"\n",1);
          }
        }
      }
    }

      int j;
      for (j = 0; j < 150; j++){
        if(hash_server[j] == NULL) continue;

        node* ptr = hash_server[j];
        for(;ptr != NULL; ptr = ptr->next){
          node* test = search_node(ptr, hash_client);
          if(strcmp(test->filename, "does not exist") == 0){
            //file exist in the server but not in client
            //has to be deleted
            write(fd,"D",1);
            write(fd,"\t",1);
            write(fd, ptr->filepath, strlen(ptr->filepath));
            write(fd,"\n",1);
          }
        }
      }
      close(fd);
    return 1;
}

int commit(char* project_name){
  char temp_path[255];
  strcpy(temp_path, project_name);
  strcat(temp_path, "/.Update");
  if (!connect_client()) {
    return 0;
  }
  int tempfd = open(temp_path, O_RDONLY);
  if (tempfd >= 0) {
    close(tempfd);
    return 0;
  }
  close(tempfd);
  write(client_socket, "commit:", strlen("commit:"));
  write(client_socket, project_name, strlen(project_name));
  write(client_socket, "$TOKEN", strlen("$TOKEN"));

  if (!recieve()) {
    printf("There occured an error recieving the .Manifest from the server...\n");
    return 0;
  }

  if (!commit_helper(project_name)) return 0;

  write(client_socket, "*.server_repo/", strlen("*.server_repo/"));
  write(client_socket, project_name, strlen(project_name));
  write(client_socket, "/version", strlen("/version"));
  char version[255];
  char c;
  while (read(client_socket, &c, 1) > 0 && c != '$') {
    char str[2];
    sprintf(str, "%c", c);
    write(client_socket, str, strlen(str));
  }
  write(client_socket, "/.Commit", strlen("/.Commit"));
  write(client_socket, "$TOKEN", strlen("$TOKEN"));
  bzero(temp_path, 255);
  strcpy(temp_path, project_name);
  strcat(temp_path, "/.Commit");
  if (send_file(temp_path)) {
    printf("Commit succeeded!\n");
    return 1;
  }
  return 0;
}

int add(char* project_name, char* file_old_path) {
  int client_manifest_version;
  char path[255];
  strcpy(path, project_name);
  strcat(path, "/.Manifest");
  node **hash_client = parse_manifest(path, &client_manifest_version);

  char* file_path = malloc(strlen(project_name) + strlen(file_old_path) + 2);
  strcpy(file_path, project_name);
  strcat(file_path, "/");
  strcat(file_path, file_old_path);

  node* result;
  node* tmp = malloc(sizeof(node));
  tmp->filepath = malloc(255);
  strcpy(tmp->filepath, file_path);
  tmp->filename = malloc(255);
  strcpy(tmp->filename, get_name(file_path));
  result = search_node(tmp, hash_client);

  if (strcmp(result->filename, "does not exist") != 0) {
    //Path exists on the manifest
    strcpy(result->code, gethash(file_path));
    printf("Warning: The file already exists...\n.Manifest has been updated with new hash\n");
  } else {
    //New path to add
    node *temp = malloc(sizeof(node));
    temp->version = 1;
    temp->filename = malloc(strlen(get_name(file_path)) + 1);
    strcpy(temp->filename, get_name(file_path));
    temp->filepath = malloc(strlen(file_path) + 1);
    strcpy(temp->filepath, file_path);
    temp->code = malloc (SHA256_DIGEST_LENGTH*2);
    if (gethash(file_path) == NULL) return 0;
    strcpy(temp->code, gethash(file_path));
    nodeInsert(temp, hash_client);
  }
  remove(path);
  if (make_manifest(hash_client, path, client_manifest_version)) {
    printf("Sucessfully added '%s'\n", file_old_path);
    return 1;
  }
  return 0;
}

int remuuv(char* project_name, char* file_old_path){
  int client_manifest_version;
  char path[255];
  strcpy(path, project_name);
  strcat(path, "/.Manifest");
  node **hash_client = parse_manifest(path, &client_manifest_version);

  char* file_path = malloc(strlen(project_name) + strlen(file_old_path) + 2);
  strcpy(file_path, project_name);
  strcat(file_path, "/");
  strcat(file_path, file_old_path);

  node* result;
  node* tmp = malloc(sizeof(node));
  tmp->filepath = malloc(255);
  strcpy(tmp->filepath, file_path);
  tmp->filename = malloc(255);
  strcpy(tmp->filename, get_name(file_path));
  result = search_node(tmp, hash_client);

  if (strcmp(result->filename, "does not exist") != 0) {
    //Path exists on the manifest --> Remove it
    hash_client[getkey(result->filename)] = delete(result->filepath,hash_client[getkey(result->filename)]);
  } else {
    //Path does not exist --> Error
    printf("Error: Could not remove the file at '%s'\n", file_path);
    return 0;
  }
  remove(path);
  if (make_manifest(hash_client, path, client_manifest_version)) {
    printf("Sucessfully removed '%s'\n", file_old_path);
    return 1;
  }
  return 0;
}

int connect_client() {
  int fd = open("./.configure", O_RDONLY);
  if (fd < 0) {
    printf("The client is not configured... Please try configuring first!\n");
    close(fd);
    return 0;
  }
  // Gets the IP and Host from the Config file
  char ip[250], host[250];
  char temp;
  int i = 0, dot = 0, valid = 1;
  while (read(fd, &temp, 1) > 0 && temp != '\t') {
    if (temp == '.') dot = 1;
    if (isalpha(temp)) valid = 0;
    ip[i] = temp;
    i++;
  }
  ip[i] = '\0';
  if (dot == 0 || valid == 0 || ip == NULL) {
    printf(".configure file is invalid/corrupted!\n");
    close(fd);
    return 0;
  }

  bzero(host, 250);
  int n = read(fd, host, 250);
  if (n < 0) {
    printf("Host is Incorrect!\n");
    close(fd);
    return 0;
  }
  valid = 1;
  for (i = 0; i < strlen(ip); i++) {
    if (isalpha(ip[i])) {
      valid = 0;
      break;
    }
  }
  if (host == NULL || valid == 0) {
    printf("Host is Incorrect!\n");
    close(fd);
    return 0;
  }

  struct hostent *server = gethostbyname(ip);
  if (server < 0) {
    printf("Error: Given IP Address is invalid\n");
    close(fd);
    return 0;
  }

  // Create the Socket
  client_socket = socket(AF_INET, SOCK_STREAM, 0);

  // Initialize the address
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(atoi(host));
  bcopy((char *) server->h_addr, (char*) &address.sin_addr.s_addr, server->h_length);

  signal(SIGINT,handle);
  // Connect to the Socket
  do {
    int connection_status = connect(client_socket, (struct sockaddr*) &address, sizeof(address));
    if (connection_status < 0) {
      printf("Could not connect to the server!\nTrying again...\n");
      sleep(3);
    } else {
      printf("\nEstablished connection with the server!\n\n");
      break;
    }
  } while (1);
  close(fd);
  return 1;
}

void usage(char *first_arg) {
  printf("Usage:\t%s configure <IP Address> <Port>\n", first_arg);
  printf("\t%s checkout <Project Name>\n", first_arg);
  printf("\t%s update <Project Name>\n", first_arg);
  printf("\t%s upgrade <Project Name>\n", first_arg);
  printf("\t%s commit <Project Name>\n", first_arg);
  printf("\t%s push <Project Name>\n", first_arg);
  printf("\t%s create <Project Name>\n", first_arg);
  printf("\t%s destroy <Project Name>\n", first_arg);
  printf("\t%s add <Project Name> <Filename>\n", first_arg);
  printf("\t%s remove <Project Name> <Filename>\n", first_arg);
  printf("\t%s currentversion <Project Name>\n", first_arg);
  printf("\t%s history <Project Name>\n", first_arg);
  printf("\t%s rollback <Project Name> <Version>\n", first_arg);
}

int main(int argc, char** argv) {

  if (argc != 3 && argc != 4) {
    usage(argv[0]);
    return 0;
  }

  signal(SIGINT,handle);

  if (strcmp(argv[1], "configure") == 0) {
    if (argc != 4) {
      usage(argv[0]);
      return 0;
    }
    configure(argv[2], argv[3]);
  } else if (strcmp(argv[1], "checkout") == 0) {
    if (argc != 3) {
      usage(argv[0]);
      return 0;
    }
    if (!checkout(argv[2])) {
      printf("Checkout failed...\n");
    }
  } else if (strcmp(argv[1], "create") == 0) {
    if (argc != 3) {
      usage(argv[0]);
      return 0;
    }
    if (!create(argv[2])) {
      printf("Create failed...\n");
    }
  } else if (strcmp(argv[1], "update") == 0) {
    if (argc != 3) {
      usage(argv[0]);
      return 0;
    }
    if (!update(argv[2])) {
      printf("Update failed...\n");
    }
  } else if (strcmp(argv[1], "add") == 0) {
    if (argc != 4) {
      usage(argv[0]);
      return 0;
    }
    if (!add(argv[2], argv[3])) {
      printf("Add failed...\n");
    }
  } else if (strcmp(argv[1], "remove") == 0) {
    if (argc != 4) {
      usage(argv[0]);
      return 0;
    }
    if (!remuuv(argv[2], argv[3])) {
      printf("Remove failed...\n");
    }
  } else if (strcmp(argv[1], "commit") == 0) {
    if (argc != 3) {
      usage(argv[0]);
      return 0;
    }
    if (!commit(argv[2])) {
      printf("Commit failed...\n");
    }
  } else if (strcmp(argv[1], "push") == 0) {
    if (argc != 3) {
      usage(argv[0]);
      return 0;
    }
    if (!push(argv[2])) {
      printf("Push failed...\n");
    }
  } else if (strcmp(argv[1], "upgrade") == 0) {
    if (argc != 3) {
      usage(argv[0]);
      return 0;
    }
    if (!upgrade(argv[2])) {
      printf("Upgrade failed...\n");
    }
  } else if (strcmp(argv[1], "currentversion") == 0) {
    if (argc != 3) {
      usage(argv[0]);
      return 0;
    }
    if (!currentversion(argv[2])) {
      printf("Current-version failed...\n");
    }
  } else if (strcmp(argv[1], "rollback") == 0) {
    if (argc != 4) {
      usage(argv[0]);
      return 0;
    }
    if (!rollback(argv[2], argv[3])) {
      printf("Rollback failed...\n");
    }
  } else if (strcmp(argv[1], "destroy") == 0) {
    if (argc != 3) {
      usage(argv[0]);
      return 0;
    }
    if (!destroy(argv[2])) {
      printf("Destroy failed...\n");
    }
  } else if (strcmp(argv[1], "history") == 0) {
    if (argc != 3) {
      usage(argv[0]);
      return 0;
    }
    if (!history(argv[2])) {
      printf("History failed...\n");
    }
  }

  close(client_socket);

  return 0;
}
