#include "SocketIO.h"
bool running = true;

#ifdef _WIN32
#pragma comment(lib,"libcurl.lib")
#pragma comment(lib,"websockets.lib")
#endif

#ifdef __linux__
#include <unistd.h>
#include <signal.h>
void signal_handler(int sig) {
    running = false;
}
#endif

class TestSIO_Delegate :
    public SocketIO::SIODelegate
{
public:
    virtual void onConnect(SIOClient* client) {
        client->emit("Hello","{1:\"tests\"}");
    }

    virtual void onClose(SIOClient* client) {
        printf("socket io close\n");
    }

    virtual void onError(SIOClient* client, const std::string& data) {
        printf("socket io occur error\n");
        running = false;
    }
};

void news_call(SIOClient* client,const std::string& data,void* user) {
    printf("[INFO] we accept:%s\n",data.c_str());
}

int main() {
    TestSIO_Delegate _del;

#ifdef __linux__
    if(signal(SIGINT, signal_handler) == SIG_ERR) {
        printf("could not register signal handler\n");
        return 1;
    }
#endif

    SIOClient* client = SocketIO::connect("localhost:80",_del);

    if(client != NULL) {
        SIOEvent e;
        e._call = news_call;
        e._userData = NULL;

        client->on("news",e);

        while (running) {
            SocketIO::getInstance()->dispatchMessage();
            CC_SLEEP(1);
        }
        client->disconnect();
    }

    return 0;
}
