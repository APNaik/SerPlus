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

class RequestParser{
public:
	std::string url_extrac();
};

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
		send(client_socket, "HTTP/1.1 200 OK\r\n\r\n", 20, 0);
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
