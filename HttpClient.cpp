#include "HttpClient.h"
#include "curl/curl.h"
#include <string.h>
typedef size_t (*write_callback)(void *ptr, size_t size, size_t nmemb, void *stream);

// Callback function used by libcurl for collect response data
static size_t writeData(void *ptr, size_t size, size_t nmemb, void *stream)
{
    std::vector<char> *recvBuffer = (std::vector<char>*)stream;
    size_t sizes = size * nmemb;

    // add data to the end of recvBuffer
    // write data maybe called more than once in a single request
    recvBuffer->insert(recvBuffer->end(), (char*)ptr, (char*)ptr+sizes);

    return sizes;
}

// Callback function used by libcurl for collect header data
static size_t writeHeaderData(void *ptr, size_t size, size_t nmemb, void *stream)
{
    std::vector<char> *recvBuffer = (std::vector<char>*)stream;
    size_t sizes = size * nmemb;

    // add data to the end of recvBuffer
    // write data maybe called more than once in a single request
    recvBuffer->insert(recvBuffer->end(), (char*)ptr, (char*)ptr+sizes);

    return sizes;
}

static bool configureCURL(HttpClient* client, CURL* handle, char* errorBuffer)
{
    if (!handle) {
        return false;
    }

    int code;
    code = curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errorBuffer);
    if (code != CURLE_OK) {
        return false;
    }
    code = curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, client->getTimeoutForRead());
    if (code != CURLE_OK) {
        return false;
    }
    code = curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS, client->getTimeoutForConnect());
    if (code != CURLE_OK) {
        return false;
    }

	std::string sslCaFilename = client->getSSLVerification();
	if (sslCaFilename.empty()) {
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
    } else {
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 2L);
		curl_easy_setopt(handle, CURLOPT_CAINFO, sslCaFilename.c_str());
    }

    // FIXED #3224: The subthread of CCHttpClient interrupts main thread if timeout comes.
    // Document is here: http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTNOSIGNAL
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L);

    curl_easy_setopt(handle, CURLOPT_ACCEPT_ENCODING, "");

    return true;
}

static int processGetTask(HttpClient* client, HttpRequest* request, write_callback callback, void *stream,
                            long *errorCode, write_callback headerCallback, void *headerStream, char* errorBuffer);
static int processPostTask(HttpClient* client, HttpRequest* request, write_callback callback, void *stream,
                            long *errorCode, write_callback headerCallback, void *headerStream, char* errorBuffer);
static int processPutTask(HttpClient* client,  HttpRequest* request, write_callback callback, void *stream,
                            long *errorCode, write_callback headerCallback, void *headerStream, char* errorBuffer);
static int processDeleteTask(HttpClient* client,  HttpRequest* request, write_callback callback, void *stream,
                            long *errorCode, write_callback headerCallback, void *headerStream, char* errorBuffer);

class CURLRaii
{
    /// Instance of CURL
    CURL *_curl;
    /// Keeps custom header data
    curl_slist *_headers;
public:
    CURLRaii()
        : _headers(NULL)
    {
        _curl = curl_easy_init();
    }

    ~CURLRaii()
    {
        if (_curl)
            curl_easy_cleanup(_curl);
        /* free the linked list for header data */
        if (_headers)
            curl_slist_free_all(_headers);
    }

    template <class T>
    bool setOption(CURLoption option, T data)
    {
        return CURLE_OK == curl_easy_setopt(_curl, option, data);
    }

    /**
     * @brief Inits CURL instance for common usage
     * @param request Null not allowed
     * @param callback Response write callback
     * @param stream Response write stream
     */
    bool init(HttpClient* client, HttpRequest* request, write_callback callback, void* stream, write_callback headerCallback, void* headerStream, char* errorBuffer)
    {
        if (!_curl)
            return false;
		if (!configureCURL(client, _curl, errorBuffer))
            return false;

        /* get custom header data (if set) */
        std::vector<std::string> headers=request->getHeaders();
        if(!headers.empty())
        {
            /* append custom headers one by one */
            for (std::vector<std::string>::iterator it = headers.begin(); it != headers.end(); ++it)
                _headers = curl_slist_append(_headers,it->c_str());
            /* set custom headers for curl */
            if (!setOption(CURLOPT_HTTPHEADER, _headers))
                return false;
        }
		std::string cookieFilename = client->getCookieFilename();
		if (!cookieFilename.empty()) {
			if (!setOption(CURLOPT_COOKIEFILE, cookieFilename.c_str())) {
                return false;
            }
			if (!setOption(CURLOPT_COOKIEJAR, cookieFilename.c_str())) {
                return false;
            }
        }

        return setOption(CURLOPT_URL, request->getUrl())
                && setOption(CURLOPT_WRITEFUNCTION, callback)
                && setOption(CURLOPT_WRITEDATA, stream)
                && setOption(CURLOPT_HEADERFUNCTION, headerCallback)
                && setOption(CURLOPT_HEADERDATA, headerStream);

    }

    /// @param responseCode Null not allowed
    bool perform(long *responseCode)
    {
        if (CURLE_OK != curl_easy_perform(_curl))
            return false;
        CURLcode code = curl_easy_getinfo(_curl, CURLINFO_RESPONSE_CODE, responseCode);
        if (code != CURLE_OK || !(*responseCode >= 200 && *responseCode < 300)) {
            printf("Curl curl_easy_getinfo failed: %s", curl_easy_strerror(code));
            return false;
        }
        // Get some mor data.

        return true;
    }
};

//Process Get Request
static int processGetTask(HttpClient* client, HttpRequest* request, write_callback callback, void* stream, long* responseCode, write_callback headerCallback, void* headerStream, char* errorBuffer)
{
    CURLRaii curl;
	bool ok = curl.init(client, request, callback, stream, headerCallback, headerStream, errorBuffer)
            && curl.setOption(CURLOPT_FOLLOWLOCATION, true)
            && curl.perform(responseCode);
    return ok ? 0 : 1;
}

//Process POST Request
static int processPostTask(HttpClient* client, HttpRequest* request, write_callback callback, void* stream, long* responseCode, write_callback headerCallback, void* headerStream, char* errorBuffer)
{
    CURLRaii curl;
	bool ok = curl.init(client, request, callback, stream, headerCallback, headerStream, errorBuffer)
            && curl.setOption(CURLOPT_POST, 1)
            && curl.setOption(CURLOPT_POSTFIELDS, request->getRequestData())
            && curl.setOption(CURLOPT_POSTFIELDSIZE, request->getRequestDataSize())
            && curl.perform(responseCode);
    return ok ? 0 : 1;
}

//Process PUT Request
static int processPutTask(HttpClient* client, HttpRequest* request, write_callback callback, void* stream, long* responseCode, write_callback headerCallback, void* headerStream, char* errorBuffer)
{
    CURLRaii curl;
	bool ok = curl.init(client, request, callback, stream, headerCallback, headerStream, errorBuffer)
            && curl.setOption(CURLOPT_CUSTOMREQUEST, "PUT")
            && curl.setOption(CURLOPT_POSTFIELDS, request->getRequestData())
            && curl.setOption(CURLOPT_POSTFIELDSIZE, request->getRequestDataSize())
            && curl.perform(responseCode);
    return ok ? 0 : 1;
}

//Process DELETE Request
static int processDeleteTask(HttpClient* client, HttpRequest* request, write_callback callback, void* stream, long* responseCode, write_callback headerCallback, void* headerStream, char* errorBuffer)
{
    CURLRaii curl;
	bool ok = curl.init(client, request, callback, stream, headerCallback, headerStream, errorBuffer)
            && curl.setOption(CURLOPT_CUSTOMREQUEST, "DELETE")
            && curl.setOption(CURLOPT_FOLLOWLOCATION, true)
            && curl.perform(responseCode);
    return ok ? 0 : 1;
}


// Process Response
void HttpClient::processRequest(HttpResponse* response, char* responseMessage)
{
	HttpRequest* request = response->getHttpRequest();
	long responseCode = -1;
	int retValue = 0;

    if(request == NULL) {
        printf("request is zero,please fix\n");
        return;
    }
	// Process the request -> get response packet
	switch (request->getRequestType())
	{
	case HttpRequest::GET: // HTTP GET
		retValue = processGetTask(this, request,
			writeData,
			response->getResponseData(),
			&responseCode,
			writeHeaderData,
			response->getResponseHeader(),
			responseMessage);
		break;

	case HttpRequest::POST: // HTTP POST
		retValue = processPostTask(this, request,
			writeData,
			response->getResponseData(),
			&responseCode,
			writeHeaderData,
			response->getResponseHeader(),
			responseMessage);
		break;

	case HttpRequest::PUT:
		retValue = processPutTask(this, request,
			writeData,
			response->getResponseData(),
			&responseCode,
			writeHeaderData,
			response->getResponseHeader(),
			responseMessage);
		break;

	case HttpRequest::_DELETE:
		retValue = processDeleteTask(this, request,
			writeData,
			response->getResponseData(),
			&responseCode,
			writeHeaderData,
			response->getResponseHeader(),
			responseMessage);
		break;

	default:
		printf("CCHttpClient: unknown request type, only GET and POSt are supported");
		break;
	}

	// write data to HttpResponse
	response->setResponseCode(responseCode);
	if (retValue != 0)
	{
		response->setSucceed(false);
		response->setErrorBuffer(responseMessage);
	}
	else
	{
		response->setSucceed(true);
	}
}

HttpClient::HttpClient() {
    _timeoutForConnect = 1000;
    _timeoutForRead = 5 * 1000;
    _cookie = NULL;
    _cookieFilename.clear();
    _sslCaFilename.clear();
    memset(_responseMessage, 0, HTTPCLIENT_RESPONSE_BUFFER_SIZE * sizeof(char));
}

HttpClient::~HttpClient() {

}

void HttpClient::setTimeoutForConnect(int value)
{
    _timeoutForConnect = value;
}

int HttpClient::getTimeoutForConnect()
{
    return _timeoutForConnect;
}

void HttpClient::setTimeoutForRead(int value)
{
    _timeoutForRead = value;
}

int HttpClient::getTimeoutForRead()
{
    return _timeoutForRead;
}

const std::string& HttpClient::getCookieFilename()
{
    return _cookieFilename;
}

const std::string& HttpClient::getSSLVerification()
{
    return _sslCaFilename;
}

void HttpClient::enableCookies(const char* cookieFile) {
    _cookieFilename = cookieFile;
}

void HttpClient::setSSLVerification(const std::string& caFile)
{
    _sslCaFilename = caFile;
}

void HttpClient::send(HttpRequest* request) {

}

void HttpClient::sendImmediate(HttpRequest* request) {

    if(request == NULL) {
        return;
    }

    HttpResponse *response = new HttpResponse(request);

    processRequest(response,_responseMessage);

    HttpResponse_Callback _call = request->getCallback();

    if(_call)
        (*_call)(response,request->getUserData());

    delete response;
}
