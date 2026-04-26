// userfun.cpp - Modular TCP DLL for Multiple Inverters/Controllers

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "digexfun.hpp"
#pragma comment(lib, "ws2_32.lib")

#define NUM_COMPONENTS 2
#define MAX_OUTPUTS 10  // must be >= largest numOutputs

struct ControlTarget {
    int port;
    int numInputs;
    int numOutputs;
};

const ControlTarget COMPONENTS[NUM_COMPONENTS] = {
    {9101, 4, 2},  // Component 0
    {9102, 4, 2},  // Component 1
};

static double output_buffer[MAX_OUTPUTS] = { 0.0 };
static int current_output_count = 0;

FP_PRINTMSG pPrintFnc = NULL;
FP_STOPSIM pStopSimFnc = NULL;

void __stdcall Init(FP_PRINTMSG _pPrintFnc, FP_STOPSIM _pStopSimFnc) {
    pPrintFnc = _pPrintFnc;
    pStopSimFnc = _pStopSimFnc;
    emitMessage(">>> DLL Init() called");
}

void stop_simulation() {
    if (pStopSimFnc) pStopSimFnc();
}

void ExchangeViaTCP_Component(int index) {
    if (index < 0 || index >= NUM_COMPONENTS) {
        emitMessage("Invalid component index");
        return;
    }

    const ControlTarget& cfg = COMPONENTS[index];
    double inputs[20];  // enough buffer
    double outputs[MAX_OUTPUTS] = { 0 };

    for (int i = cfg.numInputs - 1; i >= 0; --i) {
        inputs[i] = pop();
    }

    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in server;
    const char* server_ip = "127.0.0.1";

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
    server.sin_port = htons(cfg.port);
    server.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        emitMessage("Connection to TCP server failed");
        closesocket(sock);
        WSACleanup();
        current_output_count = 0;
        return;
    }

    int bytesSent = send(sock, (char*)inputs, sizeof(double) * cfg.numInputs, 0);
    if (bytesSent != sizeof(double) * cfg.numInputs) {
        emitMessage("Failed to send input data");
        closesocket(sock);
        WSACleanup();
        current_output_count = 0;
        return;
    }

    int bytesReceived = recv(sock, (char*)outputs, sizeof(double) * cfg.numOutputs, 0);
    if (bytesReceived != sizeof(double) * cfg.numOutputs) {
        emitMessage("Failed to receive output data");
        memset(outputs, 0, sizeof(outputs));
        current_output_count = 0;
    }
    else {
        current_output_count = cfg.numOutputs;
        for (int i = 0; i < current_output_count; ++i) {
            output_buffer[i] = outputs[i];
        }

        char msg[256];
        snprintf(msg, sizeof(msg), ">>> Component %d outputs: %.4f, %.4f", index, outputs[0], outputs[1]);
        emitMessage(msg);
    }

    closesocket(sock);
    WSACleanup();
}

// Dynamic wrappers generated using macros
#define DEFINE_EXCHANGE_WRAPPER(N) \
    void __stdcall ExchangeViaTCP##N(void) { ExchangeViaTCP_Component(N); }

DEFINE_EXCHANGE_WRAPPER(0)
DEFINE_EXCHANGE_WRAPPER(1)
DEFINE_EXCHANGE_WRAPPER(2)

void __stdcall GetOutputIndex(void) {
    int idx = static_cast<int>(pop());
    if (idx >= 0 && idx < current_output_count) {
        push(output_buffer[idx]);
    }
    else {
        emitMessage("Invalid index in GetOutputIndex()");
        push(0.0);
    }
}

int __stdcall RegisterFunctions(int ifun, char* cnam, int* iargc) {
    emitMessage(">>> RegisterFunctions() called");

    if (!cnam || !iargc) return 1;
    LoadDigsiLibrary();

    if (ifun < NUM_COMPONENTS) {
        sprintf(cnam, "ExchangeViaTCP%d", ifun);
        *iargc = COMPONENTS[ifun].numInputs;
        return 0;
    }
    else if (ifun == NUM_COMPONENTS) {
        strcpy(cnam, "GetOutputIndex");
        *iargc = 1;
        return 0;
    }

    return 1;
}
