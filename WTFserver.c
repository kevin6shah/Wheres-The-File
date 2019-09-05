#include "data.h"

int server_socket;

int send_file(int client_socket, char* server_path) {
  int fd = open(server_path, O_RDONLY);
  if (fd < 0) {
    printf("Incorrect path entered\n");
    close(fd);
    return 0;
  }

  struct stat stats;
  stat(server_path, &stats);
  int buf_size = stats.st_size;
  char size[255];
  sprintf(size, "%d", buf_size);
  write(client_socket, size, strlen(size));
  write(client_socket, "$TOKEN", strlen("$TOKEN"));

  char *buffer = malloc(buf_size + 1);
  int n = read(fd, buffer, buf_size);
  if (n < 0) {
    printf("Error Reading\n");
    return 0;
  }
  n = write(client_socket, buffer, buf_size);
  if (n < 0) {
    printf("Error Writing\n");
    return 0;
  }

  write(client_socket, "$TOKEN", strlen("$TOKEN"));
  close(fd);
  return 1;
}

int recieve(int client_socket) {
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
  close(fd);
  return 1;
}

void senddir_helper(int client_socket, char* server_path, char* client_path) {
  DIR *directory = opendir(server_path);
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
      write(client_socket, temp_path, strlen(temp_path));
      write(client_socket, "$TOKEN", strlen("$TOKEN"));

      senddir_helper(client_socket, new_path, temp_path);
    } else if (data->d_type != 4) {
      // File
      write(client_socket, "*", strlen("*"));
      write(client_socket, client_path, strlen(client_path));
      write(client_socket, "/", strlen("/"));
      write(client_socket, data->d_name, strlen(data->d_name));
      write(client_socket, "$TOKEN", strlen("$TOKEN"));

      char new_path[250];
      strcpy(new_path, server_path);
      strcat(new_path, "/");
      strcat(new_path, data->d_name);
      if (!send_file(client_socket, new_path)) {
        printf("Error: Could not send '%s' to the client\n", new_path);
        write(client_socket, "$FFF$$TOKEN", strlen("$FFF$$TOKEN"));
      }
    }
  }
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

int most_recent_version(char* path) {
  DIR *directory = opendir(path);
  if (directory == NULL) {
    printf("Could not open directory\n");
    return -1;
  }
  int recent_version = -1;
  struct dirent *data;
  while ((data = readdir(directory)) != NULL) {
    if (data->d_type == 4 && strcmp(data->d_name, ".") != 0 && strcmp(data->d_name, "..") != 0) {
      // Directory
      int temp = atoi((data->d_name) + 7);
      int current_version = temp;
      if (current_version > recent_version) {
        recent_version = current_version;
      }
    }
  }
  return recent_version;
}

int senddir(int client_socket, char* project_name) {
  write(client_socket, "#", strlen("#"));
  write(client_socket, project_name, strlen(project_name));
  write(client_socket, "$TOKEN", strlen("$TOKEN"));
  char server_path[255] = ".server_repo/";
  strcat(server_path, project_name);
  strcat(server_path, "/");

  int recent_version = most_recent_version(server_path);
  if (recent_version < 0) return 0;
  char rec_version[255];
  sprintf(rec_version, "%d", recent_version);
  strcat(server_path, "version");
  strcat(server_path, rec_version);
  senddir_helper(client_socket, server_path, project_name);
  return 1;
}

int checkout(int client_socket, char* project_name) {
  if (!exists("./", ".server_repo") || !exists(".server_repo/", project_name)) {
    return 0;
  }

  if (!senddir(client_socket, project_name)) return 0;
  printf("Checkout succeeded...\n");
  return 1;
}

int upgrade(int client_socket, char* project_name) {
  if (!exists("./", ".server_repo") || !exists(".server_repo/", project_name)) {
    return 0;
  }

  write(client_socket, "*", 1);
  write(client_socket, ".Manifest", strlen(".Manifest"));
  write(client_socket, "$TOKEN", strlen("$TOKEN"));

  char new_path[255] = ".server_repo/";
  strcat(new_path, project_name);
  strcat(new_path, "/");
  int recent_version = most_recent_version(new_path);
  if (recent_version < 0) return 0;
  char version[255];
  sprintf(version, "%d", recent_version);
  strcat(new_path, "version");
  strcat(new_path, version);
  strcat(new_path, "/.Manifest");
  if (!send_file(client_socket, new_path)) {
    printf("Error: Could not send '%s' to the server\n", new_path);
    write(client_socket, "$FFF$$TOKEN", strlen("$FFF$$TOKEN"));
    return 0;
  } else write(client_socket, "$***$$TOKEN", strlen("$***$$TOKEN"));

  int i = 0;
  char c;
  char file_path[255];
  while(1){
    //this while loop gets the path of the file it needs to send.
    bzero(file_path, 255);
    while (read(client_socket, &c, 1) > 0) {
      file_path[i] = c;
      if (file_path[i] == 'N' && file_path[i - 1] == 'E' && file_path[i - 2] == 'K' && file_path[i - 3] == 'O'
      && file_path[i - 4] == 'T' && file_path[i - 5] == '$') {
        file_path[i - 5] = '\0';
        break;
      }
      i++;
    }
    if (strcmp(file_path, "$***$") == 0) break;
    if (strcmp(file_path, "$FFF$") == 0) return 0;

    write(client_socket, "*", strlen("*"));
    write(client_socket, file_path, strlen(file_path));
    write(client_socket, "$TOKEN", strlen("$TOKEN"));

    char server_path[255] = ".server_repo/", vp[255];
    strcat(server_path, project_name);
    int version = most_recent_version(server_path);
    if (version < 0) return 0;
    sprintf(vp, "%d", version);
    strcat(server_path, "/version");
    strcat(server_path, vp);
    strcat(server_path, file_path+strlen(project_name));
    if (!send_file(client_socket, server_path)) {
      write(client_socket, "$FFF$$TOKEN", strlen("$FFF$$TOKEN"));
      return 0;
    }
    write(client_socket, "$***$$TOKEN", strlen("$***$$TOKEN"));
    i = 0;
  }
  printf("Upgrade was successful...\n");
  return 1;
}

int create(int client_socket, char* project_name) {
  char path[255] = ".server_repo/", history_path[255];
  if (!exists("./", ".server_repo")) {
    mkdir(".server_repo", 0700);
  } else {
    if (exists(path, project_name)) return 0;
  }
  strcat(path, project_name);
  strcpy(history_path, path);
  strcat(history_path, "/.History");
  mkdir(path, 0700);
  strcat(path, "/version0");
  mkdir(path, 0700);
  strcat(path, "/.Manifest");
  int fd = open(path, O_WRONLY | O_CREAT, 0700);
  int historyfd = open(history_path, O_WRONLY | O_CREAT, 0700);
  if (fd < 0) {
    printf("Could not create the Manifest file...\n");
    close(fd);
    return 0;
  }
  if (historyfd < 0) {
    printf("Could not create the History file...\n");
    close(historyfd);
    return 0;
  }
  write(historyfd, "create\n0\n\n", strlen("create\n0\n\n"));
  write(fd, "0\n", strlen("0\n"));
  close(historyfd);
  close(fd);
  if (!senddir(client_socket, project_name)) return 0;
  printf("Create was successful\n");
  return 1;
}

int currentversion(int client_socket, char* project_name) {
  if (!exists("./", ".server_repo") || !exists(".server_repo/", project_name)) {
    return 0;
  }

  char new_path[255] = ".server_repo/";
  strcat(new_path, project_name);
  strcat(new_path, "/");
  int recent_version = most_recent_version(new_path);
  if (recent_version < 0) return 0;
  char version[255];
  sprintf(version, "%d", recent_version);
  strcat(new_path, "version");
  strcat(new_path, version);
  strcat(new_path, "/.Manifest");

  int fd = open(new_path, O_RDONLY);
  if (fd < 0) return 0;
  char c;
  while (read(fd, &c, 1) > 0) write(client_socket, &c, 1);

  printf("Current-Version was successful...\n");
  return 1;
}

int update(int client_socket, char* project_name) {
  if (!exists("./", ".server_repo") || !exists(".server_repo/", project_name)) {
    return 0;
  }
  write(client_socket, "*", 1);
  write(client_socket, ".Manifest", strlen(".Manifest"));
  write(client_socket, "$TOKEN", strlen("$TOKEN"));

  char new_path[255] = ".server_repo/";
  strcat(new_path, project_name);
  strcat(new_path, "/");
  int recent_version = most_recent_version(new_path);
  if (recent_version < 0) return 0;
  char version[255];
  sprintf(version, "%d", recent_version);
  strcat(new_path, "version");
  strcat(new_path, version);
  strcat(new_path, "/.Manifest");
  if (!send_file(client_socket, new_path)) {
    printf("Error: Could not send '%s' to the server\n", new_path);
    return 0;
  }
  printf("Update was successful...\n");
  return 1;
}

int push(int client_socket, char* project_name) {
  if (!exists("./", ".server_repo") || !exists(".server_repo/", project_name)) {
    return 0;
  }
  char new_path[255] = ".server_repo/";
  strcat(new_path, project_name);
  strcat(new_path, "/");
  int version = most_recent_version(new_path);
  if (version < 0) return 0;
  char vp[255];
  sprintf(vp, "%d", version);
  write(client_socket, vp, strlen(vp));
  write(client_socket, "$", 1);
  if (!recieve(client_socket)) return 0;

  version++;
  char history_path[255], commit_path[255];
  strcpy(history_path, new_path);
  strcpy(commit_path, new_path);
  strcat(commit_path, "version");
  bzero(vp, 255);
  sprintf(vp, "%d", version);
  strcat(commit_path, vp);
  strcat(commit_path, "/.Commit");

  strcat(history_path, ".History");
  int historyfd = open(history_path, O_RDONLY);
  int commitfd = open(commit_path, O_RDONLY);
  struct stat stats;
  struct stat stats_commit;
  stat(history_path, &stats);
  stat(commit_path, &stats_commit);
  int size = stats.st_size;
  int size2 = stats_commit.st_size;
  char *historybuf = malloc(size);
  char *commitbuf = malloc(size2);
  read(historyfd, historybuf, size);
  read(commitfd, commitbuf, size2);
  close(historyfd);
  close(commitfd);

  remove(history_path);
  historyfd = open(history_path, O_WRONLY | O_CREAT, 0700);
  write(historyfd, historybuf, strlen(historybuf));
  write(historyfd, vp, strlen(vp));
  write(historyfd, "\n", 1);
  write(historyfd, commitbuf, strlen(commitbuf));
  write(historyfd, "\n", 1);

  printf("Push was successful...\n");
  return 1;
}

int destroy(char* project_name) {
  if (!exists("./", ".server_repo") || !exists(".server_repo/", project_name)) return 0;

  char new_path[255] = ".server_repo/";
  strcat(new_path, project_name);
  if (!RMDIR(new_path)) return 0;

  rmdir(new_path);
  printf("Destroy succeeded...\n");
  return 1;
}

int history(int client_socket, char* project_name) {
  if (!exists("./", ".server_repo") || !exists(".server_repo/", project_name)) {
    return 0;
  }
  char path[255] = ".server_repo/";
  strcat(path, project_name);
  strcat(path, "/.History");
  int fd = open(path, O_RDONLY);
  struct stat stats;
  stat(path, &stats);
  char *buf = malloc(stats.st_size);
  read(fd, buf, stats.st_size);
  write(client_socket, buf, stats.st_size);
  return 1;
}

int commit(int client_socket, char* project_name) {
  if (!exists("./", ".server_repo") || !exists(".server_repo/", project_name)) {
    return 0;
  }
  write(client_socket, "*", 1);
  write(client_socket, ".Manifest", strlen(".Manifest"));
  write(client_socket, "$TOKEN", strlen("$TOKEN"));

  char new_path[255] = ".server_repo/";
  strcat(new_path, project_name);
  strcat(new_path, "/");
  int recent_version = most_recent_version(new_path);
  if (recent_version < 0) return 0;
  char version[255];
  sprintf(version, "%d", recent_version);
  strcat(new_path, "version");
  strcat(new_path, version);
  strcat(new_path, "/.Manifest");
  if (!send_file(client_socket, new_path)) {
    printf("Error: Could not send '%s' to the server\n", new_path);
    return 0;
  }
  write(client_socket, "$***$$TOKEN", strlen("$***$$TOKEN"));
  write(client_socket, version, strlen(version));
  write(client_socket, "$", strlen("$"));
  if (!recieve(client_socket)) {
    printf("Error: Commit failed\n");
    return 0;
  }
  printf("Commit Succeeded\n");
  return 1;
}

int rollback(int client_socket, char* project_name, char* version_num) {
  if (!exists("./", ".server_repo") || !exists(".server_repo/", project_name)) {
    write(client_socket, "$FFF$", strlen("$FFF$"));
    return 0;
  }
  char new_path[255] = ".server_repo/";
  strcat(new_path, project_name);
  strcat(new_path, "/");
  int version = most_recent_version(new_path);
  if (version < 0) {
    write(client_socket, "$FFF$", strlen("$FFF$"));
    return 0;
  }
  char vp[255] = "version";
  strcat(vp, version_num);
  if (!exists(new_path, vp)) {
    write(client_socket, "$FFF$", strlen("$FFF$"));
    return 0;
  }
  int i = atoi(version_num);
  if (i == version) {
    printf("Rollback was inputted the current version...\n");
    write(client_socket, "$FFF$", strlen("$FFF$"));
    return 0;
  }
  i++;
  char temp[10];
  char path[255];
  for (;i <= version; i++) {
    bzero(temp, 10);
    bzero(path, 255);
    strcpy(path, new_path);
    strcat(path, "version");
    sprintf(temp, "%d", i);
    strcat(path, temp);
    if (!RMDIR(path)) {
      write(client_socket, "$FFF$", strlen("$FFF$"));
      return 0;
    }
    rmdir(path);
  }

  printf("Rollback was successful...\n");
  return 1;
}

void* main_process(void* socket) {
  int client_socket = *(int*) socket;
  int i = 0, seen_colon = 0;
  char token[255], project_name[255], file_path[255];

  char c;
  while(read(client_socket, &c, 1) > 0) {
    if (c == ':') {
      if (seen_colon == 1) {
        project_name[i] = '\0';
        seen_colon = 2;
      } else {
        token[i] = '\0';
        seen_colon = 1;
      }
      i = -1;
    }
    if (seen_colon == 0) {
      token[i] = c;
    } else if (seen_colon == 1 && i >= 0) {
      project_name[i] = c;
    } else if (seen_colon == 2 && i >= 0) {
      file_path[i] = c;
    }
    if (seen_colon == 1 && project_name[i] == 'N' && project_name[i-1] == 'E' && project_name[i-2] == 'K' &&
        project_name[i-3] == 'O' && project_name[i-4] == 'T' && project_name[i-5] == '$') {
        project_name[i-5] = '\0';
        break;
    }
    if (seen_colon == 2 && file_path[i] == 'N' && file_path[i-1] == 'E' && file_path[i-2] == 'K' &&
        file_path[i-3] == 'O' && file_path[i-4] == 'T' && file_path[i-5] == '$') {
        file_path[i-5] = '\0';
        break;
    }
    i++;
  }

  if (strcmp(token, "checkout") == 0) {
    if (!checkout(client_socket, project_name)) {
      write(client_socket, "$FFF$$TOKEN", strlen("$FFF$$TOKEN"));
      printf("Checkout failed...\n");
    } else write(client_socket, "$***$$TOKEN", strlen("$***$$TOKEN"));
  } else if (strcmp(token, "create") == 0) {
    if (!create(client_socket, project_name)) {
      write(client_socket, "$FFF$$TOKEN", strlen("$FFF$$TOKEN"));
      printf("Create failed...\n");
    } else write(client_socket, "$***$$TOKEN", strlen("$***$$TOKEN"));
  } else if (strcmp(token, "update") == 0) {
    if (!update(client_socket, project_name)) {
      write(client_socket, "$FFF$$TOKEN", strlen("$FFF$$TOKEN"));
      printf("Update failed...\n");
    } else write(client_socket, "$***$$TOKEN", strlen("$***$$TOKEN"));
  } else if (strcmp(token, "commit") == 0) {
    if (!commit(client_socket, project_name)) {
      write(client_socket, "$FFF$$TOKEN", strlen("$FFF$$TOKEN"));
      printf("Commit failed...\n");
    } else write(client_socket, "$***$$TOKEN", strlen("$***$$TOKEN"));
  } else if (strcmp(token, "push") == 0) {
    if (!push(client_socket, project_name)) {
      write(client_socket, "$FFF$$TOKEN", strlen("$FFF$$TOKEN"));
      printf("Push failed...\n");
    } else write(client_socket, "$***$$TOKEN", strlen("$***$$TOKEN"));
  } else if (strcmp(token, "upgrade") == 0) {
    if (!upgrade(client_socket, project_name)) {
      write(client_socket, "$FFF$$TOKEN", strlen("$FFF$$TOKEN"));
      printf("Upgrade failed...\n");
    } else write(client_socket, "$***$$TOKEN", strlen("$***$$TOKEN"));
  } else if (strcmp(token, "currentversion") == 0) {
    if (!currentversion(client_socket, project_name)) {
      write(client_socket, "$FFF$", strlen("$FFF$"));
      printf("Current-Version failed...\n");
    }
  } else if (strcmp(token, "rollback") == 0) {
    if (!rollback(client_socket, project_name, file_path)) {
      write(client_socket, "$FFF$", strlen("$FFF$"));
      printf("Rollback failed...\n");
    }
  } else if (strcmp(token, "destroy") == 0) {
    if (!destroy(project_name)) {
      write(client_socket, "$FFF$", strlen("$FFF$"));
      printf("Destroy failed...\n");
    }
  } else if (strcmp(token, "history") == 0) {
    if (!history(client_socket, project_name)) {
      write(client_socket, "$FFF$", strlen("$FFF$"));
      printf("History failed...\n");
    }
  }
  // Else ifs after this

  close(client_socket);
  return NULL;
}

int connect_server(char *port) {
  // Create the socket
  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    printf("Issue trying to create socket\n");
    return 0;
  }

  int enable = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    printf("setsockopt(SO_REUSEADDR) failed\n");

  // Make the address
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(atoi(port));
  address.sin_addr.s_addr = INADDR_ANY;

  // BIND
  if (bind(server_socket, (struct sockaddr*) &address, sizeof(address)) < 0) {
    printf("Issue trying to bind the socket\n");
    return 0;
  }

  // LISTEN
  listen(server_socket, 50);

  pthread_t thread[60];
  int i = 0;
  // ACCEPT
  while(1) {
    int client_socket = accept(server_socket, NULL, NULL);
    if (client_socket != -1) {
      printf("Established connection with a client!\n");
      pthread_create(&thread[i], NULL, main_process, (void*) &client_socket);
    }
    i++;
  }
  return 1;
}

int main(int argc, char** argv) {

  if (argc != 2) {
    printf("Usage: %s <Host>\n", argv[0]);
    return 0;
  }

  if (!connect_server(argv[1])) {
    printf("Could not establish a connection with the server\n");
    return 0;
  }

  return 0;
}
