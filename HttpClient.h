#ifndef __LIBSOCKETIO_HTTPCLIENT__
#define __LIBSOCKETIO_HTTPCLIENT__
#include "config.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpCookie.h"

#define HTTPCLIENT_RESPONSE_BUFFER_SIZE 256

class HttpClient {
public:
	HttpClient();

	virtual ~HttpClient();

	void enableCookies(const char* cookieFile);

	const std::string& getCookieFilename();

	void send(HttpRequest* request);

	void sendImmediate(HttpRequest* request);

	void setTimeoutForConnect(int value);

	int getTimeoutForConnect();

	void setTimeoutForRead(int value);

	int getTimeoutForRead();

	void processRequest(HttpResponse* response, char* responseMessage);

	void setSSLVerification(const std::string& caFile);

	const std::string& getSSLVerification();

	HttpCookie* getCookie() const {
		return _cookie;
	}

protected:
	int _timeoutForConnect;
	int _timeoutForRead;

	std::string _cookieFilename;
	HttpCookie* _cookie;

	std::string _sslCaFilename;
	char _responseMessage[HTTPCLIENT_RESPONSE_BUFFER_SIZE];
};
#endif
