#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

int server_socket;
char buffer[BUFFER_SIZE];

void greet_server();

void connect_to_server();

void *receive_handler(void* args) {
    while (1) {
        bzero(buffer, BUFFER_SIZE);
        int n = recv(server_socket, buffer, BUFFER_SIZE, 0);
        if (n <= 0) {
            perror("[-]Error in receiving data.");
            exit(1);
        }
        printf("Server: %s", buffer);
    }
}

void *send_handler(void* args) {//todo check if removing the args from parameters breaks the program
    char input[BUFFER_SIZE];
    while (1) {
        fgets(input, BUFFER_SIZE, stdin);
        bzero(buffer, BUFFER_SIZE);
        strcpy(buffer, input);
        send(server_socket, buffer, strlen(buffer), 0);
    }
}

int main(){
    connect_to_server();
    greet_server();

    pthread_t send_thread, receive_thread;
    pthread_create(&receive_thread, NULL, receive_handler, NULL);
    pthread_create(&send_thread, NULL, send_handler, NULL);

    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);

    close(server_socket);
    return 0;
}

void connect_to_server() {
    char *ip = "127.0.0.1";
    int server_port = 5566;

    struct sockaddr_in addr;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0){
        perror("[-]Socket error");
        exit(1);
    }
    printf("[+]TCP server socket created.\n");

    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = server_port;
    addr.sin_addr.s_addr = inet_addr(ip);

    connect(server_socket, (struct sockaddr*)&addr, sizeof(addr));
    printf("Connected to the server.\n");
}

void greet_server() {
    bzero(buffer, BUFFER_SIZE);
    strcpy(buffer, "Greetings, server\n");
    send(server_socket, buffer, strlen(buffer), 0);
}

