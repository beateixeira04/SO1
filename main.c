#define _DEFAULT_SOURCE

#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "constants.h"
#include "operations.h"
#include "parser.h"

/*GLOBAL VARIABLES*/
DIR *dir;
int MAX_PROC;
int active_child = 0;

typedef struct {
  char *dir_path;
} ThreadArgs;

pthread_mutex_t dir_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t active_child_mutex = PTHREAD_MUTEX_INITIALIZER;
/*END OF GLOBAL VARIABLES*/

/*MAIN THREAD FUNCTION*/
void *thread_operation(void *arg) {
  ThreadArgs *args = (ThreadArgs *)arg;
  char *dir_path = args->dir_path;

  struct dirent *dp;
  while (1) {

    safe_mutex_lock(&dir_lock);
    dp = readdir(dir);
    safe_mutex_unlock(&dir_lock);

    if (dp == NULL) { // If there are no more files to read
      break;
    }
    // Only process file if it is .job
    if (dp->d_type == DT_REG && strlen(dp->d_name) > 3 &&
        strcmp(dp->d_name + strlen(dp->d_name) - 4, ".job") == 0) {
      int backups = 1;

      /*CREATING STRING JOB FILE PATH*/
      size_t len_path = strlen(dir_path) + 1 + strlen(dp->d_name) + 1;
      char *jobs_file_path = (char *)safe_malloc(len_path);
      snprintf(jobs_file_path, len_path, "%s/%s", dir_path, dp->d_name);

      int jobs_fd = open(jobs_file_path, O_RDONLY);
      if (jobs_fd == -1) {
        fprintf(stderr, "Failed to open .job file\n");
        free(jobs_file_path);
        return NULL;
      }
      /*JOB FILE OPENED*/

      /*CREATING STRING OUT FILE PATH*/
      char output_file_path[PATH_MAX];
      snprintf(output_file_path, sizeof(output_file_path), "%.*sout",
               (int)(strlen(jobs_file_path) - 3), jobs_file_path);

      int out_fd = open(output_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (out_fd < 0) {
        fprintf(stderr, "Failed to create output file\n");
        free(jobs_file_path);
        return NULL;
      }
      /*OUT FILE CREATED*/

      int should_exit = 0;
      while (!should_exit) {
        char keys[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
        char values[MAX_WRITE_SIZE][MAX_STRING_SIZE] = {0};
        unsigned int delay;
        size_t num_pairs;

        switch (get_next(jobs_fd)) {
        case CMD_WRITE:
          num_pairs = parse_write(jobs_fd, keys, values, MAX_WRITE_SIZE,
                                  MAX_STRING_SIZE);
          if (num_pairs == 0) {
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            continue;
          }

          if (kvs_write(num_pairs, keys, values)) {
            fprintf(stderr, "Failed to write pair\n");
          }
          break;

        case CMD_READ:
          num_pairs =
              parse_read_delete(jobs_fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);
          if (num_pairs == 0) {
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            continue;
          }

          if (kvs_read(num_pairs, keys, out_fd)) {
            fprintf(stderr, "Failed to read pair\n");
          }
          break;

        case CMD_DELETE:
          num_pairs =
              parse_read_delete(jobs_fd, keys, MAX_WRITE_SIZE, MAX_STRING_SIZE);
          if (num_pairs == 0) {
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            continue;
          }

          if (kvs_delete(num_pairs, keys, out_fd)) {
            fprintf(stderr, "Failed to delete pair\n");
          }
          break;

        case CMD_SHOW:
          kvs_show(out_fd);
          break;

        case CMD_WAIT:
          if (parse_wait(jobs_fd, &delay, NULL) == -1) {
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            continue;
          }

          if (delay > 0) {
            write_to_file(out_fd, "Waiting...\n");
            kvs_wait(delay);
          }
          break;

        case CMD_BACKUP:

          /*CHECKING IF MAX LIMIT OF CHILD PROCESS IS REACHED AND WAITING*/
          safe_mutex_lock(&active_child_mutex);
          if (active_child == MAX_PROC) {
            pid_t pid = wait(NULL);
            if (pid == -1) {
              fprintf(stderr, "wait failed\n");
            } else {
              active_child--; // Decrement active children only if a child
                              // process exits
            }
          }
          safe_mutex_unlock(&active_child_mutex);
          /*WAIT ENDED - CREATING CHILD PROCESS*/
          lock_table();
          pid_t pid = fork();
          unlock_table();

          if (pid == -1) {
            fprintf(stderr, "Failed to fork\n");
            free(jobs_file_path);
            exit(1);
          }

          /*CHILD PROCESS*/
          if (pid == 0) {
            /*CREATING .BCK FILE*/
            char temp_path[MAX_JOB_FILE_NAME_SIZE];
            snprintf(temp_path, sizeof(temp_path), "%.*s",
                     (int)(strlen(jobs_file_path) - 4), jobs_file_path);

            char backup_file_path[PATH_MAX];
            snprintf(backup_file_path, sizeof(backup_file_path), "%s-%d.bck",
                     temp_path, backups);

            int bck_fd =
                open(backup_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (bck_fd < 0) {
              fprintf(stderr, "Failed to create backup file: %s\n",
                      backup_file_path);
              exit(1); // Ensure child exits on error
            }
            /*OPENED .BCK FILE*/
            if (kvs_backup(bck_fd)) { // Performing the backup
              fprintf(stderr, "Failed to perform backup.\n");
              if (close(bck_fd) == -1) {
                fprintf(stderr, "Failed to close .bck file\n");
                return NULL;
              }
              free(jobs_file_path);
              exit(1);
            }

            free(jobs_file_path);
            if (close(bck_fd) == -1) {
              fprintf(stderr, "Failed to close .bck file\n");
              return NULL;
            }
            kvs_terminate();
            close(jobs_fd);
            close(out_fd);
            closedir(dir);
            exit(0); // Child successfully exits after performing the backup
          }
          /*END OF CHILD PROCESS*/

          /*PARENT PROCESS JUMP*/
          active_child++; // Increment active children count
          backups++;
          break;

        case CMD_INVALID:
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          break;

        case CMD_HELP:
          printf("Available commands:\n"
                 "  WRITE [(key,value),(key2,value2),...]\n"
                 "  READ [key,key2,...]\n"
                 "  DELETE [key,key2,...]\n"
                 "  SHOW\n"
                 "  WAIT <delay_ms>\n"
                 "  BACKUP\n"
                 "  HELP\n");
          break;

        case CMD_EMPTY:
          break;

        case EOC:
          should_exit = 1;
          break;
        }
      }

      if (close(jobs_fd) == -1) {
        fprintf(stderr, "Failed to close .jobs file\n");
        return NULL;
      }
      if (close(out_fd) == -1) {
        fprintf(stderr, "Failed to close .out file\n");
        return NULL;
      }
      free(jobs_file_path);
    }
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <dir_path> <MAX_PROC> <MAX_THREADS>\n", argv[0]);
    return 1;
  }

  if (kvs_init()) {
    fprintf(stderr, "Failed to initialize KVS\n");
    return 1;
  }

  char *dir_path = argv[1];
  dir = opendir(dir_path);
  if (dir == NULL) {
    fprintf(stderr, "Failed to open directory\n");
    return 1;
  }

  if (sscanf(argv[2], "%d", &MAX_PROC) != 1) {
    fprintf(stderr, "Invalid number provided for MAX_PROC\n");
    return 1;
  }

  int MAX_THREADS = 0;
  if (sscanf(argv[3], "%d", &MAX_THREADS) != 1) {
    fprintf(stderr, "Invalid number provided for MAX_THREADS\n");
    return 1;
  }
  if (MAX_THREADS <= 0) {
    fprintf(stderr, "Invalid number of threads: %d\n", MAX_THREADS);
    return 1;
  }

  pthread_t threads[MAX_THREADS];
  int thread_created[MAX_THREADS];
  ThreadArgs args = {argv[1]};
  for (int i = 0; i < MAX_THREADS; i++) {
    thread_created[i] = 0;
  }

  for (int i = 0; i < MAX_THREADS; i++) {
    if (pthread_create(&threads[i], NULL, thread_operation, (void *)&args) !=
        0) {
      fprintf(stderr, "Error creating thread number: %d\n", i);
    } else {
      thread_created[i] = 1;
    }
  }
  for (int i = 0; i < MAX_THREADS; i++) {
    if (thread_created[i]) {
      if (pthread_join(threads[i], NULL) != 0) {
        fprintf(stderr, "Error joining thread number: %d\n", i);
      }
    }
  }

  closedir(dir);
  kvs_terminate();

  /*WAITING FOR ALL THE BACKUPS TO FINISH*/
  while (active_child > 0) {
    if (wait(NULL) > 0) {
      pthread_mutex_lock(&active_child_mutex);
      active_child--;
      pthread_mutex_unlock(&active_child_mutex);
    }
  }
  return 0;
}