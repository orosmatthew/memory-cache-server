#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

#define SERVER_PORT 1052 // we have 1050 range
#define BUF_SIZE 256

#define CACHE_SIZE 8
#define FILENAME_SIZE 256
#define FILE_CONTENT_SIZE 1024

typedef struct
{
    char filename[FILENAME_SIZE];
    int n_bytes;
    char* p_contents;
    bool is_valid;
} CacheEntry;

typedef struct
{
    CacheEntry entries[CACHE_SIZE];
} Cache;

pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
Cache cache;

void print_cache_specific(bool existsInCache, int entryIndex)
{
    printf("\nCACHE:\n");
    if (existsInCache == 1 && entryIndex < 8)
    {
        printf("===================\n");
        printf("FILENAME: %s\n", cache.entries[entryIndex].filename);
        printf("BYTES: %d\n", cache.entries[entryIndex].n_bytes);
        printf("CONTENTS: %s\n", cache.entries[entryIndex].p_contents);
        printf("===================\n");
    } else
    {
        printf("===================\n");
        printf("FILENAME: FILE DOES NOT EXIST\n");
        printf("BYTES: 0");
        printf("CONTENTS: \n");
        printf("===================\n");
    }
}

void print_cache()
{
    printf("\nCACHE:\n");
    for (int i = 0; i < CACHE_SIZE; i++)
    {
        if (cache.entries[i].is_valid)
        {
            printf("===================\n");
            printf("FILENAME: %s\n", cache.entries[i].filename);
            printf("BYTES: %d\n", cache.entries[i].n_bytes);
            printf("CONTENTS: %s\n", cache.entries[i].p_contents);
            printf("===================\n");
        }
    }
}

int hash_filename(char* filename)
{
    return (int) (filename[0]);
}

void command_load(size_t arg_size, const char* args)
{
    char filename[FILENAME_SIZE];
    for (int i = 0; i < arg_size + 1; i++)
    {
        if (args[i] != ' ')
        {
            filename[i] = args[i];
        } else
        {
            filename[i] = '\0';
        }
    }

    int index = hash_filename(filename) % CACHE_SIZE;

    pthread_mutex_lock(&cache_mutex);
    if (strcmp(cache.entries[index].filename, filename) == 0 && cache.entries[index].is_valid)
    {
        print_cache_specific(true, index);
    } else if (strcmp(cache.entries[index].filename, filename) != 0 || cache.entries[index].is_valid)
    {
        print_cache_specific(false,
                             0);//just using 0 here as a null since we don't use the second paramater in this scenario anyway
    }
    pthread_mutex_unlock(&cache_mutex);
}

void command_store(size_t arg_size, const char* args)
{
    char filename[FILENAME_SIZE];
    char n_bytes_str[8];
    char* contents = (char*) malloc(FILE_CONTENT_SIZE);

    int step = 0;   // 0: filename, 1: n_bytes_str, 2: contents
    int offset = 0; // offset from last arg
    for (int i = 0; i < arg_size + 1; i++)
    {
        // Parsing filename
        if (step == 0 && args[i] != ' ')
        {
            filename[i] = args[i];
        } else if (step == 0)
        {
            filename[i] = '\0';
            i++;
            offset = i;
            step++;
        }

        // Parsing n_bytes_str
        if (step == 1 && args[i] != ':')
        {
            n_bytes_str[i - offset] = args[i];
        } else if (step == 1)
        {
            n_bytes_str[i - offset] = '\0';
            i++;
            offset = i;
            step++;
        }

        // Parsing contents
        if (step == 2)
        {
            contents[i - offset] = args[i];
        }
    }

    CacheEntry entry;
    entry.is_valid = true;
    strcpy(entry.filename, filename);
    entry.n_bytes = atoi(n_bytes_str);
    entry.p_contents = contents;

    int index = hash_filename(filename) % CACHE_SIZE;

    pthread_mutex_lock(&cache_mutex);
    if (cache.entries[index].is_valid)
    {
        free(cache.entries[index].p_contents);
    }
    cache.entries[index] = entry;
    pthread_mutex_unlock(&cache_mutex);
}

void command_remove(size_t arg_size, const char* args)
{
    char filename[FILENAME_SIZE];
    for (int i = 0; i < FILENAME_SIZE || i < arg_size; i++)
    {
        filename[i] = args[i];
    }

    int index = hash_filename(filename) % CACHE_SIZE;

    pthread_mutex_lock(&cache_mutex);
    if (strcmp(cache.entries[index].filename, filename) == 0 && cache.entries[index].is_valid)
    {
        cache.entries[index].is_valid = false;
        printf("===================\n");
        printf("SUCCESSFULLY REMOVED FILE: %s\n", cache.entries[index].filename);
        printf("===================\n");
    } else if ((strcmp(cache.entries[index].filename, filename) != 0 ||
                ((strcmp(cache.entries[index].filename, filename) == 0 && cache.entries[index].is_valid == false))))
    {
        printf("===================\n");
        printf("SORRY BUT THE FILE YOU ARE ATTEMPTING TO REMOVE DOES NOT EXIST.\n");
        printf("PLEASE CHECK SPELLING.\n");
        printf("THE FILE NAME YOU TYPED WAS: %s\n", filename);
        printf("===================\n");
    }
    pthread_mutex_unlock(&cache_mutex);
}

void process_input(size_t input_size, const char* input)
{
    // receiveLine[bytesRead] = 0; // Make sure we put the null terminator at the end of the buffer
    // printf("Received %d bytes from server with message: %s\n", bytesRead, receiveLine);

    char command_buffer[10];
    int command_size = 0; // NOTE: Might be off by 1
    char args_buffer[100];
    bool is_command = true;
    for (int i = 0; i < input_size; i++)
    {
        if (is_command)
        {
            if (input[i] != ' ')
            {
                command_buffer[i] = input[i];
            } else
            {
                command_buffer[i] = '\0';
                is_command = false;
                command_size = i;
            }
        } else
        {
            if (input[i] != '\0')
            {
                args_buffer[i - command_size - 1] = input[i];
            } else
            {
                args_buffer[i - command_size - 1] = '\0';
                break;
            }
        }
    }

    if (strcmp(command_buffer, "load") == 0)
    {
        command_load(strlen(args_buffer), args_buffer);
    }
    if (strcmp(command_buffer, "store") == 0)
    {
        command_store(strlen(args_buffer), args_buffer);
    }
    if (strcmp(command_buffer, "rm") == 0)
    {
        command_remove(strlen(args_buffer), args_buffer);
    }
}

int main(int argc, char* argv[])
{
    memset(&cache, 0, sizeof(Cache));

    int serverSocket, bytesRead;

    // These are the buffers to talk back and forth with the server
    char sendLine[BUF_SIZE];
    char receiveLine[BUF_SIZE] = "store filename 9:qwertyuio";

    // // Create socket to server
    // if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    // {
    //     printf("Unable to create socket\n");
    //     return -1;
    // }

    // // Setup server connection
    // struct sockaddr_in serverAddress;
    // bzero(&serverAddress, sizeof(serverAddress)); // Ensure address is blank

    // // Setup the type of connection and where the server is to connect to
    // serverAddress.sin_family = AF_INET;          // AF_INET - talk over a network, could be a local socket
    // serverAddress.sin_port = htons(SERVER_PORT); // Conver to network byte order

    // // Try to convert character representation of IP to binary
    // if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0)
    // {
    //     printf("Unable to convert IP for server address\n");
    //     return -1;
    // }

    // // Connect to server, if we cannot connect, then exit out
    // if (connect(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    // {
    //     printf("Unable to connect to server");
    // }

    // // snprintf allows you to write to a buffer, think of it as a formatted print into an array
    // snprintf(sendLine, sizeof(sendLine), "Hello Server");

    // // Write will actually write to a file (in this case a socket) which will transmit it to the server
    // write(serverSocket, sendLine, strlen(sendLine));

    // Now start reading from the server
    // Read will read from socket into receiveLine up to BUF_SIZE
    // while ((bytesRead = read(serverSocket, receiveLine, BUF_SIZE)) > 0)
    process_input(26, "store filename 9:qwertyuio");
    process_input(21, "store abc 9:qwertyuio");
    process_input(21, "store agl 9:qwertyuio");
    process_input(25, "store charles 9:qwertyuip");

    process_input(13, "load charles");

    process_input(12, "rm filename");


    // Close the server socket
    // close(serverSocket);
}
