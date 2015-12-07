#include "SocketIO.h"
#include <algorithm>
#include <sstream>
#include <iterator>
#include <map>
#include "websocket.h"
#include "HttpClient.h"

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#ifdef __linux__
#include <sys/time.h>
#endif
class SocketIOPacketV10x;

class SocketIOPacket
{
public:
    enum SocketIOVersion
    {
        V09x,
        V10x
    };

    SocketIOPacket();
    virtual ~SocketIOPacket();
    void initWithType(const std::string& packetType);
    void initWithTypeIndex(int index);

    std::string toString()const;
    virtual int typeAsNumber()const;
    const std::string& typeForIndex(int index)const;

    void setEndpoint(const std::string& endpoint){ _endpoint = endpoint; };
    const std::string& getEndpoint()const{ return _endpoint; };
    void setEvent(const std::string& event){ _name = event; };
    const std::string& getEvent()const{ return _name; };

    void addData(const std::string& data);
    std::vector<std::string> getData()const{ return _args; };
    virtual std::string stringify()const;

    static SocketIOPacket * createPacketWithType(std::string type, SocketIOVersion version);
    static SocketIOPacket * createPacketWithTypeIndex(int type, SocketIOVersion version);
protected:
    std::string _pId;//id message
    std::string _ack;//
    std::string _name;//event name
    std::vector<std::string> _args;//we will be using a vector of strings to store multiple data
    std::string _endpoint;//
    std::string _endpointseparator;//socket.io 1.x requires a ',' between endpoint and payload
    std::string _type;//message type
    std::string _separator;//for stringify the object
    std::vector<std::string> _types;//types of messages
};

class SocketIOPacketV10x : public SocketIOPacket
{
public:
    SocketIOPacketV10x();
    virtual ~SocketIOPacketV10x();
    int typeAsNumber()const;
    std::string stringify()const;
private:
    std::vector<std::string> _typesMessage;
};

SocketIOPacket::SocketIOPacket() :_separator(":"), _endpointseparator("")
{
    _types.push_back("disconnect");
    _types.push_back("connect");
    _types.push_back("heartbeat");
    _types.push_back("message");
    _types.push_back("json");
    _types.push_back("event");
    _types.push_back("ack");
    _types.push_back("error");
    _types.push_back("noop");
}

SocketIOPacket::~SocketIOPacket()
{
    _types.clear();
}

void SocketIOPacket::initWithType(const std::string& packetType)
{
    _type = packetType;
}
void SocketIOPacket::initWithTypeIndex(int index)
{
    _type = _types.at(index);
}

std::string SocketIOPacket::toString()const
{
    std::stringstream encoded;
    encoded << this->typeAsNumber();
    encoded << this->_separator;

    std::string pIdL = _pId;
    if (_ack == "data")
    {
        pIdL += "+";
    }

    // Do not write pid for acknowledgements
    if (_type != "ack")
    {
        encoded << pIdL;
    }
    encoded << this->_separator;

    // Add the endpoint for the namespace to be used if not the default namespace "" or "/", and as long as it is not an ACK, heartbeat, or disconnect packet
    if (_endpoint != "/" && _endpoint != "" && _type != "ack" && _type != "heartbeat" && _type != "disconnect") {
        encoded << _endpoint << _endpointseparator;
    }
    encoded << this->_separator;


    if (_args.size() != 0)
    {
        std::string ackpId = "";
        // This is an acknowledgement packet, so, prepend the ack pid to the data
        if (_type == "ack")
        {
            ackpId += pIdL + "+";
        }

        encoded << ackpId << this->stringify();
    }

    return encoded.str();
}

int SocketIOPacket::typeAsNumber()const
{
    std::string::size_type num = 0;
    std::vector<std::string>::const_iterator item = std::find(_types.begin(), _types.end(), _type);
    if (item != _types.end())
    {
        num = item - _types.begin();
    }
    return (int)num;
}

const std::string& SocketIOPacket::typeForIndex(int index)const
{
    return _types.at(index);
}

void SocketIOPacket::addData(const std::string& data)
{
    this->_args.push_back(data);
}

std::string SocketIOPacket::stringify()const
{

    std::string outS;
    if (_type == "message")
    {
        outS = _args[0];
    }
    else
    {

        rapidjson::StringBuffer s;
        rapidjson::Writer<rapidjson::StringBuffer> writer(s);

        writer.StartObject();
        writer.String("name");
        writer.String(_name.c_str());

        writer.String("args");

        writer.StartArray();

        for(std::vector<std::string>::const_iterator item = _args.begin(); item != _args.end();++item)
        {
            writer.String(item->c_str());
        }

        writer.EndArray();
        writer.EndObject();

        outS = s.GetString();

        CCLOG("create args object: %s:", outS.c_str());
    }

    return outS;
}

SocketIOPacketV10x::SocketIOPacketV10x()
{
    _separator = "";
    _endpointseparator = ",";
    _types.push_back("disconnected");
    _types.push_back("connected");
    _types.push_back("heartbeat");
    _types.push_back("pong");
    _types.push_back("message");
    _types.push_back("upgrade");
    _types.push_back("noop");
    _typesMessage.push_back("connect");
    _typesMessage.push_back("disconnect");
    _typesMessage.push_back("event");
    _typesMessage.push_back("ack");
    _typesMessage.push_back("error");
    _typesMessage.push_back("binarevent");
    _typesMessage.push_back("binaryack");
}

int SocketIOPacketV10x::typeAsNumber()const
{
    std::vector<std::string>::size_type num = 0;
    std::vector<std::string>::const_iterator item = std::find(_typesMessage.begin(), _typesMessage.end(), _type);
    if (item != _typesMessage.end())
    {//it's a message
        num = item - _typesMessage.begin();
        num += 40;
    }
    else
    {
        item = std::find(_types.begin(), _types.end(), _type);
        num += item - _types.begin();
    }
    return (int)num;
}

std::string SocketIOPacketV10x::stringify()const
{

    std::string outS;

    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);

    writer.StartArray();
    writer.String(_name.c_str());

    for(std::vector<std::string>::const_iterator item = _args.begin(); item != _args.end();++item)
    {
        writer.String(item->c_str());
    }

    writer.EndArray();

    outS = s.GetString();

    CCLOG("create args object: %s:", outS.c_str());

    return outS;

}

SocketIOPacketV10x::~SocketIOPacketV10x()
{
    _types.clear();
    _typesMessage.clear();
    _type = "";
    _pId = "";
    _name = "";
    _ack = "";
    _endpoint = "";
}

SocketIOPacket * SocketIOPacket::createPacketWithType(std::string type, SocketIOPacket::SocketIOVersion version)
{
    SocketIOPacket *ret;
    switch (version)
    {
    case SocketIOPacket::V09x:
        ret = new SocketIOPacket;
        break;
    case SocketIOPacket::V10x:
        ret = new SocketIOPacketV10x;
        break;
    }
    ret->initWithType(type);
    return ret;
}


SocketIOPacket * SocketIOPacket::createPacketWithTypeIndex(int type, SocketIOPacket::SocketIOVersion version)
{
    SocketIOPacket *ret;
    switch (version)
    {
    case SocketIOPacket::V09x:
        ret = new SocketIOPacket;
        break;
    case SocketIOPacket::V10x:
        return new SocketIOPacketV10x;
        break;
    }
    ret->initWithTypeIndex(type);
    return ret;
}

class SIOClientImpl :
    public Ref,
    public WebSocket::Delegate
{
private:
    int _port, _heartbeat, _timeout;
    std::string _host, _sid, _uri;
    bool _connected;
    SocketIOPacket::SocketIOVersion _version;

    WebSocket *_ws;

    std::map<std::string, SIOClient*> _clients;

    cc_pthread_t _heartbeatThread;
    bool _heartbeatRunning;
#ifdef _WIN32
	long _lastHeartBeatTime;
#endif

#ifdef __linux__
    cc_mutex_t _heartbeatThreadMutex;
    pthread_cond_t _heartbeatThreadIntCond;
#endif

public:
    SIOClientImpl(const std::string& host, int port);
    virtual ~SIOClientImpl(void);

    static SIOClientImpl* create(const std::string& host, int port);

    virtual void onOpen(WebSocket* ws);
    virtual void onMessage(WebSocket* ws, const WebSocket::Data& data);
    virtual void onClose(WebSocket* ws);
    virtual void onError(WebSocket* ws, const WebSocket::ErrorCode& error);

    void connect();
    void disconnect();
    bool init();
    void handshake();
    static void handshakeResponse(HttpResponse *response,void* data);
    void openSocket();
    void heartbeat(long dt);
    void createHeartBeatThread();
    void closeHeartBeatThread();
#ifdef _WIN32
    static DWORD WINAPI hbThreadEntryFunc(LPVOID);
#elif defined(__linux__)
    static void* hbThreadEntryFunc(void*);
#endif
    SIOClient* getClient(const std::string& endpoint);
    void addClient(const std::string& endpoint, SIOClient* client);

    void connectToEndpoint(const std::string& endpoint);
    void disconnectFromEndpoint(const std::string& endpoint);

    void send(const std::string& endpoint, const std::string& s);
    void send(SocketIOPacket *packet);
    void emit(const std::string& endpoint, const std::string& eventname, const std::string& args);

    friend class SocketIO;
};


//method implementations

//begin SIOClientImpl methods
SIOClientImpl::SIOClientImpl(const std::string& host, int port) :
    _port(port),
    _host(host),
    _connected(false)
{
    std::stringstream s;
    s << host << ":" << port;
    _uri = s.str();

    _ws = NULL;
}

SIOClientImpl::~SIOClientImpl()
{
    if (_connected)
        disconnect();

    CC_SAFE_DELETE(_ws);
}

void SIOClientImpl::handshake()
{
    CCLOG("SIOClientImpl::handshake() called");
    HttpClient client;
    std::stringstream pre;
    pre << "http://" << _uri << "/socket.io/1/?EIO=2&transport=polling&b64=true";

    HttpRequest* request = new (std::nothrow) HttpRequest();
    request->setUrl(pre.str().c_str());
    request->setRequestType(HttpRequest::GET);
    request->setUserData(this);
    request->setTag("handshake");
    request->setCallback(SIOClientImpl::handshakeResponse);

    CCLOG("SIOClientImpl::handshake() waiting");
    client.setTimeoutForConnect(5000);
    client.sendImmediate(request);

    delete request;

    return;
}

void SIOClientImpl::handshakeResponse(HttpResponse *response,void* data)
{
    CCLOG("SIOClientImpl::handshakeResponse() called");
    SIOClientImpl* pointer = (SIOClientImpl*)data;

    if (0 != strlen(response->getHttpRequest()->getTag()))
    {
        CCLOG("%s completed", response->getHttpRequest()->getTag());
    }

    long statusCode = response->getResponseCode();
    char statusString[64] = {};
    sprintf(statusString, "HTTP Status Code: %ld, tag = %s", statusCode, response->getHttpRequest()->getTag());
    CCLOG("response code: %ld", statusCode);

    if (!response->isSucceed())
    {
        CCLOG("SIOClientImpl::handshake() failed");
        CCLOG("error buffer: %s", response->getErrorBuffer());

        for (std::map<std::string, SIOClient*>::iterator iter =pointer-> _clients.begin(); iter != pointer->_clients.end(); ++iter)
        {
            iter->second->getDelegate()->onError(iter->second, response->getErrorBuffer());
        }

        return;
    }

    CCLOG("SIOClientImpl::handshake() succeeded");

    std::vector<char> *buffer = response->getResponseData();
    std::stringstream s;
    s.str("");

    for (unsigned int i = 0; i < buffer->size(); i++)
    {
        s << (*buffer)[i];
    }

    CCLOG("SIOClientImpl::handshake() dump data: %s", s.str().c_str());

    std::string res = s.str();
    std::string sid = "";
    int heartbeat = 0, timeout = 0;

    if (res.at(res.size() - 1) == '}') {

        CCLOG("SIOClientImpl::handshake() Socket.IO 1.x detected");
        pointer->_version = SocketIOPacket::V10x;
        // sample: 97:0{"sid":"GMkL6lzCmgMvMs9bAAAA","upgrades":["websocket"],"pingInterval":25000,"pingTimeout":60000}

        rapidjson::Document d;
        d.Parse(res.c_str() + 4);

        sid = d["sid"].GetString();

        heartbeat = d["pingInterval"].GetInt() ;

        timeout = d["pingTimeout"].GetInt();
        /*
        std::string::size_type a, b;
        a = res.find('{');
        std::string temp = res.substr(a, res.size() - a);

        // find the sid
        a = temp.find(":");
        b = temp.find(",");

        sid = temp.substr(a + 2, b - (a + 3));
        // chomp past the upgrades
        a = temp.find(":");
        b = temp.find(",");

        temp = temp.erase(0, b + 1);

        // get the pingInterval / heartbeat
        a = temp.find(":");
        b = temp.find(",");

        std::string heartbeat_str = temp.substr(a + 1, b - a);
        heartbeat = atoi(heartbeat_str.c_str()) / 1000;
        temp = temp.erase(0, b + 1);

        // get the timeout
        a = temp.find(":");
        b = temp.find("}");

        std::string timeout_str = temp.substr(a + 1, b - a);
        timeout = atoi(timeout_str.c_str()) / 1000;
        CCLOG("done parsing 1.x");
        */
        //CCLOG("parse 1.x sid=%s heartbeat=%d timeout=%d",sid.c_str(),heartbeat,timeout);
        //if(heartbeat == 0)heartbeat = 6000;
    }
    else {

        CCLOG("SIOClientImpl::handshake() Socket.IO 0.9.x detected");
        pointer->_version = SocketIOPacket::V09x;
        // sample: 3GYzE9md2Ig-lm3cf8Rv:60:60:websocket,htmlfile,xhr-polling,jsonp-polling
        size_t pos = 0;

        pos = res.find(":");
        if (pos != std::string::npos)
        {
            sid = res.substr(0, pos);
            res.erase(0, pos + 1);
        }

        pos = res.find(":");
        if (pos != std::string::npos)
        {
            heartbeat = atoi(res.substr(pos + 1, res.size()).c_str()) * 1000;
        }

        pos = res.find(":");
        if (pos != std::string::npos)
        {
            timeout = atoi(res.substr(pos + 1, res.size()).c_str()) * 1000;
        }

    }

    pointer->_sid = sid;
    pointer->_heartbeat = heartbeat;
    pointer->_timeout = timeout;

    pointer->openSocket();

    return;

}

void SIOClientImpl::openSocket()
{
    CCLOG("SIOClientImpl::openSocket() called");

    std::stringstream s;

    switch (_version)
    {
        case SocketIOPacket::V09x:
            s << _uri << "/socket.io/1/websocket/" << _sid;
            break;
        case SocketIOPacket::V10x:
            s << _uri << "/socket.io/1/websocket/?EIO=2&transport=websocket&sid=" << _sid;
            break;
    }

    _ws = new (std::nothrow) WebSocket();
    if (!_ws->init(*this, s.str()))
    {
        CC_SAFE_DELETE(_ws);
    }

    return;
}

bool SIOClientImpl::init()
{
    CCLOG("SIOClientImpl::init() successful");
    return true;
}

void SIOClientImpl::connect()
{
    this->handshake();
}

void SIOClientImpl::disconnect()
{
    if(_ws->getReadyState() == WebSocket::OPEN)
    {
        std::string s, endpoint;
        s = "";
        endpoint = "";

        if (_version == SocketIOPacket::V09x)
            s = "0::" + endpoint;
        else
            s = "41" + endpoint;
        _ws->send(s);
    }

    _ws->close();

    _connected = false;

    SocketIO::getInstance()->removeSocket(_uri);
}

SIOClientImpl* SIOClientImpl::create(const std::string& host, int port)
{
    SIOClientImpl *s = new (std::nothrow) SIOClientImpl(host, port);

    if (s && s->init())
    {
        return s;
    }

	return NULL;
}

SIOClient* SIOClientImpl::getClient(const std::string& endpoint)
{
	if (_clients.find(endpoint) != _clients.end())
		return _clients.at(endpoint);
	else
		return NULL;
}

void SIOClientImpl::addClient(const std::string& endpoint, SIOClient* client)
{
    client->retain();
    _clients.insert(std::pair<std::string,SIOClient*>(endpoint, client));
}

void SIOClientImpl::connectToEndpoint(const std::string& endpoint)
{
    SocketIOPacket *packet = SocketIOPacket::createPacketWithType("connect", _version);
    packet->setEndpoint(endpoint);
    this->send(packet);
}


void SIOClientImpl::disconnectFromEndpoint(const std::string& endpoint)
{
    _clients.erase(endpoint);
    if (_clients.empty() || endpoint == "/")
    {
        CCLOG("SIOClientImpl::disconnectFromEndpoint out of endpoints, checking for disconnect");

        if (_connected)
            this->disconnect();
    }
    else
    {
        std::string path = endpoint == "/" ? "" : endpoint;

        std::string s = "0::" + path;

        _ws->send(s);
    }
}

#ifdef _WIN32
DWORD WINAPI SIOClientImpl::hbThreadEntryFunc(LPVOID data) {
	SIOClientImpl* pointer = (SIOClientImpl*)data;
	pointer->_lastHeartBeatTime = GetTickCount();
	long nowt;
	while (pointer->_heartbeatRunning) {
		nowt = GetTickCount();
		if (pointer->_lastHeartBeatTime + pointer->_heartbeat < nowt)
		{
			pointer->heartbeat(nowt - pointer->_lastHeartBeatTime);
			pointer->_lastHeartBeatTime = nowt;
		}
		Sleep(1);
	}
	return 0;
}
#elif defined(__linux__)
void* SIOClientImpl::hbThreadEntryFunc(void* data) {
    SIOClientImpl* pointer = (SIOClientImpl*)data;
    struct timeval now;
	struct timespec outtime;

    CC_MUTEX_LOCK(pointer->_heartbeatThreadMutex);

    while(pointer->_heartbeatRunning) {
        gettimeofday(&now, NULL);
        outtime.tv_sec = now.tv_sec + long(pointer->_heartbeat * 0.0009f);
        outtime.tv_nsec = (now.tv_usec ) * 1000;
        pthread_cond_timedwait(&pointer->_heartbeatThreadIntCond, &pointer->_heartbeatThreadMutex, &outtime);
        pointer->heartbeat(0);
        CCLOG("running heartbeat thread");
    }

    CC_MUTEX_UNLOCK(pointer->_heartbeatThreadMutex);
    return NULL;
}
#endif

void SIOClientImpl::createHeartBeatThread() {
    _heartbeatRunning = true;
#ifdef _WIN32
    _heartbeatThread = CreateThread(NULL,0,SIOClientImpl::hbThreadEntryFunc,(void*)this,0,NULL);
#elif defined(__linux__)
    CC_INIT_MUTEX(_heartbeatThreadMutex);
    pthread_cond_init(&_heartbeatThreadIntCond,NULL);

    pthread_create(&_heartbeatThread,NULL,SIOClientImpl::hbThreadEntryFunc,(void*)this);
#endif
}

void SIOClientImpl::closeHeartBeatThread() {
    _heartbeatRunning = false;
#ifdef _WIN32
    WaitForSingleObject(_heartbeatThread, INFINITE);
    CloseHandle(_heartbeatThread);
#elif defined(__linux__)
    CC_MUTEX_LOCK(_heartbeatThreadMutex);
    pthread_cond_signal(&_heartbeatThreadIntCond);
    CC_MUTEX_UNLOCK(_heartbeatThreadMutex);

    pthread_join(_heartbeatThread,NULL);
    CC_DESTROY_MUTEX(_heartbeatThreadMutex);
#endif
}

void SIOClientImpl::heartbeat(long dt)
{
    CC_UNUSED_PARAM(dt);
    SocketIOPacket *packet = SocketIOPacket::createPacketWithType("heartbeat", _version);

    this->send(packet);

    CCLOG("Heartbeat sent");
}


void SIOClientImpl::send(const std::string& endpoint, const std::string& s)
{
    switch (_version) {
    case SocketIOPacket::V09x:
		{
			SocketIOPacket *packet = SocketIOPacket::createPacketWithType("message", _version);
			packet->setEndpoint(endpoint);
			packet->addData(s);
			this->send(packet);
			break;
		}
    case SocketIOPacket::V10x:
		{
			this->emit(endpoint, "message", s);
			break;
		}
	}
}

void SIOClientImpl::send(SocketIOPacket *packet)
{
    std::string req = packet->toString();
    if (_connected)
    {
        CCLOG("-->SEND:%s", req.data());
        _ws->send(req.data());
    }
    else
        CCLOG("Cant send the message (%s) because disconnected", req.c_str());
}

void SIOClientImpl::emit(const std::string& endpoint, const std::string& eventname, const std::string& args)
{
    CCLOG("Emitting event \"%s\"", eventname.c_str());
    SocketIOPacket *packet = SocketIOPacket::createPacketWithType("event", _version);
    packet->setEndpoint(endpoint == "/" ? "" : endpoint);
    packet->setEvent(eventname);
    packet->addData(args);
    this->send(packet);
}

void SIOClientImpl::onOpen(WebSocket* ws)
{
    CC_UNUSED_PARAM(ws);
    _connected = true;

    SocketIO::getInstance()->addSocket(_uri, this);

    if (_version == SocketIOPacket::V10x)
    {
        std::string s = "5";//That's a ping https://github.com/Automattic/engine.io-parser/blob/1b8e077b2218f4947a69f5ad18be2a512ed54e93/lib/index.js#L21
        _ws->send(s.data());
    }

    //Director::getInstance()->getScheduler()->schedule(CC_SCHEDULE_SELECTOR(SIOClientImpl::heartbeat), this, (_heartbeat * .9f), false);

    for ( std::map<std::string, SIOClient*>::iterator iter = _clients.begin(); iter != _clients.end(); ++iter)
    {
        iter->second->onOpen();
    }

    createHeartBeatThread();

    CCLOG("SIOClientImpl::onOpen socket connected!");
}

void SIOClientImpl::onMessage(WebSocket* ws, const WebSocket::Data& data)
{
    CCLOG("SIOClientImpl::onMessage received: %s", data.bytes);
    CC_UNUSED_PARAM(ws);

    std::string payload = data.bytes;
    int control = atoi(payload.substr(0, 1).c_str());
    payload = payload.substr(1, payload.size() - 1);

    SIOClient *c = NULL;

    switch (_version)
    {
        case SocketIOPacket::V09x:
        {
            std::string msgid, endpoint, s_data, eventname;

            std::string::size_type pos, pos2;

            pos = payload.find(":");
            if (pos != std::string::npos)
            {
                payload.erase(0, pos + 1);
            }

            pos = payload.find(":");
            if (pos != std::string::npos)
            {
                msgid = atoi(payload.substr(0, pos + 1).c_str());
            }
            payload.erase(0, pos + 1);

            pos = payload.find(":");
            if (pos != std::string::npos)
            {
                endpoint = payload.substr(0, pos);
                payload.erase(0, pos + 1);
            }
            else
            {
                endpoint = payload;
            }

            if (endpoint == "") endpoint = "/";

            c = getClient(endpoint);

            s_data = payload;

            if (c == NULL) CCLOG("SIOClientImpl::onMessage client lookup returned NULL");

            switch (control)
            {
            case 0:
                CCLOG("Received Disconnect Signal for Endpoint: %s", endpoint.c_str());
                disconnectFromEndpoint(endpoint);
                c->fireEvent("disconnect", payload);
                break;
            case 1:
                CCLOG("Connected to endpoint: %s", endpoint.c_str());
                if (c) {
                    c->onConnect();
                    c->fireEvent("connect", payload);
                }
                break;
            case 2:
                CCLOG("Heartbeat received");
                break;
            case 3:
                CCLOG("Message received: %s", s_data.c_str());
                if (c) c->getDelegate()->onMessage(c, s_data);
                if (c) c->fireEvent("message", s_data);
                break;
            case 4:
                CCLOG("JSON Message Received: %s", s_data.c_str());
                if (c) c->getDelegate()->onMessage(c, s_data);
                if (c) c->fireEvent("json", s_data);
                break;
            case 5:
                CCLOG("Event Received with data: %s", s_data.c_str());

                if (c)
                {
                    eventname = "";
                    pos = s_data.find(":");
                    pos2 = s_data.find(",");
                    if (pos2 > pos)
                    {
                        eventname = s_data.substr(pos + 2, pos2 - (pos + 3));
                        s_data = s_data.substr(pos2 + 9, s_data.size() - (pos2 + 11));
                    }

                    c->fireEvent(eventname, s_data);
                }

                break;
            case 6:
                CCLOG("Message Ack");
                break;
            case 7:
                CCLOG("Error");
                //if (c) c->getDelegate()->onError(c, s_data);
                if (c) c->fireEvent("error", s_data);
                break;
            case 8:
                CCLOG("Noop");
                break;
            }
        }
        break;
        case SocketIOPacket::V10x:
        {
            switch (control)
            {
            case 0:
                CCLOG("Not supposed to receive control 0 for websocket");
                CCLOG("That's not good");
                break;
            case 1:
                CCLOG("Not supposed to receive control 1 for websocket");
                break;
            case 2:
                CCLOG("Ping received, send pong");
                payload = "3" + payload;
                _ws->send(payload.c_str());
                break;
            case 3:
                CCLOG("Pong received");
                if (payload == "probe")
                {
                    CCLOG("Request Update");
                    _ws->send("5");
                }
                break;
            case 4:
            {
                const char second = payload.at(0);
                int control2 = atoi(&second);
                CCLOG("Message code: [%i]", control);

                SocketIOPacket *packetOut = SocketIOPacket::createPacketWithType("event", _version);
                std::string endpoint = "";

                std::string::size_type a = payload.find("/");
                std::string::size_type b = payload.find("[");

                if (b != std::string::npos)
                {
                    if (a != std::string::npos && a < b)
                    {
                        //we have an endpoint and a payload
                        endpoint = payload.substr(a, b - (a + 1));
                    }
                }
                else if (a != std::string::npos) {
                    //we have an endpoint with no payload
                    endpoint = payload.substr(a, payload.size() - a);
                }

                // we didn't find and endpoint and we are in the default namespace
                if (endpoint == "") endpoint = "/";

                packetOut->setEndpoint(endpoint);

                c = getClient(endpoint);

                payload = payload.substr(1);

                if (endpoint != "/") payload = payload.substr(endpoint.size());
                if (endpoint != "/" && payload != "") payload = payload.substr(1);

                switch (control2)
                {
                case 0:
                    CCLOG("Socket Connected");
                    if (c) {
                        c->onConnect();
                        c->fireEvent("connect", payload);
                    }
                    break;
                case 1:
                    CCLOG("Socket Disconnected");
                    disconnectFromEndpoint(endpoint);
                    c->fireEvent("disconnect", payload);
                    break;
                case 2:
                {
                    CCLOG("Event Received (%s)", payload.c_str());

                    std::string::size_type payloadFirstSlashPos = payload.find("\"");
                    std::string::size_type payloadSecondSlashPos = payload.substr(payloadFirstSlashPos + 1).find("\"");

                    std::string eventname = payload.substr(payloadFirstSlashPos + 1,
                                                           payloadSecondSlashPos - payloadFirstSlashPos + 1);

                    CCLOG("event name %s between %ld and %ld", eventname.c_str(),
                              payloadFirstSlashPos, payloadSecondSlashPos);

                    payload = payload.substr(payloadSecondSlashPos + 4,
                                             payload.size() - (payloadSecondSlashPos + 5));

                    if (c) c->fireEvent(eventname, payload);
                    if (c) c->getDelegate()->onMessage(c, payload);

                }
                break;
                case 3:
                    CCLOG("Message Ack");
                    break;
                case 4:
                    CCLOG("Error");
                    if (c) c->fireEvent("error", payload);
                    break;
                case 5:
                    CCLOG("Binary Event");
                    break;
                case 6:
                    CCLOG("Binary Ack");
                    break;
                }
            }
            break;
            case 5:
                CCLOG("Upgrade required");
                break;
            case 6:
                CCLOG("Noop\n");
                break;
            }
        }
        break;
    }

    return;
}

void SIOClientImpl::onClose(WebSocket* ws)
{
    CC_UNUSED_PARAM(ws);

    for(std::list<SIOClientImpl*>::iterator iter = SocketIO::getInstance()->_allSockets.begin(); iter != SocketIO::getInstance()->_allSockets.end(); ++iter) {
        if(*iter == this) {
            SocketIO::getInstance()->_allSockets.erase(iter);
            break;
        }
    }

    if (!_clients.empty())
    {
        for (std::map<std::string, SIOClient*>::iterator iter = _clients.begin(); iter != _clients.end(); ++iter)
        {
            iter->second->socketClosed();
        }
    }

    closeHeartBeatThread();

    this->release();
}

void SIOClientImpl::onError(WebSocket* ws, const WebSocket::ErrorCode& error)
{
    CC_UNUSED_PARAM(ws);
    CCLOG("Websocket error received: %d", error);
}

//begin SIOClient methods
SIOClient::SIOClient(const std::string& host, int port, const std::string& path, SIOClientImpl* impl, SocketIO::SIODelegate& delegate)
    : _port(port)
    , _host(host)
    , _path(path)
    , _connected(false)
    , _socket(impl)
    , _delegate(&delegate)
{

}

SIOClient::~SIOClient(void)
{
    if (_connected)
    {
        _socket->disconnectFromEndpoint(_path);
    }
}

void SIOClient::onOpen()
{
    if (_path != "/")
    {
        _socket->connectToEndpoint(_path);
    }

}

void SIOClient::onConnect()
{
    _connected = true;
    _delegate->onConnect(this);
}

void SIOClient::send(const std::string& s)
{
    if (_connected)
    {
        _socket->send(_path, s);
    }
    else
    {
        _delegate->onError(this, "Client not yet connected");
    }

}

void SIOClient::emit(const std::string& eventname, const std::string& args)
{
    if(_connected)
    {
        _socket->emit(_path, eventname, args);
    }
    else
    {
        _delegate->onError(this, "Client not yet connected");
    }

}

void SIOClient::disconnect()
{
    _connected = false;

    _socket->disconnectFromEndpoint(_path);

    this->release();
}

void SIOClient::socketClosed()
{
    _connected = false;

    _delegate->onClose(this);

    this->release();
}

void SIOClient::on(const std::string& eventName, SIOEvent e)
{
    _eventRegistry[eventName] = e;
}

void SIOClient::fireEvent(const std::string& eventName, const std::string& data)
{
    CCLOG("SIOClient::fireEvent called with event name: %s and data: %s", eventName.c_str(), data.c_str());

    _delegate->fireEventToScript(this, eventName, data);

    SIOEvent e = _eventRegistry[eventName];

    if(e._call != NULL)
        e._call(this, data,e._userData);
    else
        CCLOG("SIOClient::fireEvent no native event with name %s found", eventName.c_str());

    return;

}

void SIOClient::setTag(const char* tag)
{
    _tag = tag;
}

//begin SocketIO methods
SocketIO *SocketIO::_inst = NULL;

SocketIO::SocketIO()
{
}

SocketIO::~SocketIO(void)
{
    for(std::map<std::string, SIOClientImpl*>::iterator iter = _sockets.begin();iter != _sockets.end();++iter) {
        iter->second->release();
    }
}

SocketIO* SocketIO::getInstance()
{
    if (NULL == _inst)
        _inst = new (std::nothrow) SocketIO();

    return _inst;
}

void SocketIO::destroyInstance()
{
    CC_SAFE_DELETE(_inst);
}

SIOClient* SocketIO::connect(const std::string& uri, SocketIO::SIODelegate& delegate)
{
    std::string host = uri;
    int port = 0;
    size_t pos = 0;

    pos = host.find("//");
    if (pos != std::string::npos)
    {
        host.erase(0, pos+2);
    }

    pos = host.find(":");
    if (pos != std::string::npos)
    {
        port = atoi(host.substr(pos+1, host.size()).c_str());
    }

    pos = host.find("/", 0);
    std::string path = "/";
    if (pos != std::string::npos)
    {
        path += host.substr(pos + 1, host.size());
    }

    pos = host.find(":");
    if (pos != std::string::npos)
    {
        host.erase(pos, host.size());
    }else{
        host.erase(pos, host.size());
    }

    std::stringstream s;
    s << host << ":" << port;

    SIOClientImpl* socket = NULL;
    SIOClient *c = NULL;

    socket = SocketIO::getInstance()->getSocket(s.str());

    if(socket == NULL)
    {
        //create a new socket, new client, connect
        socket = SIOClientImpl::create(host, port);

        c = new (std::nothrow) SIOClient(host, port, path, socket, delegate);

        socket->addClient(path, c);

        socket->connect();

        SocketIO::getInstance()->_allSockets.push_back(socket);
    }
    else
    {
        //check if already connected to endpoint, handle
        c = socket->getClient(path);

        if(c == NULL)
        {
            c = new (std::nothrow) SIOClient(host, port, path, socket, delegate);

            socket->addClient(path, c);

            socket->connectToEndpoint(path);
        }else{
    	    CCLOG("SocketIO: disconnect previous client");
    	    c->disconnect();

    	    CCLOG("SocketIO: recreate a new socket, new client, connect");
    	    SIOClientImpl* newSocket = NULL;
    	    SIOClient *newC = NULL;

    	    newSocket = SIOClientImpl::create(host, port);

        	newC = new (std::nothrow) SIOClient(host, port, path, newSocket, delegate);

    	    newSocket->addClient(path, newC);

    	    newSocket->connect();
            SocketIO::getInstance()->_allSockets.push_back(newSocket);

    	    return newC;
    	}
    }

    return c;
}

SIOClientImpl* SocketIO::getSocket(const std::string& uri)
{
	if (_sockets.find(uri) != _sockets.end())
		return _sockets.at(uri);
	else
		return NULL;
}

void SocketIO::addSocket(const std::string& uri, SIOClientImpl* socket)
{
    socket->retain();
    _sockets.insert(std::pair<std::string,SIOClientImpl*>(uri, socket));
}

void SocketIO::removeSocket(const std::string& uri)
{
	if (_sockets.find(uri) != _sockets.end()) {
		_sockets.at(uri)->release();
		_sockets.erase(uri);
	}
}

void SocketIO::dispatchMessage() {

    for(std::list<SIOClientImpl*>::iterator iter = _allSockets.begin();iter != _allSockets.end();++iter) {
        if((*iter)->_ws != NULL)
            (*iter)->_ws->processMessage();
    }
}
