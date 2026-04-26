#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#include "digexfun.hpp"
#pragma comment(lib, "ws2_32.lib")

FP_PRINTMSG pPrintFnc = NULL;
FP_STOPSIM pStopSimFnc = NULL;

void __stdcall Init(FP_PRINTMSG _pPrintFnc, FP_STOPSIM _pStopSimFnc) {
    pPrintFnc = _pPrintFnc;
    pStopSimFnc = _pStopSimFnc;
    emitMessage(">>> DLL Init() called: digexfun.dll is loaded");
}

void print_pf(const char* msg, unsigned int msgType) {
    if (pPrintFnc) (*pPrintFnc)(msg, msgType);
}

void stop_simulation() {
    if (pStopSimFnc) pStopSimFnc();
}

void __stdcall ExchangeViaTCP(void) {
    double dbool = pop();
    double x = pop();
    double y = pop();

    emitMessage(">>> ExchangeViaTCP() called");

    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in server;
    const char* server_ip = "127.0.0.1";
    int port = 9090;
    double result = 0.0;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        emitMessage("WSAStartup failed");
        push(0.0);
        return;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        emitMessage("Socket creation failed");
        WSACleanup();
        push(0.0);
        return;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        emitMessage("Connection to external TCP server failed");
        closesocket(sock);
        WSACleanup();
        push(0.0);
        return;
    }

    if (send(sock, (char*)&dbool, sizeof(double), 0) != sizeof(double)) {
        emitMessage("Failed to send data over TCP");
        closesocket(sock);
        WSACleanup();
        push(0.0);
        return;
    }

    int bytesReceived = recv(sock, (char*)&result, sizeof(double), 0);
    if (bytesReceived != sizeof(double)) {
        emitMessage("Failed to receive data over TCP");
        result = 0.0;
    }

    closesocket(sock);
    WSACleanup();
    push(result);
}

int __stdcall RegisterFunctions(int ifun, char* cnam, int* iargc) {
    emitMessage(">>> RegisterFunctions() called");

    if (!cnam || !iargc) return 1;
    LoadDigsiLibrary();

    if (ifun == 0) {
        strcpy(cnam, "ExchangeViaTCP");
        *iargc = 3;
    }
    else {
        return 1;
    }

    return 0;
}
