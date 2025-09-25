#include "http_request.hpp"
#include <sstream>
#include <algorithm>
#include <iterator>

std::string HttpRequest::toLowerStr(const std::string &s) {
    std::string out = s; 
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c){
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

HttpMethod str_to_http_method(const std::string &in){
    std::string method = in;
    std::transform(method.begin(), method.end(), method.begin(), ::toupper);

    if(method == "GET") return HttpMethod::GET;
    if(method == "POST") return HttpMethod::POST;
    if(method == "PUT") return HttpMethod::PUT;
    if(method == "DELETE") return HttpMethod::DELETE;
    if(method == "PATCH") return HttpMethod::PATCH;
    if(method == "HEAD") return HttpMethod::HEAD;
    if(method == "OPTIONS") return HttpMethod::OPTIONS;
    if(method == "TRACE") return HttpMethod::TRACE;
    if(method == "CONNECT") return HttpMethod::CONNECT;
    return HttpMethod::UNKNOWN;
}

std::string http_method_to_str(HttpMethod in){
    switch(in){
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE: return "DELETE";
        case HttpMethod::PATCH: return "PATCH";
        case HttpMethod::HEAD: return "HEAD";
        case HttpMethod::CONNECT: return "CONNECT";
        case HttpMethod::TRACE: return "TRACE";
        case HttpMethod::OPTIONS: return "OPTIONS";
        default: return "UNKNOWN";
    }
}

bool HttpRequest::getHeader(const std::string& name, std::string &out) {
    auto it = headers.find(toLowerStr(name));
    if (it == headers.end()) return false;
    out = it->second;
    return true;
}

void HttpRequest::setHeader(const std::string &key, const std::string &value){
    headers[toLowerStr(key)] = value;
}

bool HttpRequest::hasHeader(const std::string name){
    return (headers.find(toLowerStr(name)) != headers.end());
}

void HttpRequest::setBody(const std::string &text){
    body.assign(text.begin(), text.end()); 
}

std::string HttpRequest::getBodyString(){
    return std::string(body.begin(), body.end());
}

static std::string urlDecode(const std::string &s){
    std::string out;
    out.reserve(s.size());
    for(size_t i=0; i<s.size(); i++){
        char c = s[i];
        if(c == '+'){
            out.push_back(' ');
        }
        else if(c == '%' && i+2 < s.size()){
            auto hex = s.substr(i+1, 2);
            char decoded = static_cast<char> (std::strtol(hex.c_str(), nullptr, 16));
            out.push_back(decoded);
            i += 2;
        }
        else{
            out.push_back(c);
        }
    }
    return out;
}

std::map<std::string, std::string> HttpRequest::parseQueryString(const std::string &Qstring){
    std::map<std::string, std::string> out;
    size_t pos = 0;
    while(pos < Qstring.size()){
        size_t amp = Qstring.find("&", pos);
        std::string part = (amp == std::string::npos)? Qstring.substr(pos): Qstring.substr(pos, amp - pos);
        if(!part.empty()){
            size_t eq = part.find("=");
            if(eq == std::string::npos){
                out[urlDecode(part)] = "";
            }
            else{
                std::string key = part.substr(0, eq);
                std::string val = part.substr(eq + 1);
                out[urlDecode(key)] = urlDecode(val);
            }
        }
        if(amp == std::string::npos) break;
        pos = amp + 1;
    }
    return out;
}

HttpRequest HttpRequest::parse(const std::string &objString){
    HttpRequest req;
    req.raw = objString;
    std::istringstream stream(objString);
    std::string line;

    if(!std::getline(stream, line)) return req;
    if(!line.empty() && line.back() == '\r') line.pop_back();

    std::istringstream rl(line);
    std::string method_, target_, version_;
    rl >> method_ >> target_ >> version_;

    req.method = str_to_http_method(method_);
    if(!version_.empty()) req.http_version = version_;

    auto parse_abs_URI = [&](const std::string &uri){
        // simple parser for: scheme://host[:port]/path[?qs]
        size_t schemePos = uri.find("://");
        if(schemePos != std::string::npos){
            req.url.protocol = uri.substr(0, schemePos);
            size_t hostStart = schemePos + 3;
            size_t pathStart = uri.find("/", hostStart);
            std::string hostPort;
            if(pathStart == std::string::npos){
                hostPort = uri.substr(hostStart);
            }
            else{
                hostPort = uri.substr(hostStart, pathStart - hostStart);
            }
            size_t colon = hostPort.find(":");
            if(colon != std::string::npos){
                req.url.host = hostPort.substr(0, colon);
                try{
                    unsigned long p = std::stoul(hostPort.substr(colon+1));
                    req.url.port = static_cast<uint16_t> (p);
                    req.url.hasPort = true;
                }
                catch(...){
                    req.url.hasPort = false;
                }
            }
            else{
                req.url.host = hostPort;
            }
            std::string path_n_queries = (pathStart == std::string::npos) ? "/": uri.substr(pathStart);
            size_t qpos = path_n_queries.find("?");
            if(qpos == std::string::npos){
                req.url.path = path_n_queries;
            }
            else{
                req.url.path = path_n_queries.substr(0, qpos);
                std::string queries = path_n_queries.substr(qpos+1);
                req.url.query_params = parseQueryString(queries);
            }
            return true;
        }
        return false;
    };

    // If absolute uri is present
    if(!parse_abs_URI(target_)){
        // Path + optional query
        size_t qpos = target_.find("?");
        if(qpos == std::string::npos) req.url.path = target_.empty()? "/": target_;    
        else{
            req.url.path = target_.substr(0, qpos);
            req.url.query_params = parseQueryString(target_.substr(qpos + 1));
        }
    }
    
    // Headers
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break; // end of headers

        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string val = line.substr(pos + 1);
            // trim leading spaces from value
            if (!val.empty() && val[0] == ' ') val.erase(0, 1);
            req.headers[toLowerStr(key)] = val;
        }
    }

    // If we didn't get host from absolute-URI, get it from Host header
    std::string hostHeader;
    if (req.url.host.empty() && req.getHeader("host", hostHeader)) {
        // host may be "example.com" or "example.com:8080"
        size_t colon = hostHeader.find(':');
        if (colon != std::string::npos) {
            req.url.host = hostHeader.substr(0, colon);
            try {
                unsigned long p = std::stoul(hostHeader.substr(colon + 1));
                req.url.port = static_cast<uint16_t>(p);
                req.url.hasPort = true;
            } catch(...) { req.url.hasPort = false; }
        } 
        else {
            req.url.host = hostHeader;
        }
    }

    // Determine keep-alive: HTTP/1.1 default is persistent unless Connection: close
    std::string connVal;
    if (req.getHeader("connection", connVal)) {
        std::string lc = toLowerStr(connVal);
        if (lc.find("close") != std::string::npos) req.keepAlive = false;
        else if (lc.find("keep-alive") != std::string::npos) req.keepAlive = true;
    } 
    else {
        // default: http/1.1 -> keep-alive, http/1.0 -> close
        if (req.http_version == "HTTP/1.1") req.keepAlive = true;
        else req.keepAlive = false;
    }

    // Respect content-length if present
    std::string len_content;
    if(req.getHeader("content-length", len_content)){
        try{
            size_t len = static_cast<size_t> (std::stoul(len_content));
            std::string bodystr;
            bodystr.resize(len);
            stream.read(&bodystr[0], static_cast<std::streamsize> (len));
            req.body.assign(bodystr.begin(), bodystr.end());
        }
        catch(...){
            std::string rest((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
            req.body.assign(rest.begin(), rest.end());
        }
    }
    else{
        // read remaining as body (may be empty)
        std::string rest((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        req.body.assign(rest.begin(), rest.end());
    }

    return req;
}   

std::string HttpRequest::serialize(){
    std::ostringstream out;
    std::string path = url.path.empty() ? "/": url.path;
    out << http_method_to_str(method) << " " << path;
    if(!url.query_params.empty()){
        out << "?";
        bool first = true;
        for(auto i: url.query_params){
            if(!first){
                out << "&";
            }
            out << i.first << "=" << i.second;
            first = false;
        }
    }
    out << " " << http_version << "\r\n";
    if(!url.host.empty()){
        if(hasHeader("host") == false){
            if(url.hasPort && url.port != 0){
                out << "host: " << url.host << ":" << url.port << "\r\n";
            }
            else{
                out << "host: " << url.host << "\r\n";
            }
        }
    }
    // Connection header
    if (keepAlive) {
        if (!hasHeader("connection") && http_version == "HTTP/1.0") {
            out << "connection: keep-alive\r\n";
        }
    } 
    else {
        if (!hasHeader("connection") && http_version == "HTTP/1.1") {
            out << "connection: close\r\n";
        }
    }
    // Remaining headers, if any
    for ( auto &v : headers) {
        out << v.first << ": " << v.second << "\r\n";
    }

    // Separate header and body
    out << "\r\n";
    // Body (binary-safe)
    if (!body.empty()) {
        out.write(reinterpret_cast<const char*>(body.data()), static_cast<std::streamsize>(body.size()));
    }

    return out.str();
}