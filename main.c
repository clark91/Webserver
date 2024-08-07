#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <winsock2.h>
#include <process.h>
#include <ws2tcpip.h>
#include "httpfuncs.h"

#define DEFAULT_PORT "9999"
#define DEFAULT_BUFLEN 512

void sendFile(SOCKET socket, char *resource)
{
    //Direct requested resource to the src/ folder
    char correctedRes[2048] = "src";
    strncat(correctedRes, resource, strlen(resource));

    FILE *sendFile = fopen(correctedRes, "rb");
    if (sendFile == NULL)
    {
        send(socket, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n", 46, 0);
    }
    else
    {
        //Find the length of the file
        fseek(sendFile, 0L, SEEK_END);
        size_t size = ftell(sendFile);
        rewind(sendFile);

        //Create a buffer to store the data in the file
        char *fileBuf = malloc(size + 1);
        if (fileBuf == NULL)
        {
            printf("Malloc failed\n");
            fclose(sendFile);
            return;
        }

        size_t bytesRead = fread(fileBuf, 1, size, sendFile);
        if (bytesRead != size)
        {
            printf("Incorrect number of Bytes read\n");
            fclose(sendFile);
            free(fileBuf);
            return;
        }

        fclose(sendFile);
        fileBuf[size] = '\0'; 

        //Convert the size of the file to a string for use in the HTTP response
        char sizeStr[20];
        snprintf(sizeStr, 20, "%d", size);

        char *fileType = findMsgType(correctedRes);

        //Create a buffer for the full HTTP response
        long msgBufSize = size + 54 + strlen(sizeStr) + strlen(fileType);
        char *msgBuf = malloc(msgBufSize);
        //HTTP headers
        snprintf(msgBuf, msgBufSize,
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Length: %s\r\n"
                 "Content-Type: %s\r\n"
                 "\r\n\0",
                 sizeStr, fileType);

        //Append the HTTP body
        memcpy(&msgBuf[strlen(msgBuf)], fileBuf, size);

        //Send the HTTP response
        int sendRes = send(socket, msgBuf, msgBufSize, 0);
        if (sendRes == SOCKET_ERROR)
        {
            printf("send failed: %d\n", WSAGetLastError());
        }

        free(fileBuf);
        free(msgBuf);
    }
}

void mngSocket(SOCKET ListenSocket)
{
    SOCKET ClientSocket;

    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET)
    {
        printf("accept failed: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
    }

    char recvbuf[DEFAULT_BUFLEN];
    int iSendResult, iResult;
    int recvbuflen = DEFAULT_BUFLEN;

    do
    {
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0)
        {
            //If requested resource is "/", direct to index.html
            struct request content = parseReq(recvbuf);
            if (strcmp("/", content.resource) == 0)
            {
                free(content.resource);
                content.resource = strdup("/index.html");
            }
            //Handle all other requested resources
            if (strcmp(content.type, "GET") == 0)
            {
                sendFile(ClientSocket, content.resource);
            }
        }
        else if (iResult == 0)
        {
            printf("Closing Connection...\n");
        }
        else
        {
            // printf("recv failed: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            break;
        }

    } while (iResult != 0);
}

int main(int argc, char const *argv[])
{
    //Initialise Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStrtrup failed");
        return 1;
    }

    struct addrinfo *result = NULL, *ptr = NULL, hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_addr = INADDR_ANY;

    char hostname[1024];
    hostname[1023] = '\0';
    int iResult = gethostname(hostname, 1023);
    if (iResult != 0)
    {
        printf("Failed to resolve hostname\n");
        WSACleanup();
        return 1;
    } // Resolves hostname

    iResult = getaddrinfo(hostname, DEFAULT_PORT, &hints, &result); // For Localhost replace hostname with NULL and vice versa
    if (iResult != 0)
    {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    } // Gets addrinfo for the IP, port and protocols provided

    SOCKET ListenSocket = INVALID_SOCKET;

    //Create an IPv4 TCP stream socket
    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET)
    {
        printf("Error at socket():  %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
    
    //Create timeout for recv function
    DWORD timeout = 1000 * 0.2;
    setsockopt(ListenSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));

    //Binds local address to the socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("Bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    while (TRUE)
    {
        if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
        {
            printf("Listen failed with error: %ld\n", WSAGetLastError());
            closesocket(ListenSocket);
        }
        else
        {
            mngSocket(ListenSocket);
        }
    }

    printf("Closing program...\n");
    freeaddrinfo(result);
    closesocket(ListenSocket);

    WSACleanup();
    return 0;
}
