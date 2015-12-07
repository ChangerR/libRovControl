#ifndef __LIBSOCKETIO_SOCKETIO_H
#define __LIBSOCKETIO_SOCKETIO_H
#include <string>
#include <stdio.h>
#include "Ref.h"
#include <map>
#include <list>
#include "config.h"

class SIOClientImpl;
class SIOClient;

class SocketIO {
public:
    class SIODelegate
    {
    public:
       /** Destructor of SIODelegate. */
       virtual ~SIODelegate() {}

       virtual void onConnect(SIOClient* client) { CC_UNUSED_PARAM(client); CCLOG("SIODelegate onConnect fired"); };

       virtual void onMessage(SIOClient* client, const std::string& data) { CC_UNUSED_PARAM(client); CCLOG("SIODelegate onMessage fired with data: %s", data.c_str()); };

       virtual void onClose(SIOClient* client) = 0;

       virtual void onError(SIOClient* client, const std::string& data) = 0;

       virtual void fireEventToScript(SIOClient* client, const std::string& eventName, const std::string& data) { CC_UNUSED_PARAM(client); CCLOG("SIODelegate event '%s' fired with data: %s", eventName.c_str(), data.c_str()); };
    };

    static SIOClient* connect(const std::string& uri, SocketIO::SIODelegate& delegate);

    static SocketIO* getInstance();
    static void destroyInstance();

    void dispatchMessage();

private:

    SocketIO();
    virtual ~SocketIO(void);

    static SocketIO *_inst;

    std::map<std::string, SIOClientImpl*> _sockets;
	std::list<SIOClientImpl*> _allSockets;
    SIOClientImpl* getSocket(const std::string& uri);
    void addSocket(const std::string& uri, SIOClientImpl* socket);
    void removeSocket(const std::string& uri);

    friend class SIOClientImpl;
};

typedef void (*SIOEventCall)(SIOClient*,const std::string&,void*);

class SIOEvent {
public:
    SIOEvent():_call(NULL),_userData(NULL){}
    SIOEventCall _call;
    void* _userData;
};

class SIOClient:
    public Ref
{
private:
    int _port;
    std::string _host, _path, _tag;
    bool _connected;
    SIOClientImpl* _socket;

    SocketIO::SIODelegate* _delegate;

    std::map<std::string,SIOEvent> _eventRegistry;

    void fireEvent(const std::string& eventName, const std::string& data);

    void onOpen();
    void onConnect();
    void socketClosed();

    friend class SIOClientImpl;

public:

    SIOClient(const std::string& host, int port, const std::string& path, SIOClientImpl* impl, SocketIO::SIODelegate& delegate);

    virtual ~SIOClient(void);

    SocketIO::SIODelegate* getDelegate() { return _delegate; };

    void disconnect();

    void send(const std::string& s);

    void emit(const std::string& eventname, const std::string& args);

    void on(const std::string& eventName, SIOEvent e);

    void setTag(const char* tag);

    inline const char* getTag()
    {
        return _tag.c_str();
    };

};

#endif
