#ifndef __LIBSOCKETIO_HTTPREQUSET_
#define __LIBSOCKETIO_HTTPREQUSET_
#include <string>
#include <vector>
class HttpResponse;

typedef void (*HttpResponse_Callback)(HttpResponse*,void*);

class HttpRequest {
public:
    enum Type {
        GET = 0,
        POST,
        PUT,
        _DELETE,
        UNKNOWN,
    };

    HttpRequest() {
        _requestType = UNKNOWN;
        _url.clear();
        _requestData.clear();
        _headers.clear();
        _callback = NULL;
        _pUserData = NULL;
    }

    virtual ~HttpRequest() {

    }

    inline void setRequestType(Type type) {
        _requestType = type;
    }

    inline Type getRequestType() {
        return _requestType;
    }

    inline void setUrl(const char* url) {
        _url = url;
    }

    inline const char* getUrl() {
        return _url.c_str();
    }

    inline void setRequestData(const char* buffer,size_t len) {
        _requestData.assign(buffer,buffer + len);
    }

    inline char* getRequestData() {
        if(_requestData.size() != 0)
            return &(_requestData.front());
        return NULL;
    }

    inline size_t getRequestDataSize() {
        return _requestData.size();
    }

    inline void setUserData(void* pUserData) {
        _pUserData = pUserData;
    }

    inline void* getUserData() {
        return _pUserData;
    }

    inline void setHeaders(std::vector<std::string> pHeaders) {
        _headers = pHeaders;
    }

    inline std::vector<std::string> getHeaders() {
        return _headers;
    }

    inline HttpResponse_Callback getCallback() {
        return _callback;
    }

    inline void setCallback(HttpResponse_Callback _call) {
        _callback = _call;
    }

    inline void setTag(const char* tag) {
        _tag = tag;
    }

    inline const char* getTag() {
        return _tag.c_str();
    }
protected:
    Type _requestType;
    std::vector<char> _requestData;
    std::string _url;
    std::vector<std::string> _headers;
    void* _pUserData;
    HttpResponse_Callback _callback;
    std::string _tag;
};
#endif
