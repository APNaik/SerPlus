#include "http_response.hpp"
#include <sstream>

HttpResponse::HttpResponse(StatusCode code, const std::string &body){
    statuscode = code;
    this->body = body;

    setDefaultReasonPhrase();
}

void HttpResponse::setStatus(StatusCode code){
    statuscode = code;
    setDefaultReasonPhrase();
}

void HttpResponse::setHeader(const std::string &key, const std::string &value){
    headers[key] = value;
}

void HttpResponse::setBody(const std::string &b){
    body = b;
    headers["Content-Length"] = std::to_string(body.size());
}

std::string HttpResponse::toString() const{
    std::ostringstream response;
    response << "HTTP/1.1 " << statuscode << " " << reasonPhrase << "\r\n";
    for(const auto& header: headers){
        response << header.first << ": " << header.second << "\r\n";
    }

    response << "\r\n";
    response << body;
    return response.str();
}

void HttpResponse::setDefaultReasonPhrase(){
    switch(statuscode){
        case OK: reasonPhrase = "OK"; break;
        case BAD_REQUEST: "Bad Request"; break;
        case NOT_FOUND: "Not Found"; break;
        case INTERNAL_SERVER_ERROR: "Internal Server Error"; break;
        default: reasonPhrase = "Unknown"; break;
    }
}