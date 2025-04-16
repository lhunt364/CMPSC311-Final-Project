//Name: Brayden Aubry
//Class: CMPSC 311
//Assignment: Project (Client Side)
//Date: IDK


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>     // windows socket functions
#include <ws2tcpip.h>     // extra socket utilities
#include <windows.h>      // needed for winmain types
#include <process.h>      // for creating threads

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define MAX_MSG_LEN 1024

SOCKET sockfd;

// thread that receives messages from the server
unsigned __stdcall receive_messages(void* arg) {
    char buffer[MAX_MSG_LEN];

    while (1) {
        int bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            printf("disconnected from server.\n");
            closesocket(sockfd);
            WSACleanup();
            exit(1);
        }

        buffer[bytes] = '\0';
        printf("\n[server]: %s", buffer);
        printf("you: ");
        fflush(stdout);
    }
    return 0;
}

int main() {
    WSADATA wsa;
    struct sockaddr_in server_addr;
    char message[MAX_MSG_LEN];
    uintptr_t recv_thread;

    // start winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("winsock init failed. error: %d\n", WSAGetLastError());
        return 1;
    }

    // create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        printf("socket creation failed. error: %d\n", WSAGetLastError());
        return 1;
    }

    // setup server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("connection failed. error: %d\n", WSAGetLastError());
        return 1;
    }

    printf("connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);

    // create thread to receive messages
    recv_thread = _beginthreadex(NULL, 0, receive_messages, NULL, 0, NULL);
    if (recv_thread == 0) {
        perror("thread failed");
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    // main loop to send messages
    while (1) {
        printf("you: ");
        if (fgets(message, MAX_MSG_LEN, stdin) != NULL) {
            send(sockfd, message, strlen(message), 0);
        }
    }

    closesocket(sockfd);
    WSACleanup();
    return 0;
}

// fixes linker error for windows gui mode
#ifdef _WIN32
int WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow) {
    return main();
}
#endif
