#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstring>
#include <csignal>
#include <cerrno>

#define CLIENTS_AMOUNT 1
#define PORT 8080

volatile sig_atomic_t wasSighup = 0;

void sighupHandler(int signalNumber) {
    if (signalNumber == SIGHUP) {
        wasSighup = 1;
    }
}

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Socket creation error");
        return 1;
    }

    struct sigaction sa{};
    sigaction(SIGHUP, nullptr, &sa);
    sa.sa_handler = sighupHandler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGHUP, &sa, nullptr);

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    memset(serverAddress.sin_zero, 0, sizeof(serverAddress.sin_zero));

    if (bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == -1) {
        perror("Socket binding error");
        return 1;
    }

    if (listen(serverSocket, CLIENTS_AMOUNT) == -1) {
        perror("Listening error");
        return 1;
    }

    sigset_t blockedMask, origMask;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blockedMask, &origMask);

    int clientSocket = -1;
    int socketForAcceptingConnections;

    while (true) {
        fd_set readFileDescriptors;
        FD_ZERO(&readFileDescriptors);
        FD_SET(serverSocket, &readFileDescriptors);

        if(clientSocket != -1) {
            FD_SET(clientSocket, &readFileDescriptors);
        }

        int maxFd;
        if (serverSocket > clientSocket) {
            maxFd = serverSocket;
        } else {
            maxFd = clientSocket;
        }

        if (pselect(maxFd + 1, &readFileDescriptors, nullptr, nullptr, nullptr, &origMask) == -1) {
            if (errno == EINTR) {
                if (wasSighup) {
                    printf("Received SIGHUP");
                    break;
                } else {
                    perror("EINTR, but not sighup");
                    return -1;
                }
            } else {
                perror("Pselect unknown error.");
                return -1;
            }
        }

        if (FD_ISSET(serverSocket, &readFileDescriptors)) {
            if ((socketForAcceptingConnections = accept(serverSocket, NULL, NULL)) == -1) {
                perror("accept error");
                return -1;
            }

            if (clientSocket == -1) {
                clientSocket = socketForAcceptingConnections;
                printf("new client, socket num = %d \n", clientSocket);
            } else {
                close(socketForAcceptingConnections);
            }
        }

        if (clientSocket != -1 && FD_ISSET(clientSocket, &readFileDescriptors)) {
            char buffer[1024];
            ssize_t bytes_received = recv(clientSocket, buffer, sizeof(buffer), 0);

            if (bytes_received > 0) {
                printf("%ld bytes received from socket %d\n", bytes_received, clientSocket);
                printf("Message: %s\n", buffer);
            } else if (bytes_received == 0) {
                printf("Socket %d sent no bytes, closing it\n", clientSocket);
                close(clientSocket);
                clientSocket = -1;
            } else {
                perror("recv unknown error");
                return -1;
            }
        }
    }
    close(serverSocket);
    close(clientSocket);
    return 0;
}