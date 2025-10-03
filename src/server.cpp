#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif
#include "http_request.hpp"
#include "http_response.hpp"

int main(int argc, char **argv){
	// Flush after every std::cout / std::cerr
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;

	// You can use print statements as follows for debugging, they'll be visible when running tests.
	std::cout << "Logs from your program will appear here!\n";

	// Uncomment this block to pass the first stage
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed\n";
		return 1;
	} 
#endif
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0){
		std::cerr << "Failed to create server socket: " << WSAGetLastError() << "\n";
		return 1;
	}

	// Since the tester restarts your program quite often, setting SO_REUSEADDR
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0){
		std::cerr << "setsockopt failed\n";
		return 1;
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(4221);

	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0){
		std::cerr << "Failed to bind to port 4221\n";
		return 1;
	}

	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0){
		std::cerr << "listen failed\n";
		return 1;
	}

	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);

	std::cout << "Waiting for a client to connect...\n";
	while(true){
		int client_socket = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
		if(client_socket < 0){
			std::cerr << "Failed to accept connection\n";
			continue;
		}
		std::cout << "client connected\n";
		char buffer[4096];
		int bytes_received;
		std::string req_data;
		while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
			req_data.append(buffer, bytes_received);

			// Look for end of headers (\r\n\r\n)
			size_t header_end = req_data.find("\r\n\r\n");
			if (header_end != std::string::npos) {
				// Extract headers
				std::string headers_part = req_data.substr(0, header_end + 4);

				// Look for Content-Length
				size_t cont_len_pos = headers_part.find("Content-Length:");
				if (cont_len_pos != std::string::npos) {
					// Extract the number
					size_t line_end = headers_part.find("\r\n", cont_len_pos);
					std::string len_str = headers_part.substr(cont_len_pos + 15, line_end - (cont_len_pos + 15));
					int cont_len = std::stoi(len_str);

					// Total needed = headers + body
					size_t total_needed = header_end + 4 + cont_len;
					if (req_data.size() >= total_needed) {
						break;  // got the full request
					}
				} 
				else {
					break; // no body
				}
			}
		}

		HttpRequest request = HttpRequest::parse(req_data);
		std::string body;
		body += "Method: " + http_method_to_str(request.method) + "\n";
		body += "Path: " + request.url.path + "\n";
		body += "HTTP version: " + request.http_version + "\n";
		body += "Headers:\n";
		for(auto h: request.headers){
			body += " " + h.first + ": " + h.second + "\n";
		}
		body += "Body:\n";
		body += request.getBodyString();

		std::cout << "--Entire content of the http request--\n";
		std::cout << body << std::endl;

		HttpResponse response(HttpResponse::OK);
		
		// Set the headers from request
		response.setHeader("Content-Type", "text/plain");
		response.setHeader("Connection", "close");
		response.setBody(request.getBodyString());
		std::string resString = response.toString();
		size_t len_res = resString.size();
		send(client_socket, resString.c_str(), len_res, 0);
#ifdef _WIN32
		closesocket(client_socket);
#else
		close(client_socket);
#endif
	}
#ifdef _WIN32
	closesocket(server_fd);
	WSACleanup();
#else
	close(server_fd);
#endif
	return 0;
}
