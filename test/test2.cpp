#include "HttpClient.h"
#include <stdio.h>

#ifdef _WIN32
#pragma comment(lib,"libcurl.lib")
#endif

void test_callback(HttpResponse* res,void* data) {
    printf("CURL SUCCESS:%s\n",res->isSucceed()?"TRUE":"FALSE");
    printf("CURL RETURN CODE:%d\n",(int)res->getResponseCode());
    printf("SHOW RESPONSE HEADER\n");
    res->setResponseDataString(&((*res->getResponseHeader())[0]),res->getResponseHeader()->size());
    printf("%s--end\n",res->getResponseDataString());
    printf("SHOW RESPONSE DATA\n");
    res->setResponseDataString(&((*res->getResponseData())[0]),res->getResponseData()->size());
    printf("%s--end\n",res->getResponseDataString());
}

int main() {
    HttpClient client;
    HttpRequest req;

    req.setRequestType(HttpRequest::GET);
    req.setUrl("http://www.baidu.com/");
    req.setCallback(test_callback);

    client.sendImmediate(&req);
}
