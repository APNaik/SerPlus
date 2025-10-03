#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP
#include <string>
#include <map>

class HttpResponse{
public:
    enum StatusCode{
        OK = 200,
        BAD_REQUEST = 400,
        NOT_FOUND = 404,
        INTERNAL_SERVER_ERROR = 500
    };
    HttpResponse(StatusCode code = OK, const std::string &body = "");

    void setStatus(StatusCode code);
    void setHeader(const std::string &key, const std::string &value);
    void setBody(const std::string &body);
    std::string toString() const;
private:
    StatusCode statuscode;
    std::string reasonPhrase;
    std::map<std::string, std::string> headers;
    std::string body;

    void setDefaultReasonPhrase();
};

#endif