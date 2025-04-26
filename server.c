/*
File: Server Class
Date: 04/18/2025
*/

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef int socklen_t;
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <unistd.h>
    #include <errno.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    typedef int SOCKET;
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 8080 //Port number for the server
#define MAX_CLIENTS 10 //Maximum number of simultaneous clients
#define BUFFER_SIZE 1024 //Buffer size for messages

//Structure to store client info
typedef struct {
    SOCKET socket;
    struct sockaddr_in address;
    pthread_t thread;
    char username[50];
} Client;

Client *clients[MAX_CLIENTS]; //Array to store clients
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; //Mutex for clients list (Mutex prevents race conditions )
int server_socket; // global for signal handler

void broadcast_message(char *message, SOCKET sender_socket);
void remove_client(SOCKET socket);
void *handle_client(void *arg);
void handle_new_client(SOCKET client_socket, struct sockaddr_in client_addr);

//Adds timestamps when the Server updates
void timestamp_log(const char *msg) {
    time_t now = time(NULL);
    char tbuf[26];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    printf("[%s] %s\n", tbuf, msg);
}

//Handles new client connections
void handle_new_client(SOCKET client_socket, struct sockaddr_in client_addr) {
    char username[50];
    recv(client_socket, username, sizeof(username), 0);//Receives username from the client
    username[strcspn(username, "\n")] = 0;
//Allocates a new Client structure
    Client *new_client = (Client *)malloc(sizeof(Client));
    if (!new_client) {
        perror("Failed to allocate memory");
        return;
    }
//Assigns properties to client
    new_client->socket = client_socket;
    new_client->address = client_addr;
    strcpy(new_client->username, username);

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i]) {
            clients[i] = new_client;
            pthread_create(&new_client->thread, NULL, handle_client, (void *)new_client);
            pthread_mutex_unlock(&clients_mutex);
            return;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    send(client_socket, "Server full. Try again later.\n", 32, 0);
#ifdef _WIN32
    closesocket(client_socket);
#else
    close(client_socket);
#endif
    free(new_client);
}

//Handles messages from individual clients
void *handle_client(void *arg) {
    Client *client = (Client *)arg;
    char buffer[BUFFER_SIZE];
    int bytes_received;

    while ((bytes_received = recv(client->socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("%s: %s", client->username, buffer);
        broadcast_message(buffer, client->socket);
    }
    remove_client(client->socket);
    free(client);
    pthread_exit(NULL);
}

//Sends message to all connected clients
void broadcast_message(char *message, SOCKET sender_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->socket != sender_socket) {
            send(clients[i]->socket, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

//Removes a disconnected client from the list
void remove_client(SOCKET socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->socket == socket) {
            char msg[100];
            snprintf(msg, sizeof(msg), "%s has left the chat.", clients[i]->username);
            timestamp_log(msg);
            close(clients[i]->socket);
            free(clients[i]);
            clients[i] = NULL;
            break;
#ifdef _WIN32
            closesocket(socket);
#else
            close(socket);
#endif
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

//safely shuts the server down when the window is disconnected
void cleanup_server(int signum) {
    timestamp_log("Server shutting down...");

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i]) {
            close(clients[i]->socket);
            free(clients[i]);
            clients[i] = NULL;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    close(server_socket);
    exit(0);
}

//Main function
int main() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialize Winsock. Error Code: %d\n", WSAGetLastError());
        return 1;
    }
#endif
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
//Creates server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
//Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
//Binds socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }
    printf("Server started on port %d\n", PORT);

    timestamp_log("Server started."); //timestamp for server start

//Loop for accepting socket
    while (1) {
        addr_size = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (client_socket == INVALID_SOCKET) {
            perror("Connection failed");
            continue;
        }
        handle_new_client(client_socket, client_addr); //Handles client connection
    }
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
