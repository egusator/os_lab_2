#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstring>
#include <csignal>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int clientSocketFd;
    struct sockaddr_in serverAddress;

    if ((clientSocketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    if (connect(clientSocketFd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Connection failed");
        return -1;
    }

    char message[] = "allo kak slishno priem priem";
    if (send(clientSocketFd, message, strlen(message), 0) < 0) {
        perror("Message sending failed");
        return -1;
    }

    printf("Message sent successfully\n");

    close(clientSocketFd);

    return 0;
}