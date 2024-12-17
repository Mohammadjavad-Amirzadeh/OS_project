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
int user_processed_list[MAX_USERS] = {0};
sem_t *global_semaphore; 
int store_purchases[3] = {0}; 

struct Product {
    char name[100];
    float price;
    float score;
    int entity;
    char last_modified[50];
    char file_path[MAX_PATH_LENGTH];
};

struct ShoppingList {
    struct Product products[MAX_PRODUCTS];
    int product_count;
    int threshold;
};

struct ThreadArgs {
    char file_path[MAX_PATH_LENGTH];
    struct Product *shopping_list;
    int shopping_list_count;
    char user_dir[MAX_PATH_LENGTH];
};

struct StoreScore {
    char store_name[50];
    float total_score;
    float total_price;
    int valid; 
};


struct ValuePuttingArgs {
    int threshold;
    int user_number;
    int user_list_number;
    struct Product *shopping_list;
    int shopping_list_count;
};
void read_user_shopping_list(const char *file_path, struct ShoppingList *shopping_lists, int *list_count);
void read_product_file(const char *file_path, struct Product *product);
void *process_file(void *arg);
void process_directory(const char *dir_path, const char *user_dir, struct Product *shopping_list, int shopping_list_count, int User_pid, int threshold, int user_number, int user_list_number);

void initialize_shared_semaphore() {
    global_semaphore = sem_open("/shared_semaphore", O_CREAT, 0666, 1);
    if (global_semaphore == SEM_FAILED) {
        perror("Failed to create or open semaphore");
        exit(EXIT_FAILURE);
    }
}

void destroy_shared_semaphore() {
    if (sem_close(global_semaphore) == -1) {
        perror("Failed to close semaphore");
    }
    if (sem_unlink("/shared_semaphore") == -1) {
        perror("Failed to unlink semaphore");
    }
}

void update_product_paths(const char *user_dir, struct Product *shopping_list, int shopping_list_count, const char *store_file) {
    FILE *store = fopen(store_file, "r");
    if (!store) {
        perror("Failed to open store file for updating product paths");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), store)) {
        struct Product product;
        if (sscanf(line, "Name: %99[^\n]", product.name) == 1) {
            fgets(line, sizeof(line), store);
            sscanf(line, "Price: %f", &product.price);

            fgets(line, sizeof(line), store);
            sscanf(line, "Score: %f", &product.score);

            fgets(line, sizeof(line), store);
            sscanf(line, "Entity: %d", &product.entity);

            fgets(line, sizeof(line), store);
            fgets(line, sizeof(line), store);
            sscanf(line, "Path: %s", product.file_path);

            for (int i = 0; i < shopping_list_count; i++) {
                struct Product *item = &shopping_list[i];
                if (strcmp(product.name, item->name) == 0) {
                    strncpy(item->file_path, product.file_path, MAX_PATH_LENGTH - 1);
                    item->file_path[MAX_PATH_LENGTH - 1] = '\0';
                }
            }
        }
    }

    fclose(store);
}

void calculate_store_scores(const char *user_dir, struct Product *shopping_list, int shopping_list_count, int threshold, struct StoreScore *store_scores) {
    char stores[3][50] = {"store1.txt", "store2.txt", "store3.txt"};

    for (int store_idx = 0; store_idx < 3; store_idx++) {
        char store_file[MAX_PATH_LENGTH];
        snprintf(store_file, sizeof(store_file), "%s/%s", user_dir, stores[store_idx]);
        //printf("STORE FILE : %s\n", store_file);

        FILE *store = fopen(store_file, "r");
        if (!store) {
            // printf("%s\n",store_file);
            perror("Failed to open store file");
            store_scores[store_idx].valid = 0;
            continue;
        }

        store_scores[store_idx].total_score = 0;
        store_scores[store_idx].total_price = 0;
        store_scores[store_idx].valid = 1; 
        strcpy(store_scores[store_idx].store_name, stores[store_idx]);

        char line[256];
        while (fgets(line, sizeof(line), store)) {
            struct Product product;
            if (sscanf(line, "Name: %99[^\n]", product.name) == 1) {
                fgets(line, sizeof(line), store);
                sscanf(line, "Price: %f", &product.price);

                fgets(line, sizeof(line), store);
                sscanf(line, "Score: %f", &product.score);
                fgets(line, sizeof(line), store);
                sscanf(line, "Entity: %d", &product.entity);

                fgets(line, sizeof(line), store); 
                fgets(line, sizeof(line), store);
                sscanf(line, "Path: %60[^\n]", product.file_path);
                //printf("aaaaaaaaaa %s\n" , product.file_path);


               read_product_file(product.file_path, &product);
               //printf("p1 name : %s , p1 entity : %d , p1 filepath : %s \n",product.name, product.entity, product.file_path);
               


                for (int i = 0; i < shopping_list_count; i++) {
                    struct Product *item = &shopping_list[i];
                    if (strcmp(product.name, item->name) == 0) {
                        if (product.entity < item->entity) {
                            
                            store_scores[store_idx].valid = 0;
                            break;
                        }

                        float discount = store_purchases[store_idx] > 0 ? 0.9 : 1.0; // 10% تخفیف
                        if (discount == 0.9){
                            printf("LIST get 10 percent discount. \nStore : %d\n", store_idx + 1);
                        }else{
                            printf("LIST didnt get 10 percent discount. \nStore : %d\n" , store_idx + 1);
                        }
                       
                        store_scores[store_idx].total_score += (product.score /( product.price * discount)) * item->entity;
                        store_scores[store_idx].total_price += (product.price * discount) * item->entity;

                       
                        if (store_scores[store_idx].total_price > threshold) {
                            store_scores[store_idx].valid = 0;
                        }
                    }
                }
            }
            if (!store_scores[store_idx].valid) break; 
        }
        //printf("STORE SCORE [%d] . VALID : %d\n", store_idx, store_scores[store_idx].valid);
        fclose(store);
    }
}


void update_store_inventory(const char *user_dir, struct Product *shopping_list, int shopping_list_count, int best_store) {
    for (int i = 0; i < shopping_list_count; i++) {
        struct Product *item = &shopping_list[i];

        FILE *product_file = fopen(item->file_path, "r+");
        if (!product_file) {
            printf("this is the given path: %s\n", item->file_path);
            perror("Failed to open product file for update");
            continue;
        }

        struct Product product;

       
        fscanf(product_file,
               "Name: %99[^\n]\nPrice: %f\nScore: %f\nEntity: %d\nLast Modified: %49[^\n]\: %s\n",
               product.name, &product.price, &product.score, &product.entity, product.last_modified);

 
        if (product.entity >= item->entity) {
            product.entity -= item->entity;
        } else {
            printf("Error: Insufficient stock for product %s in file %s\n", product.name, item->file_path);
            fclose(product_file);
            continue;
        }

        
        time_t now = time(NULL);
        strftime(product.last_modified, sizeof(product.last_modified), "%Y-%m-%d %H:%M:%S", localtime(&now));

        
        fseek(product_file, 0, SEEK_SET);
        fprintf(product_file,
                "Name: %s\nPrice: %.2f\nScore: %.2f\nEntity: %d\nLast Modified: %s\n",
                product.name, product.price, product.score, product.entity, product.last_modified);

        fclose(product_file);

        printf("Updated inventory product: %s in file: %s\n", product.name, item->file_path);
    }
}


void update_store_score(const char *user_dir, struct Product *shopping_list, int shopping_list_count, int best_store, int *votes) {
    for (int i = 0; i < shopping_list_count; i++) {
        struct Product *item = &shopping_list[i];

        
        FILE *product_file = fopen(item->file_path, "r+");
        if (!product_file) {
            printf("this is the given path: %s\n", item->file_path);
            perror("Failed to open product file for update");
            continue;
        }

        struct Product product;

        fscanf(product_file,
               "Name: %99[^\n]\nPrice: %f\nScore: %f\nEntity: %d\nLast Modified: %49[^\n]\: %s\n",
               product.name, &product.price, &product.score, &product.entity, product.last_modified);


        product.score = (product.score + votes[i]) / 2.0 ; 

        
        time_t now = time(NULL);
        strftime(product.last_modified, sizeof(product.last_modified), "%Y-%m-%d %H:%M:%S", localtime(&now));

     
        fseek(product_file, 0, SEEK_SET);
        fprintf(product_file,
                "Name: %s\nPrice: %.2f\nScore: %.2f\nEntity: %d\nLast Modified: %s\n",
                product.name, product.price, product.score, product.entity, product.last_modified);

        fclose(product_file);

        printf("Updated score product: %s in file: %s\n", product.name, item->file_path);
    }
}


int finalize_purchase(struct StoreScore *store_scores) {
  
    int best_store_idx = -1;
    float best_score = -1;

    

    for (int i = 0; i < 3; i++) {
        if (store_scores[i].valid && store_scores[i].total_score > best_score) {
            best_score = store_scores[i].total_score;
            best_store_idx = i;
        }
    }

    if (best_store_idx == -1) {
        printf("No valid store found for the purchase.\n");
        return -1;
    }

    struct StoreScore *best_store = &store_scores[best_store_idx];
    printf("Purchase completed at store: %s\n", best_store->store_name);
    printf("Total Price: %.2f\n", best_store->total_price);
    printf("Total Score: %.2f\n", best_store->total_score);
    store_purchases[best_store_idx]++; 
    return best_store_idx;
}

int read_integers_from_file_with_array(const char *filename, int *arr, int size) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        return -1; 
    }

    int num;
    int idx = 0;
    while (idx < size && fscanf(fp, "%d", &num) == 1) {
        arr[idx++] = num;
    }

    fclose(fp);

    return idx; 
}

void *give_value_to_list(void *arg) {
    struct ValuePuttingArgs *args = (struct ValuePuttingArgs *)arg;

    while (args->user_list_number-1 > user_processed_list[args->user_number]) {
        sleep(0.2);
    }

    sem_wait(global_semaphore);

    char user_dir[MAX_PATH_LENGTH];
    snprintf(user_dir, sizeof(user_dir), "output/user%d_list%d_output", args->user_number, args->user_list_number);

    struct StoreScore store_scores[3];
    calculate_store_scores(user_dir, args->shopping_list, args->shopping_list_count, args->threshold, store_scores);
    
    printf("Finalizing purchase for user%d list%d\n",args->user_number , args->user_list_number);
    int selected = finalize_purchase(store_scores);
    if (selected != -1)
    {
        char store_file[MAX_PATH_LENGTH];
        snprintf(store_file, sizeof(store_file), "%s/store%d.txt", user_dir, selected + 1);

   
        update_product_paths(user_dir, args->shopping_list, args->shopping_list_count, store_file);

        update_store_inventory(user_dir, args->shopping_list, args->shopping_list_count, selected);
    }
    
    user_processed_list[args->user_number]++;

    sem_post(global_semaphore);



    if (selected != -1)
    {

    char user_vote_file[MAX_PATH_LENGTH];
    snprintf(user_vote_file, sizeof(user_vote_file), "./rates/user%d_list%d.txt", args->user_number, args->user_list_number);
    printf("Waiting for user %d votes ...\n", args->user_number);
    while (access(user_vote_file, F_OK) != 0) {
        sleep(0.2);
    }    
    int array_size = args->shopping_list_count; // مثلاً ما می‌خواهیم حداکثر 100 عدد بخوانیم
    int *votes = (int *)malloc(array_size * sizeof(int));
    if (!votes) {
        perror("malloc");
        return 1;
    }

    int count = read_integers_from_file_with_array(user_vote_file, votes, array_size);


    sem_wait(global_semaphore);
    

    update_store_score(user_dir, args->shopping_list, args->shopping_list_count, selected, votes);

    sem_post(global_semaphore);
    printf("User %d voting is finished.\n", args->user_number);

    }

    pthread_exit(NULL);
}


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
            shopping_lists[current_list].threshold = INT_MAX;
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
                    thread_count++;
                }
            }
        }
    }
    if (getpid() == User_pid){
        printf("this is parent of %d\n", User_pid);
        sleep(2);
        printf("this is parent of %d after sleep\n", User_pid);

        pthread_t value_thread;
        struct ValuePuttingArgs *args = malloc(sizeof(struct ValuePuttingArgs));
        args->threshold = threshold;
        args->user_number = user_number;
        args->user_list_number = user_list_number;
        args->shopping_list = shopping_list;
        args->shopping_list_count = shopping_list_count;

        pthread_create(&value_thread, NULL, give_value_to_list, args);
        pthread_join(value_thread, NULL);
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

    initialize_shared_semaphore(); 
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
                snprintf(user_dir, sizeof(user_dir), "output/user%d_list%d_output", i, j + 1); // مسیر جدید در پوشه outpu
                mkdir(user_dir, 0777);

                for (int k = 0; k < 3; k ++){
                    char store_file[MAX_PATH_LENGTH];
                    snprintf(store_file, sizeof(store_file), "%s/store%d.txt", user_dir, k + 1);
               
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
    destroy_shared_semaphore(); 
    return 0;
}
