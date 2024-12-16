#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <semaphore.h>
#include <errno.h>

#define MAX_PATH_LENGTH 1024
#define MAX_PRODUCTS 300
#define MAX_LISTS 10
#define MAX_USERS 100
#define MAX_THREADS 10000

pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

struct Product {
    char name[100];
    float price;
    float score;
    int entity;
    char last_modified[50];
    char file_path[MAX_PATH_LENGTH];
};


void read_user_shopping_list(const char *file_path, struct ShoppingList *shopping_lists, int *list_count) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("Failed to open user shopping list file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    *list_count = 0;
    int current_list = -1;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "list")) {
            current_list++;
            shopping_lists[current_list].product_count = 0;
            shopping_lists[current_list].threshold = INT_MAX; // Default threshold
        } else if (strstr(line, "Treshhold")) {
            sscanf(line, "Treshhold : %d", &shopping_lists[current_list].threshold);
        } else {
            char item[100];
            int quantity;
            if (sscanf(line, "Item : %99[^,], Quantity : %d", item, &quantity) == 2) {
                struct Product *product = &shopping_lists[current_list].products[shopping_lists[current_list].product_count++];
                strncpy(product->name, item, sizeof(product->name) - 1);
                product->entity = quantity; // از ورودی کاربر
                product->name[sizeof(product->name) - 1] = '\0';
            }
        }
    }

    *list_count = current_list + 1;
    fclose(file);
    printf("Read shopping list from %s. Total lists: %d\n", file_path, *list_count);
}

void read_product_file(const char *file_path, struct Product *product) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("Unable to open product file");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "Name: %99[^\n]", product->name) == 1) {
            fgets(line, sizeof(line), file);
            sscanf(line, "Price: %f", &product->price);

            fgets(line, sizeof(line), file);
            sscanf(line, "Score: %f", &product->score);

            fgets(line, sizeof(line), file);
            sscanf(line, "Entity: %d", &product->entity);

            fgets(line, sizeof(line), file);
            sscanf(line, "Last Modified: %49[^\n]", product->last_modified);

            strncpy(product->file_path, file_path, MAX_PATH_LENGTH - 1);
            product->file_path[MAX_PATH_LENGTH - 1] = '\0';
        }
    }

    fclose(file);
    
}

void *process_file(void *arg) {
    struct ThreadArgs *args = (struct ThreadArgs *)arg;
    char *file_path = args->file_path;
    struct Product *shopping_list = args->shopping_list;
    int shopping_list_count = args->shopping_list_count;
    char *user_dir = args->user_dir;

    struct Product product;
    read_product_file(file_path, &product);

    char store_name[50];
    if (strstr(file_path, "Store1")) {
        strcpy(store_name, "store1.txt");
    } else if (strstr(file_path, "Store2")) {
        strcpy(store_name, "store2.txt");
    } else if (strstr(file_path, "Store3")) {
        strcpy(store_name, "store3.txt");
    } else {
        free(arg);
        pthread_exit(NULL);
    }

    for (int i = 0; i < shopping_list_count; i++) {
        if (strcmp(product.name, shopping_list[i].name) == 0) {
            char output_file[MAX_PATH_LENGTH];
            snprintf(output_file, sizeof(output_file), "%s/%s", user_dir, store_name);

            pthread_mutex_lock(&file_mutex);
            FILE *output = fopen(output_file, "a");
            if (output) {
                fprintf(output, "Name: %s\nPrice: %.2f\nScore: %.2f\nEntity: %d\nLast Modified: %s\nPath: %s\n\n",
                        product.name, product.price, product.score, product.entity, product.last_modified, product.file_path);
                fclose(output);
            } else {
                perror("Failed to open output file");
            }
            pthread_mutex_unlock(&file_mutex);
        }
    }

    free(arg);
    pthread_exit(NULL);
}

void process_directory(const char *dir_path, const char *user_dir, struct Product *shopping_list, int shopping_list_count, int User_pid, int threshold, int user_number, int user_list_number) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Failed to open directory");
        return;
    }

    pthread_t threads[MAX_THREADS];
    int thread_count = 0;
    pid_t child_pids[MAX_PRODUCTS];
    int child_count = 0;

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat statbuf;
        if (stat(full_path, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                pid_t pid = fork();
                if (pid == 0) {
                    printf("PID : %d Created Child process (PID : %d) for Path: %s \n", getppid(), getpid(), full_path);
                    process_directory(full_path, user_dir, shopping_list, shopping_list_count, User_pid, threshold, user_number, user_list_number);
                    exit(0);
                } else {
                    child_pids[child_count++] = pid;
                }
            } else if (S_ISREG(statbuf.st_mode) && strstr(entry->d_name, ".txt")) {
                struct ThreadArgs *args = malloc(sizeof(struct ThreadArgs));
                strncpy(args->file_path, full_path, MAX_PATH_LENGTH - 1);
                args->file_path[MAX_PATH_LENGTH - 1] = '\0';
                args->shopping_list = shopping_list;
                args->shopping_list_count = shopping_list_count;
                strncpy(args->user_dir, user_dir, MAX_PATH_LENGTH - 1);
                args->user_dir[MAX_PATH_LENGTH - 1] = '\0';

                if (pthread_create(&threads[thread_count], NULL, process_file, args) != 0) {
                    perror("Failed to create thread");
                } else {
                    //printf("PID : %d Create Thread with Thread TID %ld created for file %s\n", getpid(), threads[thread_count], full_path);
                    thread_count++;
                }
            }
        }
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    for (int i = 0; i < child_count; i++) {
        waitpid(child_pids[i], NULL, 0);
    }

    closedir(dir);
}

int main() {
    int user_count;
    printf("Enter the number of users: ");
    scanf("%d", &user_count);

  
    if (mkdir("output", 0777) == -1 && errno != EEXIST) {
        perror("Failed to create output directory");
        exit(EXIT_FAILURE);
    }


    for (int i = 1; i <= user_count; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            char user_file[50];
            snprintf(user_file, sizeof(user_file), "users_shopping_list/user%d.txt", i);

            struct ShoppingList shopping_lists[MAX_LISTS];
            int list_count = 0;
            read_user_shopping_list(user_file, shopping_lists, &list_count);

            for (int j = 0; j < list_count; j++) {
                char user_dir[100];
                snprintf(user_dir, sizeof(user_dir), "output/user%d_list%d_output", i, j + 1);
                mkdir(user_dir, 0777);

                for (int k = 0; k < 3; k ++){
                    char store_file[MAX_PATH_LENGTH];
                    snprintf(store_file, sizeof(store_file), "%s/store%d.txt", user_dir, k + 1);
                    // printf("store FILE : %s\n", store_file);
                    fclose(fopen(store_file, "w"));
                }

                printf("Processing directory for user %d, list %d\n", i, j + 1);
                process_directory("./stores", user_dir, shopping_lists[j].products, shopping_lists[j].product_count, getpid(), shopping_lists[j].threshold, i, j+1);
            }
            exit(0);
        }
    }

    for (int i = 0; i < user_count; i++) {
        wait(NULL);
    }

    pthread_mutex_destroy(&file_mutex);
    return 0;
}
