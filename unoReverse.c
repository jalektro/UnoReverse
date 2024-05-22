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
    FILE* file_ptr;
    char client_address_str[INET6_ADDRSTRLEN];
};

int initServer();
int init_HTTP_client();
int connectClient(int server_socket, char *client_address_str, int client_addr_str_size);
void getRequest(const char *client_address_str, FILE *log_file, char *thread_id_str);
void sendData(int client_socket, FILE *file_ptr, char *client_address_str);
void cleanup(int client_socket);
void* threadExecution(void* arg);
int openingFile(FILE *filepointer);


int main() {
	FILE* file_ptr = fopen("log.log", "a"); // no clean code but works for me.fault checking is inside next function but here i have file_ptr pointed to the right location. 
	openingFile(file_ptr); 
	fclose(file_ptr);
	
	 OSInit();
	 
    int server_socket = initServer();

	char client_address_str[INET6_ADDRSTRLEN];
	 
	while(1){
		
    int client_socket = connectClient(server_socket, client_address_str, sizeof(client_address_str));	
	if (client_socket ==-1){
	continue;
	}
    pthread_t thread;
	
    struct ThreadArgs* args = (struct ThreadArgs*)malloc(sizeof(struct ThreadArgs));
    args->client_socket = client_socket;
    args->file_ptr = file_ptr;
    strncpy(args->client_address_str, client_address_str, INET6_ADDRSTRLEN - 1); // Copy client addinternet_address_results to args struct
	
    args->client_address_str[INET6_ADDRSTRLEN - 1] = '\0'; // Ensure null-termination

    int thread_create_result = pthread_create(&thread, NULL, threadExecution, args);  //create a thread for each client
		if(thread_create_result!=0)
		{	
		 fprintf(stderr, "Failed to create thread: %d\n", thread_create_result);
		 close(client_socket);
		 free(args);
		}
		pthread_detach(thread);
	}

    fclose(file_ptr);
	
	cleanup(server_socket);
	
	OSCleanup();
	
    return 0;
}


int initServer() {
   struct addrinfo internet_address_setup;
    struct addrinfo * internet_address_result;
    memset( &internet_address_setup, 0, sizeof internet_address_setup );
    internet_address_setup.ai_family = AF_UNSPEC;
    internet_address_setup.ai_socktype = SOCK_STREAM;
    internet_address_setup.ai_flags = AI_PASSIVE;

    int getaddrinfo_return = getaddrinfo(NULL, SERVERPORT, &internet_address_setup, &internet_address_result); // server is listening on port 22
    if (getaddrinfo_return != 0) 
	{
        fprintf(stderr, "Server getaddrinfo error: %s\n", gai_strerror(getaddrinfo_return));
        exit(EXIT_FAILURE);
    }
 int internet_socket = -1;
        struct addrinfo * internet_address_result_iterator = internet_address_result;
        while( internet_address_result_iterator != NULL )
        {
            //Step 1.2
            internet_socket = socket( internet_address_result_iterator->ai_family, internet_address_result_iterator->ai_socktype, internet_address_result_iterator->ai_protocol );
            int mode = 0;
            setsockopt(internet_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&mode, sizeof(mode));
            if( internet_socket == -1 )
            {
                perror( "socket" );
            }
            else
            {
                //Step 1.3
                int bind_return = bind( internet_socket, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen );
                if( bind_return == -1 )
                {
                    perror( "bind" );
                    close( internet_socket );
                }
                else
                {
                    //Step 1.4
                    int listen_return = listen( internet_socket, 1 );
                    if( listen_return == -1 )
                    {
                        close( internet_socket );
                        perror( "listen" );
                    }
                    else
                    {
                        break;
                    }
                }
            }
            internet_address_result_iterator = internet_address_result_iterator->ai_next;
        }

        freeaddrinfo( internet_address_result );

        if( internet_socket == -1 )
        {
            fprintf( stderr, "socket: no valid socket address found\n" );
            exit( 2 );
        }

        return internet_socket	;	
}

int init_HTTP_client()
{
	struct addrinfo internet_address_setup;
    struct addrinfo * internet_address_result;
    memset( &internet_address_setup, 0, sizeof internet_address_setup );
    internet_address_setup.ai_family = AF_UNSPEC;
    internet_address_setup.ai_socktype = SOCK_STREAM;
    internet_address_setup.ai_flags = AI_PASSIVE;
	
        int getaddrinfo_return = getaddrinfo( "ip-api.com", "80", &internet_address_setup, &internet_address_result );
        if( getaddrinfo_return != 0 )
        {
            fprintf( stderr, "init HTTP CLient getaddrinfo: %s\n", gai_strerror( getaddrinfo_return ) );
            exit( 1 );
        }

        int internet_socket = -1;
        struct addrinfo * internet_address_result_iterator = internet_address_result;
        while( internet_address_result_iterator != NULL )
        {
            internet_socket = socket( internet_address_result_iterator->ai_family, internet_address_result_iterator->ai_socktype, internet_address_result_iterator->ai_protocol );
            if( internet_socket == -1 )
            {
                perror( "socket" );
            }
            else
            {
                int connect_return = connect( internet_socket, internet_address_result_iterator->ai_addr, internet_address_result_iterator->ai_addrlen );
                if( connect_return == -1 )
                {
                    perror( "connect" );
                    close( internet_socket );
                }
                else
                {
                    break;
                }
            }
            internet_address_result_iterator = internet_address_result_iterator->ai_next;
        }

        freeaddrinfo( internet_address_result );

        if( internet_socket == -1 )
        {
            fprintf( stderr, "socket: no valid socket address found\n" );
            exit( 2 );
        }

        return internet_socket;
}



int connectClient(int server_socket, char *client_address_str, int client_address_str_lenght ) 
{
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof client_addr;
    int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_size);
    if (client_socket == -1) {
        perror("accept");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
		//get IP address from client
        if (client_addr.ss_family == AF_INET) //IPv4
		{ 		
			struct sockaddr_in* s = (struct sockaddr_in*)&client_addr;
            inet_ntop(AF_INET, &s->sin_addr, client_address_str, client_address_str_lenght);
		} else { // AF_INET6
            struct sockaddr_in6* s = (struct sockaddr_in6*)&client_addr;
            inet_ntop(AF_INET6, &s->sin6_addr, client_address_str, client_address_str_lenght);
        }

        printf("First Client IP address: %s\n", client_address_str);
		printf("client_socket = %d\n",client_socket);
		
    return client_socket;
}



void getRequest(const char *client_address_str, FILE *file_ptr, char *thread_id_str) {
	
    int API_socket_HTTP  = init_HTTP_client();
	
	openingFile(file_ptr);
   
   
	
										
	fclose(file_ptr);

    char buffer[1000];
    char HTTPrequest[100] ={0};

// http://ip-api.com/json/%s?fields=status,message,country,countryCode,region,regionName,city,zip,lat,lon,timezone,isp,org,as,query
    sprintf(HTTPrequest, "GET /json/%s HTTP/1.0\r\nHost: ip-api.com\r\nConnection: close\r\n\r\n", client_address_str);
    printf("HTTP request = %s",HTTPrequest);

    //SEND HTTPREQUEST TO GET LOCATION OF BOTNET
    int number_of_bytes_send = 0;
    number_of_bytes_send = send( API_socket_HTTP, HTTPrequest , strlen(HTTPrequest), 0 );
    if( number_of_bytes_send == -1 )
    {
        perror( "send" );
    }

    //receive json data and cut the HTTP header off
    int number_of_bytes_received = 0;
    number_of_bytes_received = recv( API_socket_HTTP, buffer, ( sizeof buffer ) - 1, 0 );
    if( number_of_bytes_received == -1 )
    {
        perror( "recv" );
    }
    else
    {
        buffer[number_of_bytes_received] = '\0';
        printf( "Received : %s\n", buffer );
    }

	char* jsonFile = strchr(buffer,'{');
	openingFile(file_ptr);
	
    if( jsonFile == NULL){
        number_of_bytes_received = recv( API_socket_HTTP, buffer, ( sizeof buffer ) - 1, 0 );
        if( number_of_bytes_received == -1 )
        {
            perror( "recv" );
        }  else {
            buffer[number_of_bytes_received] = '\0';
            printf("Received : %s\n", buffer );
		}		
     }
	fprintf(file_ptr,"*****************New thread ID*****************\n");
	fprintf(file_ptr, "Thread ID: %s\t",thread_id_str);
	fprintf(file_ptr, "the client IP: %s\n", client_address_str );
	fprintf(file_ptr, "%s\n", jsonFile);

	fclose(file_ptr);
    //clean HTTP socket
    cleanup(API_socket_HTTP);
}

void sendData(int client_socket, FILE *file_ptr, char *client_address_str){	
	openingFile(file_ptr);

    pthread_t thread_id = pthread_self();
    char thread_id_str[20];
    sprintf(thread_id_str, "%ld", thread_id);

    int number_of_bytes_received = 0;
    char buffer[100];

    number_of_bytes_received = recv(client_socket, buffer, (sizeof buffer) - 1, 0);
    if (number_of_bytes_received == -1) {
        perror("recv");
    } else {
        buffer[number_of_bytes_received] = '\0';
        printf("Received : %s\n", buffer);
    }

	// get the geolocation
   // getRequest(client_address_str, file_ptr,thread_id_str);

	fprintf(file_ptr, "Client sent : %s\n", buffer);
	
	fclose(file_ptr);
    //Step 3.1
    int number_of_bytes_send = 0;
    int totalBytesSend = 0;

    char totalBytesSendStr[20];


    while(1){

        //UNO REVERSE TIME
        number_of_bytes_send = send( client_socket, "Never gonna give you up\nNever gonna let you down\nNever gonna run around and desert you\n"
		"Never gonna make you cry\nNever gonna say goodbye\nNever gonna tell a lie and hurt you\nNever gonna give you up\nNever gonna let you down\n"
		"Never gonna run around and desert you\nNever gonna make you cry\nNever gonna say goodbye\nNever gonna tell a lie and hurt you\n"
		"Never gonna give you up\nNever gonna let you down\nNever gonna run around and desert you\nNever gonna make you cry\n"
		"Never gonna say goodbye\nNever gonna tell a lie and hurt you\n", 517, 0);

        //check if client left and put number of bytes send in the log file
        if( number_of_bytes_send == -1 )
        {
            printf("Client left. I sent %d bytes\n",totalBytesSend);

		 openingFile(file_ptr);
		fprintf(file_ptr,"Total bytes I've send to client : %d\n", totalBytesSend);
		fprintf(file_ptr,"***********************************************\n");
	
		fclose(file_ptr);
	
            break;
        }else{
            totalBytesSend +=number_of_bytes_send;
         // usleep(100000);// Just here to test or i wil attack myself to hard
        }
    }
    close(client_socket);

}

void* threadExecution(void* arg) 
{
    struct ThreadArgs* args = (struct ThreadArgs*)arg;
    char thread_id_str[20];
    pthread_t thread_id = pthread_self();
    sprintf(thread_id_str, "%ld", thread_id);

	getRequest(args->client_address_str, args->file_ptr, thread_id_str);
    sendData(args->client_socket, args->file_ptr, thread_id_str);

    close(args->client_socket);
	
    free(args);
	return NULL;
    //pthread_exit(NULL);
}

void cleanup( int client_socket)
{
	int shutdown_return = shutdown( client_socket, 0 );
	if( shutdown_return == -1 )
	{
		perror( "shutdown" );
	}

	close( client_socket );
}

int openingFile(FILE *filepointer)
{
	FILE *file_ptr = fopen("log.log", "a");
	if (file_ptr == NULL)
	{
		perror("error opening file\n");
	return 1;
	}
	else
	{
	return 0;
	}
}
