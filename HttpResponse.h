#ifndef __LIBSOCKETIO_HTTPRESPONSE__
#define __LIBSOCKETIO_HTTPRESPONSE__

class HttpResponse
{
public:

    HttpResponse(HttpRequest* request)
    {
        _pHttpRequest = request;
        _succeed = false;
        _responseData.clear();
        _errorBuffer.clear();
        _responseDataString = "";
    }

    virtual ~HttpResponse()
    {
    }

    inline HttpRequest* getHttpRequest()
    {
        return _pHttpRequest;
    }

    inline bool isSucceed()
    {
        return _succeed;
    };

    inline std::vector<char>* getResponseData()
    {
        return &_responseData;
    }

    inline std::vector<char>* getResponseHeader()
    {
        return &_responseHeader;
    }

    inline long getResponseCode()
    {
        return _responseCode;
    }

    inline const char* getErrorBuffer()
    {
        return _errorBuffer.c_str();
    }

    inline void setSucceed(bool value)
    {
        _succeed = value;
    };

    inline void setResponseData(std::vector<char>* data)
    {
        _responseData = *data;
    }

    inline void setResponseHeader(std::vector<char>* data)
    {
        _responseHeader = *data;
    }

    inline void setResponseCode(long value)
    {
        _responseCode = value;
    }

    inline void setErrorBuffer(const char* value)
    {
        _errorBuffer.clear();
        _errorBuffer.assign(value);
    };

    inline void setResponseDataString(const char* value, size_t n)
    {
        _responseDataString.clear();
        _responseDataString.assign(value, n);
    }

    inline const char* getResponseDataString()
    {
        return _responseDataString.c_str();
    }

protected:
    // properties
    HttpRequest*        _pHttpRequest;
    bool                _succeed;
    std::vector<char>   _responseData;
    std::vector<char>   _responseHeader;
    long                _responseCode;
    std::string         _errorBuffer;
    std::string         _responseDataString;
};

#endif
