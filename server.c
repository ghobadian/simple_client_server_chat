#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>

#define BUFFER_SIZE 1024

typedef struct {
    int sock;
    int port;
    volatile int connected;
    pthread_mutex_t lock;
} client_descriptor;

int server_sock;
char buffer[1024];
client_descriptor *clients[100];  // Array to store client descriptors
int client_count = 0;  // Number of connected clients
pthread_mutex_t clients_lock = PTHREAD_MUTEX_INITIALIZER;

void disconnect_client(client_descriptor* cd) {
    pthread_mutex_lock(&cd->lock);
    cd->connected = 0;
    close(cd->sock);
    printf("[-]Client %d disconnected.\n", cd->port);
    pthread_mutex_unlock(&cd->lock);
    pthread_mutex_destroy(&cd->lock);
}

void *receive_client_handler(void *args) {
    client_descriptor* cd = (client_descriptor*) args;
    int res;
    while(cd->connected) {
        bzero(buffer, BUFFER_SIZE);
        res = recv(cd->sock, buffer, BUFFER_SIZE, 0);
        printf("%d: %s", cd->port, buffer);
        if(res == 0) {
            disconnect_client(cd);
            break;
        }
        sleep(1);
    }
    return NULL;
}

void *send_client_handler(void *args) {
    char input[BUFFER_SIZE];
    while (1) {
        fgets(input, BUFFER_SIZE, stdin);
        bzero(buffer, BUFFER_SIZE);
        strcpy(buffer, input);
        pthread_mutex_lock(&clients_lock);
        for (int i = 0; i < client_count; i++) {
            if (clients[i]->connected) {
                send(clients[i]->sock, buffer, strlen(buffer), 0);
            }
        }
        pthread_mutex_unlock(&clients_lock);
    }
    return NULL;
}

void greet_client(int client_sock, char *buffer, int client_port) {
    bzero(buffer, 1024);
    sprintf(buffer, "Greetings, user %d\n", client_port);
    send(client_sock, buffer, strlen(buffer), 0);
}

void check_server_error(int server_sock) {
    if (server_sock < 0) {
        perror("socket error");
        exit(1);
    }
}

void set_reuse_address_option(const int *server_sock) {
    int opt = 1;
    if (setsockopt(*server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(1);
    }
}

void start_server(int *server_sock) {
    char *ip = "127.0.0.1";
    int port = 5566;
    struct sockaddr_in server_address;
    (*server_sock) = socket(AF_INET, SOCK_STREAM, 0);
    check_server_error((*server_sock));
    printf("TCP server socket created\n");

    set_reuse_address_option(server_sock);

    memset(&server_address, '\0', sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = port;
    server_address.sin_addr.s_addr = inet_addr(ip);
    int n = -1;
    while (n < 0) {
        n = bind((*server_sock), (struct sockaddr*)&server_address, sizeof(server_address));
        sleep(1);
    }
    printf("[+]Bind to the port number: %d\n", port);

    listen((*server_sock), 5);
    printf("Listening...\n");
}

void gracefully_close_server_socket(int signal) {
    printf("Caught signal %d\n", signal);
    close(server_sock);
    exit(0);
}

void *client_handler(void *args) {
    client_descriptor *cd = (client_descriptor *)args;

    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_client_handler, cd);

    pthread_join(receive_thread, NULL);

    pthread_mutex_lock(&clients_lock);
    for (int i = 0; i < client_count; i++) {
        if (clients[i] == cd) {
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_lock);

    free(cd);
    return NULL;
}

int main() {
    signal(SIGINT, gracefully_close_server_socket);  // Catch Ctrl+C
    signal(SIGTERM, gracefully_close_server_socket); // Catch kill signals

    struct sockaddr_in client_address;
    socklen_t address_size;

    start_server(&server_sock);

    pthread_t send_thread;
    pthread_create(&send_thread, NULL, send_client_handler, NULL);

    while(true) {
        int client_sock;
        address_size = sizeof(client_address);
        client_sock = accept(server_sock, (struct sockaddr*)&client_address, &address_size);
        int client_port = client_address.sin_port;
        printf("[+]Client %d connected.\n", client_port);

        greet_client(client_sock, buffer, client_port);

        client_descriptor *cd = (client_descriptor *)malloc(sizeof(client_descriptor));
        cd->sock = client_sock;
        cd->port = client_port;
        cd->connected = 1;
        pthread_mutex_init(&cd->lock, NULL);

        pthread_mutex_lock(&clients_lock);
        clients[client_count++] = cd;
        pthread_mutex_unlock(&clients_lock);

        pthread_t client_thread;
        pthread_create(&client_thread, NULL, client_handler, (void *)cd);
        pthread_detach(client_thread);
    }

    pthread_join(send_thread, NULL);
    return 0;
}
