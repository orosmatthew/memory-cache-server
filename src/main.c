#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERVER_PORT 1052 // we have 1050 range
#define BUF_SIZE 256

int main(int argc, char *argv[])
{
    int serverSocket, bytesRead;

    // These are the buffers to talk back and forth with the server
    char sendLine[BUF_SIZE];
    char receiveLine[BUF_SIZE] = "rm filename";

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
    //while ((bytesRead = read(serverSocket, receiveLine, BUF_SIZE)) > 0)
    {
        bytesRead = 26;
        //receiveLine[bytesRead] = 0; // Make sure we put the null terminator at the end of the buffer
        //printf("Received %d bytes from server with message: %s\n", bytesRead, receiveLine);

        char command_buffer[10];
        for (int i = 0; i < bytesRead; i++)
        {
            if (receiveLine[i] != ' ')
            {
                command_buffer[i] = receiveLine[i];
            }
            else
            {
                command_buffer[i] = '\0';
            }
        }

        if (strcmp(command_buffer, "load") == 0)
        {
            printf("LOAD\n");
        }
        if (strcmp(command_buffer, "store") == 0)
        {
            printf("STORE\n");
        }
        if (strcmp(command_buffer, "rm") == 0)
        {
            printf("DELETE\n");
        }

        // Got response, get out of here
        //break;
    }

    // Close the server socket
    //close(serverSocket);
}
