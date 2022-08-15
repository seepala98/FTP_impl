#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>

#include "clientHeader.h"

// cmdRunner is used to run the commands from the client side
int cmdRunner(char *buffer, char *command, char *input, int server_fd, int *data_port)
{

    // initialize the buffer and input length values
    int buff_len = strlen(buffer);
    int input_len = strlen(input);

    // check if the command is valid and equal to USER to send username to the server side
    if (strcmp(command, "USER") == 0 && input_len != 0)
    {
        send(server_fd, buffer, buff_len, 0);
        displayHelper(server_fd);
    }
    // check if the command is valid and equal to PASS to send password to the server side
    else if (strcmp(command, "PASS") == 0 && input_len != 0)
    {
        send(server_fd, buffer, buff_len, 0);
        displayHelper(server_fd);
    }
    // check if the command is valid and equa to STOR to send a file to server side
    else if (strcmp(command, "STOR") == 0 && input_len != 0)
    {
        FILE *fptr = fopen(input, "r");
        // check if the file exist and can be opened
        if (!fptr)
        {
            perror("550 No such file or directory.");
        }
        else
        {
            int srv_socket = portClient(server_fd, data_port);
            if (srv_socket == -1)
                return 0;
            displayHelper(server_fd);

            // send response back to the server side to start the data transfer
            send(server_fd, buffer, buff_len, 0);
            char message[BUFFER_SIZE];
            bzero(&message, BUFFER_SIZE);
            if (recv(server_fd, message, sizeof(message), 0) < 0)
            {
                perror("Recv");
            }
            else if (strcmp(message, "ready") == 0)
            {
                storHelperClientSide(input, srv_socket);
            }
            else
            {
                printf("%s\n", message);
            }
        }
    }
    // check if the command is RETR to retrive data from the serverside to client side
    else if (strcmp(command, "RETR") == 0 && input_len != 0)
    {

        // initialize the server socket and data port values
        int srv_socket = portClient(server_fd, data_port);
        if (srv_socket == -1)
            return 0;

        displayHelper(server_fd);

        // send response back to the server side to start the data transfer
        send(server_fd, buffer, buff_len, 0);
        char message[BUFFER_SIZE];
        bzero(&message, BUFFER_SIZE);
        // check if the message is valid and equal to ready to start the data transfer
        if (recv(server_fd, message, sizeof(message), 0) < 0)
        {
            perror("Recv");
        }
        else if (strcmp(message, "150 File status okay; about to open data connection.") == 0)
        {
            retrHelperClient(input, srv_socket);
        }
        // when the file or directory doesnt exist
        else if (strcmp(message, "550 No such file or directory.") == 0)
        {
            printf("550 No such file or directory.\n");
        }
        else
        {
            printf("530 Not logged in.\n");
        }
    }
    // check if the command passed is LIST to list the contents of the server side
    else if (strcmp(command, "LIST") == 0)
    {

        // initialize the server socket and data port values
        int srv_socket = portClient(server_fd, data_port);
        if (srv_socket == -1)
            return 0;
        displayHelper(server_fd);

        // send response back to the server side to start the data transfer
        send(server_fd, buffer, buff_len, 0);
        char message[BUFFER_SIZE];
        bzero(&message, BUFFER_SIZE);
        if (recv(server_fd, message, sizeof(message), 0) < 0)
        {
            perror("Recv");
        }
        else if (strcmp(message, "ls_pwd") == 0)
        {
            listHelperPwdClient(command, srv_socket);
            printf("226 Transfer completed.\n");
        }
        else if (strcmp(message, "non-existed") == 0)
        {
            printf("%s: no such file on server\n", input);
        }
        else
        {
            printf("%s\n", message);
        }
    }
    // check if the command passed is CWD or PWD and send the response to the server side
    else if (strcmp(command, "CWD") == 0 || strcmp(command, "PWD") == 0)
    {

        send(server_fd, buffer, buff_len, 0);
        displayHelper(server_fd);
    }
    // check if the command is not LIST or PWD and send the response to the server side
    else if (strcmp(command, "!LIST") == 0 || strcmp(command, "!PWD") == 0)
    {
        if (strcmp(command, "!LIST") == 0)
            system("ls");
        else
        {
            printf(">>>Your current directory:");
            fflush(stdout);
            system("pwd");
        }
    }
    else if ((strcmp(command, "!CWD") == 0) && input_len != 0)
    {
        if (chdir(input) == -1)
            perror("chdir");
    }
    // check if the command is NOOP and receive a OK response using the noopHelper()
    else if (strcmp(command, "NOOP") == 0)
    {
        send(server_fd, buffer, buff_len, 0);
        noopHelperClient(server_fd);
    }
    // check if the command is DELE and send the response to the server side.
    else if (strcmp(command, "DELE") == 0)
    {
        send(server_fd, buffer, buff_len, 0);
        displayHelper(server_fd);
    }
    // check if the command is STAT to get the status of the file from server side
    else if (strcmp(command, "STAT") == 0)
    {
        send(server_fd, buffer, buff_len, 0);
        displayHelper(server_fd);
    }
    // check if the command is QUIT to quit the program and close the connection to the server
    else if (strcmp(command, "QUIT") == 0)
    {
        send(server_fd, buffer, buff_len, 0);
        displayHelper(server_fd);

        close(server_fd);
        return -1;
    }
    else
    {
        send(server_fd, buffer, buff_len, 0);
        displayHelper(server_fd);
    }

    return 0;
}

// function to transfer the file from the client side to the server side
int storHelperClientSide(char *filename, int srv_socket)
{
    FILE *fptr = fopen(filename, "r");
    if (!fptr)
    {
        perror("Cant open the file");
    }
    else
    {
        // GET file size
        fseek(fptr, 0, SEEK_END);
        long long int file_size = ftell(fptr);
        rewind(fptr);

        int bytes = 0;
        char buffer[1500];
        bzero(buffer, sizeof(buffer));

        // write the data to bytes
        for (int i = 0; i < file_size / 1500; i++)
        {
            fread(buffer, sizeof(buffer), 1, fptr);
            bytes += write(srv_socket, buffer, sizeof(buffer));
        }
        fread(buffer, file_size % 1500, 1, fptr);
        bytes += write(srv_socket, buffer, file_size % 1500);

        printf("226 Transfer completed.\n");
    }

    fclose(fptr);
    close(srv_socket);
    return 0;
}

// displayHelper is used to the status of the command ran on client side
void displayHelper(int server_fd)
{
    char message[BUFFER_SIZE];
    bzero(&message, BUFFER_SIZE);
    if (recv(server_fd, message, sizeof(message), 0) < 0)
    {
        perror("Recv");
        exit(-1);
    }
    else
    {
        printf("%s \n", message);
    }
}

// noop command helper  This command does not affect any parameters or previously
// entered commands. It specifies no action other than that the
// server send an OK reply.

void noopHelperClient(int server_fd)
{
    char message[BUFFER_SIZE];
    bzero(&message, BUFFER_SIZE);
    if (recv(server_fd, message, sizeof(message), 0) < 0)
    {
        perror("Recv");
        exit(-1);
    }
    else
    {
        printf("OK \n");
        printf("%s \n", message);
    }
}

// DELE command helper This command causes the file specified in the pathname to be deleted at the server.
// If the file does not exist, the server returns a 550 error code.
void deleHelperClient(char *filename, int server_fd)
{
    char buffer[BUFFER_SIZE];
    bzero(&buffer, BUFFER_SIZE);
    sprintf(buffer, "DELE %s", filename);
    send(server_fd, buffer, strlen(buffer), 0);
    displayHelper(server_fd);
}

// function to retrieve the file from the server side to the client side
int retrHelperClient(char *filename, int srv_socket)
{

    send(srv_socket, filename, strlen(filename), 0);
    char buffer[1500];
    int bytes;
    long long int size = 0;
    FILE *fptr;
    // check if the file is write accessible
    if (!(fptr = fopen(filename, "w")))
        perror("Cant create file");
    else
    {
        do
        {
            bytes = recv(srv_socket, buffer, sizeof(buffer), 0);
            if (bytes > 0)
            {
                fwrite(buffer, bytes, 1, fptr);
                size += bytes;
            }
        } while (bytes > 0);
        fclose(fptr);
        printf("226 Transfer completed.\n");
    }
    // close the socket and return 0
    close(srv_socket);
    return 0;
}

// function used to list and pwd
int listHelperPwdClient(char *command, int srv_socket)
{
    char recvline[BUFFER_SIZE + 1];
    bzero(recvline, (int)sizeof(recvline));
    while (read(srv_socket, recvline, BUFFER_SIZE) > 0)
    {
        printf("%s", recvline);
        bzero(recvline, (int)sizeof(recvline));
    }
    close(srv_socket);
    return 0;
}

// function to create a port and use that for data transfer
int portClient(int server_fd, int *data_port)
{
    // arithemtic to deal with port numbers
    int p1 = *data_port / 256;
    int p2 = *data_port % 256;

    char hostInfo[256];
    char *IPInfo;
    struct hostent *host_entry;

    char port_command[BUFFER_SIZE];

    gethostname(hostInfo, sizeof(hostInfo));

    // To retrieve host information
    host_entry = gethostbyname(hostInfo);

    // To convert an Internet network address into ASCII string
    IPInfo = inet_ntoa(*((struct in_addr *)host_entry->h_addr_list[0]));

    int h_ip[4];
    sscanf(IPInfo, "%d.%d.%d.%d", &h_ip[0], &h_ip[1], &h_ip[2], &h_ip[3]);

    sprintf(port_command, "PORT %d,%d,%d,%d,%d,%d", h_ip[0], h_ip[1], h_ip[2], h_ip[3], p1, p2);

    // send
    send(server_fd, port_command, strlen(port_command), 0);
    char message[BUFFER_SIZE];
    bzero(&message, BUFFER_SIZE);
    if (recv(server_fd, message, sizeof(message), 0) < 0)
    {
        perror("Recv");
    }
    else if (strcmp(message, "ready") != 0)
    {
        printf("%s\n", message);
        return -1;
    }

    int srv_socket = tcpHelper(data_port, IPInfo);

    while (srv_socket == -1)
    {
        srv_socket = tcpHelper(data_port, IPInfo);
    }
    *data_port = *data_port + 1;
    return srv_socket;
}

// function to create a tcp socket and return the socket descriptor to be used by portClient during data transfer
int tcpHelper(int *data_port, char *IPInfo)
{
    int srv_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in srv_addr;            // structure to hold the type, address and port
    memset(&srv_addr, 0, sizeof(srv_addr)); // set the Fill the structure with 0s
    srv_addr.sin_family = AF_INET;          // Address family
    srv_addr.sin_port = htons(*data_port);  // Port Number
    srv_addr.sin_addr.s_addr = inet_addr(IPInfo);

    // connect
    if (connect(srv_socket, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) == -1)
    {
        return -1;
    }
    else
        return srv_socket;
}