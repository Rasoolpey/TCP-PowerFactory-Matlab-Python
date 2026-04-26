#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "digexfun.hpp"
#pragma comment(lib, "ws2_32.lib")

// === USER SETTINGS ===
#define NUM_INPUTS  4
#define NUM_OUTPUTS 2
#define MAX_OUTPUTS 10  // This can change based on your need

// === STATIC STATE ===
static double output_buffer[MAX_OUTPUTS] = { 0.0 };
static int current_output_count = 0;

FP_PRINTMSG pPrintFnc = NULL;
FP_STOPSIM pStopSimFnc = NULL;

// === INIT FUNCTION ===
void __stdcall Init(FP_PRINTMSG _pPrintFnc, FP_STOPSIM _pStopSimFnc) {
    pPrintFnc = _pPrintFnc;
    pStopSimFnc = _pStopSimFnc;
    emitMessage(">>> DLL Init() called");
}

// === PRINT/STOP UTILITIES ===
void print_pf(const char* msg, unsigned int msgType) {
    if (pPrintFnc) (*pPrintFnc)(msg, msgType);
}

void stop_simulation() {
    if (pStopSimFnc) pStopSimFnc();
}

// === MAIN FUNCTION: DO TCP ONCE ===
void __stdcall ExchangeViaTCP(void) {
    emitMessage(">>> ExchangeViaTCP() called");

    double inputs[NUM_INPUTS];
    double outputs[NUM_OUTPUTS] = { 0 };

    for (int i = NUM_INPUTS - 1; i >= 0; --i) {
        inputs[i] = pop();
    }

    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in server;
    const char* server_ip = "127.0.0.1";
    int port = 9090;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        emitMessage("WSAStartup failed");
        current_output_count = 0;
        return;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        emitMessage("Socket creation failed");
        WSACleanup();
        current_output_count = 0;
        return;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        emitMessage("Connection to TCP server failed");
        closesocket(sock);
        WSACleanup();
        current_output_count = 0;
        return;
    }

    int bytesSent = send(sock, (char*)inputs, sizeof(inputs), 0);
    if (bytesSent != sizeof(inputs)) {
        emitMessage("Failed to send input data");
        closesocket(sock);
        WSACleanup();
        current_output_count = 0;
        return;
    }

    int bytesReceived = recv(sock, (char*)outputs, sizeof(outputs), 0);
    if (bytesReceived != sizeof(outputs)) {
        emitMessage("Failed to receive output data");
        memset(outputs, 0, sizeof(outputs));
        current_output_count = 0;
    }
    else {
        current_output_count = NUM_OUTPUTS;
        for (int i = 0; i < NUM_OUTPUTS; ++i) {
            output_buffer[i] = outputs[i];
        }

        char msg[256];
        snprintf(msg, sizeof(msg),
            ">>> Received outputs: %.6f, %.6f",
            outputs[0], outputs[1]);
        emitMessage(msg);
    }

    closesocket(sock);
    WSACleanup();
}

// === GENERAL-PURPOSE GET FUNCTION ===
void __stdcall GetOutputIndex(void) {
    double idx_raw = pop();
    int idx = static_cast<int>(idx_raw);

    if (idx >= 0 && idx < current_output_count) {
        push(output_buffer[idx]);
    }
    else {
        emitMessage(">>> Invalid index in GetOutputIndex()");
        push(0.0);
    }
}

// === FUNCTION REGISTRATION ===
int __stdcall RegisterFunctions(int ifun, char* cnam, int* iargc) {
    emitMessage(">>> RegisterFunctions() called");

    if (!cnam || !iargc) return 1;
    LoadDigsiLibrary();

    if (ifun == 0) {
        strcpy(cnam, "ExchangeViaTCP");
        *iargc = NUM_INPUTS;
    }
    else if (ifun == 1) {
        strcpy(cnam, "GetOutputIndex");
        *iargc = 1;
    }
    else {
        return 1;
    }

    return 0;
}
