#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <winsock2.h>
#include <process.h>
#include <ws2tcpip.h>
#include "httpfuncs.h"

#define DEFAULT_PORT "9999"
#define DEFAULT_BUFLEN 512


void sendFile(SOCKET socket, char* resource){
    resource++;
    //printf("Resource requested: %s\n", resource);
    FILE *sendFile = fopen(resource, "rb");
    if(sendFile == NULL){
        //printf("File not found: %s\n", resource);
        send(socket, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n", 46, 0);
        fclose(sendFile); 
    }
    else{
        fseek(sendFile, 0L, SEEK_END);
        size_t size = ftell(sendFile);
        rewind(sendFile);

        char *fileBuf = malloc(size+1);
        size_t bytesRead = fread(fileBuf, 1, size, sendFile);
        fclose(sendFile);
        fileBuf[size] = '\0';

        char sizeStr[20];
        sprintf(sizeStr,"%d", size);

        char *fileType = findMsgType(resource);

        long msgBufSize = size + 54 + strlen(sizeStr) + strlen(fileType);
        char *msgBuf = malloc(msgBufSize);

        snprintf(msgBuf, msgBufSize,
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: %s\r\n"
        "Content-Type: %s\r\n"
        "\r\n"
        "%s", sizeStr, fileType, fileBuf); 

        int sendRes = send(socket, msgBuf, strlen(msgBuf), 0);

        if (sendRes == SOCKET_ERROR) {
            printf("send failed: %d\n", WSAGetLastError());
        }

        //printf("%s", msgBuf);

        free(fileBuf);
        free(msgBuf);
    }
}



int main(int argc, char const *argv[])
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStrtrup failed");
        return 1;
    }

    struct addrinfo *result = NULL, *ptr = NULL, hints;

    ZeroMemory(&hints, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;


    int iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    SOCKET ListenSocket = INVALID_SOCKET;
    
    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (ListenSocket == INVALID_SOCKET){
        printf("Error at socket():  %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    //printf("Socket is: %s\n", ListenSocket);

    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR){
        printf("Bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    if ( listen( ListenSocket, SOMAXCONN ) == SOCKET_ERROR ) {
    printf( "Listen failed with error: %ld\n", WSAGetLastError() );
    closesocket(ListenSocket);
    WSACleanup();
    return 1;
    }

    SOCKET ClientSocket;

    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    char recvbuf[DEFAULT_BUFLEN];
    int iSendResult;
    int recvbuflen = DEFAULT_BUFLEN;
           
    do
    {
        //printf("Waiting for MSG\n");
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if(iResult > 0){
            //printf("Bytes received: %d\n\n", iResult);
            //printf("%s\n\n\n", recvbuf);

            struct request content = parseReq(recvbuf);
            if(strcmp("/", content.resource) == 0){
                free(content.resource);
                content.resource = malloc(sizeof("/index.html"));
                content.resource = "/index.html";
            }

            if (strcmp(content.type, "GET") == 0){
                sendFile(ClientSocket, content.resource);
            }

            
        } else if (iResult == 0){
            printf("Closing Connection...\n");
        }
        else{
            printf("recv failed: %d\n", WSAGetLastError());
            break;
        }
        

    } while (iResult != 0);


    printf("Closing program...\n");
    freeaddrinfo(result);
    closesocket(ListenSocket);
    WSACleanup();

    

    return 0;
}

