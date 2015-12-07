#include "websocket.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#pragma comment(lib,"websockets.lib")
#endif

bool running = true;

#ifdef __linux__
#include <unistd.h>
#include <signal.h>
void signal_handler(int sig) {
    running = false;
}
#endif

class test_delegate : public WebSocket::Delegate {
public:
    virtual ~test_delegate(){}

    virtual void onOpen(WebSocket* ws) {
        printf("Websocket connected\n");
    }

    virtual void onMessage(WebSocket* ws,const WebSocket::Data& data) {
		printf("We have accept data!!!\n");
        if(!data.isBinary)
            printf("RECV:%s\n",data.bytes);
    }

    virtual void onClose(WebSocket* ws) {
        printf("WebSocket Closed\n");
    }

    virtual void onError(WebSocket* ws,const WebSocket::ErrorCode& error) {
        printf("WebSocket Occur Error\n");
    }
};

int main(int argc,char** argv) {
    test_delegate _del;
    WebSocket ws;

#ifdef __linux__
    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        printf("could not register signal handler\n");
        exit(1);
    }
#endif

	if (ws.init(_del,"ws://changer.site:3000/")) {
		while (running) {
			ws.processMessage();
#ifdef _WIN32
			Sleep(50);
#else
            usleep(50 * 1000);
#endif
		}

        ws.close();
    }

}
