#ifndef __WINAPI_SOCKET_TEMPLATE_H__
#define __WINAPI_SOCKET_TEMPLATE_H__

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")


/*
    A template for opening a non-blocking WINAPI socket.
*/

int init_nb_socket()
{
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        fprintf(stderr, "Failed to init sockets: %i\n", iResult);
        return iResult;
    }

    return 0;
}


SOCKET open_nb_socket(const char* addr, const char* port)
{
    struct addrinfo hints = {0};

    hints.ai_family = AF_UNSPEC; /* IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Must be TCP */
    SOCKET sockfd = INVALID_SOCKET;
    int rv;
    struct addrinfo *p, *servinfo;
    int iMode = 1;	/* non-blocking */

    /* get address information */
    rv = getaddrinfo(addr, port, &hints, &servinfo);
    if (rv != 0) {
        fprintf(stderr, "Failed to open socket (getaddrinfo): %s\n", gai_strerror(rv));
        return -1;
    }

    /* open the first possible socket */
    for (p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
            continue;

        /* connect to server */
        rv = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
        if (rv == -1)
            continue;
        break;
    }

    /* free servinfo */
    freeaddrinfo(servinfo);

    /* make non-blocking */
    if (sockfd != INVALID_SOCKET)
        ioctlsocket(sockfd, FIONBIO, &iMode);

    /* return the new socket */
    return sockfd;
}

void close_nb_socket(SOCKET sock)
{
    closesocket(sock);
}

void shutdown_nb_socket()
{
    WSACleanup();
}


#endif
