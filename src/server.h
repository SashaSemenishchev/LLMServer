#define MAX_CLIENTS 10
#define BUFFER_SIZE 4096
#define SMALL_BUFFER_SIZE 512
#include <arpa/inet.h>
// Structure to hold client information
#define SocketAddr struct sockaddr_in
#include "arraylist.h"
#include "hashmap.h"
#include <setjmp.h>
#define HashMap struct hashmap

typedef struct Client {
    int socket;
    SocketAddr address;
    HashMap* clients;
    // pthread_key_t thread_key;
    pthread_t tid;
    ArrayList completions;
    bool has_execution_point;
    jmp_buf last_execution_point;
} Client;

#define SEARCH_CLIENT (Client){}
// #define ClientCallback void* ((*)(Client* client))

// Pointer to ClientThreadTransfer
// always returns 0 (NULLPTR)
void sigabort(int dummy);
void sigint(int dummy);
void release_socket(Client* client);
void* client_handle(void* arg);
void shutdown_server();
int compare_clients(const void* a, const void* b, void* udata);
static inline void send_message(Client* client, uint8_t* message, size_t len);
static inline void on_read(Client* client, uint8_t* buff, size_t bytes_read);
void send_protobuf(Client* client, size_t (*packer)(void* message, uint8_t* out), void* message);
static inline void send_error(Client* client, const char* request_id, const char* error_message, int error_code);
