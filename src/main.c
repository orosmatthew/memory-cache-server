#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_PORT 1052 // we have 1050 range
#define BUF_SIZE 256

#define CACHE_SIZE 8
#define FILENAME_SIZE 256
#define FILE_CONTENT_SIZE 1024

#define MAX_THREADS 8

#define MAX_OUTPUT 128

typedef struct {
    char filename[FILENAME_SIZE];
    int n_bytes;
    char *p_contents;
    bool is_valid;
} CacheEntry;

typedef struct {
    CacheEntry entries[CACHE_SIZE];
} Cache;

typedef struct {
    pthread_t thread;
    bool is_running;
} Thread;

typedef struct {
    size_t size;
    char *data;
    int thread_index;
    int connection;
} Input;

pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
Cache cache;

pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;
Thread threads[MAX_THREADS];

void send_string(int connection, const char *str)
{
    // Send the string over the network
    write(connection, str, strlen(str));
}

int hash_filename(char *filename)
{
    return (int)(filename[0]);
}

void command_load(size_t arg_size, const char *args, int connection)
{
    char filename[FILENAME_SIZE];
    for (int i = 0; i < arg_size + 1; i++) {
        if (args[i] != ' ') {
            filename[i] = args[i];
        }
        else {
            filename[i] = '\0';
        }
    }

    int index = hash_filename(filename) % CACHE_SIZE;

    pthread_mutex_lock(&cache_mutex);
    if (strcmp(cache.entries[index].filename, filename) == 0 && cache.entries[index].is_valid) {
        char output[MAX_OUTPUT];
        snprintf(output, sizeof(output), "%d:%s\n", cache.entries[index].n_bytes, cache.entries[index].p_contents);
        send_string(connection, output);
    }
    else {
        char output[MAX_OUTPUT];
        snprintf(output, sizeof(output), "0:\n");
        send_string(connection, output);
    }
    pthread_mutex_unlock(&cache_mutex);
}

void command_store(size_t arg_size, const char *args)
{
    char filename[FILENAME_SIZE];
    char n_bytes_str[8];
    char *contents = (char *)malloc(FILE_CONTENT_SIZE);

    int step = 0; // 0: filename, 1: n_bytes_str, 2: contents
    int offset = 0; // offset from last arg
    for (int i = 0; i < arg_size + 1; i++) {
        // Parsing filename
        if (step == 0 && args[i] != ' ') {
            filename[i] = args[i];
        }
        else if (step == 0) {
            filename[i] = '\0';
            i++;
            offset = i;
            step++;
        }

        // Parsing n_bytes_str
        if (step == 1 && args[i] != ':') {
            n_bytes_str[i - offset] = args[i];
        }
        else if (step == 1) {
            n_bytes_str[i - offset] = '\0';
            i++;
            offset = i;
            step++;
        }

        // Parsing contents
        if (step == 2) {
            contents[i - offset] = args[i];
        }
    }

    CacheEntry entry;
    entry.is_valid = true;
    strcpy(entry.filename, filename);

    char *end_ptr;
    entry.n_bytes = (int)strtol(n_bytes_str, &end_ptr, 10);

    entry.p_contents = contents;

    int index = hash_filename(filename) % CACHE_SIZE;

    pthread_mutex_lock(&cache_mutex);
    if (cache.entries[index].is_valid) {
        free(cache.entries[index].p_contents);
    }
    cache.entries[index] = entry;
    pthread_mutex_unlock(&cache_mutex);
}

void command_remove(size_t arg_size, const char *args)
{
    char filename[FILENAME_SIZE];
    for (int i = 0; i < FILENAME_SIZE || i < arg_size; i++) {
        filename[i] = args[i];
    }

    int index = hash_filename(filename) % CACHE_SIZE;

    pthread_mutex_lock(&cache_mutex);
    if (strcmp(cache.entries[index].filename, filename) == 0 && cache.entries[index].is_valid) {
        cache.entries[index].is_valid = false;
    }
    pthread_mutex_unlock(&cache_mutex);
}

void *process_input(void *input_ptr)
{
    Input input = *((Input *)(input_ptr));

    char command_buffer[10];
    int command_size = 0; // NOTE: Might be off by 1
    char args_buffer[100];
    bool is_command = true;
    for (int i = 0; i < input.size; i++) {
        if (is_command) {
            if (input.data[i] != ' ') {
                command_buffer[i] = input.data[i];
            }
            else {
                command_buffer[i] = '\0';
                is_command = false;
                command_size = i;
            }
        }
        else {
            if (input.data[i] != '\0') {
                args_buffer[i - command_size - 1] = input.data[i];
            }
            else {
                args_buffer[i - command_size - 1] = '\0';
                break;
            }
        }
    }

    if (strcmp(command_buffer, "load") == 0) {
        command_load(strlen(args_buffer), args_buffer, input.connection);
    }
    if (strcmp(command_buffer, "store") == 0) {
        command_store(strlen(args_buffer), args_buffer);
    }
    if (strcmp(command_buffer, "rm") == 0) {
        command_remove(strlen(args_buffer), args_buffer);
    }

    pthread_mutex_lock(&thread_mutex);
    threads[input.thread_index].is_running = false;
    pthread_mutex_unlock(&thread_mutex);

    return NULL;
}

int serverSocket;

// Function to close socket connections
void closeConnection()
{
    printf("\nClosing Connection\n");
    close(serverSocket);
    exit(1);
}

int main()
{
    memset(&cache, 0, sizeof(Cache));
    memset(&threads, 0, sizeof(threads));

    char receiveLine[BUF_SIZE];

    int connectionToClient;
    size_t bytesRead;

    // Creating a server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddress;
    bzero(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;

    // Configure to listen to any address and convert from host to network format
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(SERVER_PORT);

    // Bind to port, error message if failure to bind
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1) {
        printf("Unable to bind to port\n");
        exit(-1);
    }

    // Allows for Ctrl+C to close connection
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = closeConnection;
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    // Start listening for up to 10 connections
    listen(serverSocket, 10);

    while (true) {
        connectionToClient = accept(serverSocket, (struct sockaddr *)NULL, NULL);

        // Read command from client
        while ((bytesRead = read(connectionToClient, receiveLine, BUF_SIZE)) > 0) {
            // Put NULL terminator at end
            receiveLine[bytesRead] = '\0';

            // Terminate newline from input if present
            for (int i = 0; i < BUF_SIZE; i++) {
                if (receiveLine[i] == '\n') {
                    receiveLine[i] = '\0';
                    break;
                }
                else if (receiveLine[i] == '\0') {
                    break;
                }
            }

            int thread_index = -1;
            pthread_mutex_lock(&thread_mutex);
            for (int i = 0; i < MAX_THREADS; i++) {
                if (threads[i].is_running == false) {
                    threads[i].is_running = true;
                    thread_index = i;
                    break;
                }
            }
            pthread_mutex_unlock(&thread_mutex);

            if (thread_index != -1) {
                Input input;
                input.size = bytesRead + 1;
                input.data = receiveLine;
                input.thread_index = thread_index;
                input.connection = connectionToClient;
                // Start thread to handle command input
                pthread_create(&(threads[thread_index].thread), NULL, process_input, &input);
            }

            // pthread_create(&processThread, NULL, process_input, (void *))
            //  Need to change process_input to allow for threading
        }
    }
}
