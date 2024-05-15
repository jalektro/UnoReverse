#ifdef _WIN32
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#include <winsock2.h> //for all socket programming
#include <ws2tcpip.h> //for getaddrinfo, inet_pton, inet_ntop
#include <stdio.h> //for fprintf, perror
#include <unistd.h> //for close
#include <stdlib.h> //for exit
#include <string.h> //for memset
void OSInit( void )
{
    WSADATA wsaData;
    int WSAError = WSAStartup( MAKEWORD( 2, 0 ), &wsaData );
    if( WSAError != 0 )
    {
        fprintf( stderr, "WSAStartup errno = %d\n", WSAError );
        exit( -1 );
    }
}
void OSCleanup( void )
{
    WSACleanup();
}
#define perror(string) fprintf( stderr, string ": WSA errno = %d\n", WSAGetLastError() )
#else
#include <sys/socket.h> //for sockaddr, socket, socket
	#include <sys/types.h> //for size_t
	#include <netdb.h> //for getaddrinfo
	#include <netinet/in.h> //for sockaddr_in
	#include <arpa/inet.h> //for htons, htonl, inet_pton, inet_ntop
	#include <errno.h> //for errno
	#include <stdio.h> //for fprintf, perror
	#include <unistd.h> //for close
	#include <stdlib.h> //for exit
	#include <string.h> //for memset
	void OSInit( void ) {}
	void OSCleanup( void ) {}
#endif

#include <pthread.h>

#define SERVERPORT "22"

struct ThreadArgs {
    int client_socket;
    FILE* log_file;
    char client_address[INET6_ADDRSTRLEN];
};

int initServer();
int connectClient(int server_socket, char *client_address);
void getRequest(const char *client_address, FILE *log_file, char *thread_id_str);
void sendData(int client_socket, FILE *log_file, char *thread_id_str);
void* threadExecution(void* arg);

int main() {
    FILE *file_ptr = fopen("log.log", "a");
	if (file_ptr == NULL)
	{
		perror("error opening file\n");
	return 1;
	}
	
	 OSInit();
	 
    int server_socket = initServer();

  while (1) {
    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof client_addr;
    int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
    if (client_socket == -1) {
        perror("accept");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    char client_address[INET6_ADDRSTRLEN];
	
	
    inet_ntop(client_addr.ss_family, &((struct sockaddr_in*)&client_addr)->sin_addr, client_address, INET6_ADDRSTRLEN);

    pthread_t thread;
    struct ThreadArgs* args = (struct ThreadArgs*)malloc(sizeof(struct ThreadArgs));
    args->client_socket = client_socket;
    args->log_file = log_file;
    strncpy(args->client_address, client_address, INET6_ADDRSTRLEN - 1); // Copy client address to args struct
    args->client_address[INET6_ADDRSTRLEN - 1] = '\0'; // Ensure null-termination

    pthread_create(&thread, NULL, threadExecution, args);
}

    fclose(log_file);
    return 0;
}

int initServer() {
    struct addrinfo internet_adress_setup, 
	struct addrinfo *internet_adress_result;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(NULL, SERVERPORT, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    int server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (bind(server_socket, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, SOMAXCONN) == -1) {
        perror("listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);
    return server_socket;
}

int connectClient(int server_socket, char *client_address) {
    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof client_addr;
    int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
    if (client_socket == -1) {
        perror("accept");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    inet_ntop(client_addr.ss_family, &((struct sockaddr_in*)&client_addr)->sin_addr, client_address, INET6_ADDRSTRLEN);
    return client_socket;
}

void getRequest(const char *client_address, FILE *log_file, char *thread_id_str) {
    int api_socket = initServer();
    FILE *api_file = fdopen(api_socket, "r+");

    char request[1024];
    sprintf(request, "GET /json/%s HTTP/1.0\r\nHost: ip-api.com\r\nConnection: close\r\n\r\n", client_address);
    fprintf(api_file, "%s", request);
    fflush(api_file);

    char response[1024];
    fgets(response, sizeof(response), api_file);

    char* json_start = strchr(response, '{');
    if (json_start == NULL) {
        perror("strchr");
        fclose(api_file);
        close(api_socket);
        return;
    }

    fprintf(log_file, "Thread ID: %s, Geolocation: %s\n", thread_id_str, json_start);
    fclose(api_file);
    close(api_socket);
}

void sendData(int client_socket, FILE *log_file, char *thread_id_str) {
    char buffer[1024];
    int total_bytes_sent = 0;
    while (1) {
        int bytes_sent = send(client_socket, "I like beer!!!\n", 17, 0);
        if (bytes_sent == -1) {
            perror("send");
            break;
        } else if (bytes_sent == 0) {
            break;
        }
        total_bytes_sent += bytes_sent;
        usleep(100000);
    }
    fprintf(log_file, "Thread ID: %s, Total bytes sent: %d\n", thread_id_str, total_bytes_sent);
}

void* threadExecution(void* arg) {
    struct ThreadArgs* args = (struct ThreadArgs*)arg;
    char thread_id_str[20];
    pthread_t thread_id = pthread_self();
    sprintf(thread_id_str, "%ld", thread_id);

    getRequest(args->client_address, args->log_file, thread_id_str);
    sendData(args->client_socket, args->log_file, thread_id_str);

    close(args->client_socket);
    free(args);
    pthread_exit(NULL);
}
