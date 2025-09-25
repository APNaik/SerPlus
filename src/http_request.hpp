#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <algorithm>

enum class HttpMethod{
    GET, POST, PUT, DELETE, HEAD, OPTIONS, PATCH, TRACE, CONNECT, UNKNOWN
};

HttpMethod str_to_http_method(const std::string &in);
std::string http_method_to_str(HttpMethod in);

struct Url{
    std::string protocol = "http";
    std::string host;
    uint16_t port = 80;
    bool hasPort = false;
    std::string path = "/";
    std::map<std::string, std::string> query_params;
};

class HttpRequest{
public:
    HttpMethod method = HttpMethod::UNKNOWN;
    Url url;
    std::unordered_map<std::string, std::string> headers;
    std::string http_version = "http/1.1";
    std::vector<uint8_t> body;
    bool keepAlive = false;
    std::string raw;

    // Header helper functions
    bool getHeader(const std::string& name, std::string &out);
    void setHeader(const std::string &key, const std::string &value);
    bool hasHeader(const std::string name);

    // Body helper functions
    void setBody(const std::string &bodyText);
    std::string getBodyString();

    // Serialization and parsing
    std::string serialize();
    static HttpRequest parse(const std::string &objString);

    // Utility functions
    static std::string toLowerStr(const std::string &s);
    static std::map<std::string, std::string> parseQueryString(const std::string &Qstring);
};

#endif 