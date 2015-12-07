#ifndef __LIBSOCKETIO_WEBSOCKET_
#define __LIBSOCKETIO_WEBSOCKET_
#include <string>
#include <vector>

struct libwebsocket;
struct libwebsocket_context;
struct libwebsocket_protocols;

class WsThreadHelper;
class WsMessage;

class WebSocket {
public:
    WebSocket();
    virtual ~WebSocket();

    struct Data
    {
        Data():bytes(NULL), len(0), issued(0), isBinary(false){}
        char* bytes;
        size_t len, issued;
        bool isBinary;
    };

    /**
     * ErrorCode enum used to represent the error in the websocket.
     */
    enum ErrorCode
    {
        TIME_OUT,           /** &lt; value 0 */
        CONNECTION_FAILURE, /** &lt; value 1 */
        UNKNOWN,            /** &lt; value 2 */
    };

    /**
     *  State enum used to represent the Websocket state.
     */
    enum State
    {
        CONNECTING,  /** &lt; value 0 */
        OPEN,        /** &lt; value 1 */
        CLOSING,     /** &lt; value 2 */
        CLOSED,      /** &lt; value 3 */
    };

public:

    class Delegate {
    public:
        virtual ~Delegate(){}

        virtual void onOpen(WebSocket* ws) = 0;

        virtual void onMessage(WebSocket* ws,const Data& data) = 0;

        virtual void onClose(WebSocket* ws) = 0;

        virtual void onError(WebSocket* ws,const ErrorCode& error) = 0;
    };

    bool init(const Delegate& delegate,
              const std::string& url,
              const std::vector<std::string>* protocols = NULL);

    void send(const std::string& message);

    void send(const unsigned char* binaryMsg, unsigned int len);

    void close();

    State getReadyState();

    bool processMessage();

private:
    virtual void onSubThreadStarted();
    virtual int onSubThreadLoop();
    virtual void onSubThreadEnded();
    virtual void onUIThreadReceiveMessage(WsMessage* msg);


    friend class WebSocketCallbackWrapper;
    int onSocketCallback(struct libwebsocket_context *ctx,
                         struct libwebsocket *wsi,
                         int reason,
                         void *user, void *in, size_t len);

private:
    State        _readyState;
    std::string  _host;
    unsigned int _port;
    std::string  _path;

    size_t _pendingFrameDataLen;
    size_t _currentDataLen;
    char *_currentData;

    friend class WsThreadHelper;
    WsThreadHelper* _wsHelper;

    struct libwebsocket*         _wsInstance;
    struct libwebsocket_context* _wsContext;
    Delegate* _delegate;
    int _SSLConnection;
    struct libwebsocket_protocols* _wsProtocols;
};

#endif
