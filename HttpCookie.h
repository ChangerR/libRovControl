#ifndef __LIBSOCKETIO_HTTP_COOKIE_H
#define __LIBSOCKETIO_HTTP_COOKIE_H

#include <string>
#include <vector>

struct CookiesInfo
{
    std::string domain;
    bool tailmatch;
    std::string path;
    bool secure;
    std::string name;
    std::string value;
    std::string expires;
};

class HttpCookie
{
public:
    void readFile();

    void writeFile();
    void setCookieFileName(const std::string fileName);

    const std::vector<CookiesInfo>* getCookies()const;
    const CookiesInfo* getMatchCookie(const std::string& url) const;
    void updateOrAddCookie(CookiesInfo* cookie);

private:
    std::string _cookieFileName;
    std::vector<CookiesInfo> _cookies;
};

#endif
