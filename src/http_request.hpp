#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <algorithm>

HttpMethod toHttpMethod(const std::string &in);
std::string toString(HttpMethod in);

enum class HttpMethod{
    GET, POST, PUT, DELETE, HEAD, OPTIONS, PATCH, TRACE, CONNECT, UNKNOWN
};

struct Url{
    std::string protocol = "http";
    std::string host;
    uint16_t port = 80;
    std::string path = "/";
    std::map<std::string, std::string> query_params;
};

class HttpRequest{
    HttpMethod method = HttpMethod::UNKNOWN;
    Url url;
    std::unordered_map<std::string, std::string> headers;
    std::string http_version = "http/1.1";
    std::vector<uint8_t> body;

    // Headre helper functions
    std::string getHeader(const std::string &name);

};

#endif 