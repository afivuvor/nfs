#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>

#define MAX_MESSAGE_SIZE 20000

int main(int argc , char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char message[MAX_MESSAGE_SIZE] , server_reply[MAX_MESSAGE_SIZE];
    char command[MAX_MESSAGE_SIZE]; // To store user input
    
    // Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
        return 1;
    }
    puts("Socket created");
    
    server.sin_addr.s_addr = inet_addr("10.10.59.53"); // Server IP address
    server.sin_family = AF_INET;
    server.sin_port = htons(8890); // Server port
    
    // Connect to file system server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }
    
    puts("Connected to file system server\n");
    printf("Welcome to the Afi filesystem! Here's a list of commands you can run:\n"
        "ACCESS file/path\n"
        "FILESTAT file/path\n"
        "CREATEFILE file/path\n"
        "WRITE contentOfFile file/path\n");
        
    // Prompt the user for command
    printf("Enter desired file system command: ");
    fgets(command, sizeof(command), stdin);
    
    // Remove trailing newline character from command
    command[strcspn(command, "\n")] = '\0';
    
    // Send the command to the server
    if( send(sock , command , strlen(command) , 0) < 0)
    {
        puts("Send failed");
        return 1;
    }
    
    // Receive response from the server
    if( recv(sock , server_reply , sizeof(server_reply) , 0) < 0)
    {
        puts("recv failed");
        return 1;
    }
    
    // Process server reply (handle success or error)
    printf("Server reply: %s\n", server_reply);
    
    // Close socket
    close(sock);
    
    return 0;
}
