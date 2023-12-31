#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <setjmp.h>
#include <dirent.h>
#include "ai/ai.h"
#include "server.h"
#include "protocol/client.pb-c.h"
#include "protocol/server.pb-c.h"
#include "hashmap.h"
#include "util.h"

#define BENCH

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile HashMap* clients;
int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }
    signal(SIGABRT, sigabort);
    signal(SIGINT, sigint);
    int port = atoi(argv[1]);
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);

    clients = hashmap_new(sizeof(size_t), 2, 0, 0, NULL, compare_clients, NULL, NULL);
    // client_t *clients[MAX_CLIENTS] = {0};

    // Create the server socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return 1;
    }

    // Set up the server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the server address
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Bind failed");
        close(server_socket);
        return 1;
    }

    // Start listening for incoming connections
    if (listen(server_socket, 5) == -1) {
        perror("Listen failed");
        close(server_socket);
        return 1;
    }

    printf("Server is listening on port %d\n", port);

    // Main server loop
    while (true) {
        // Accept a new client connection
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
        if (client_socket == -1) {
            perror("Accept failed");
            continue;
        }

        // Create a new client structure and add it to the client list
        Client* client = (Client*) malloc(sizeof(Client));
        if (client == NULL) {
            perror("Memory allocation failed");
            close(client_socket);
            continue;
        }
        pthread_t tid;
        client->socket = client_socket;
        client->address = client_address;
        client->completions = arraylist(1);
        client->clients = clients;
        // pthread_getspecific
        if (pthread_create(&tid, NULL, client_handle, (void*) client) != 0) {
            perror("Thread creation failed");
            send_error(client, NULL, "Failed to create handle for the connection (our end)", 0x11);
            release_socket(client);
            continue;
        }
    }
    shutdown_server();
    return 0;
}

void sigabort(int dummy) {
    pthread_t thread = pthread_self();
    printf("Abortion thread: %p\n", (void*) thread);

    size_t iter = 0;
    Client* item = hashmap_get_with_hash(clients, NULL, (uint64_t) thread);
    if(item != NULL && item->has_execution_point) {
        longjmp(item->last_execution_point, 1);
    }
    return;
}

void sigint(int dummy) {
    shutdown_server();
    exit(0);
}

void release_socket(Client* client) {
    close(client->socket);
    void** elmData = client->completions.element_data;
    if(elmData) {
        arraylist_free(&client->completions);
    }
    free(client);
}

void* client_handle(void* _transfer) {
    #ifdef BENCH
    int64_t handle_start = micros();
    #endif
    Client* client = (Client*) _transfer;
    HashMap* clients = client->clients;
    pthread_t self_thread = pthread_self();
    client->tid = self_thread;
    uint64_t client_hash = (uint64_t) self_thread;
    
    pthread_mutex_lock(&mutex);
    hashmap_set_with_hash(clients, _transfer, client_hash);
    pthread_mutex_unlock(&mutex);
    println("THREAD ID: %p", (void*)client->tid);
    println("Client connected: %s:%d", inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port));
    #ifdef BENCH
    println("Handled client: %lu microseconds", micros() - handle_start);
    #endif
    uint8_t buffer[BUFFER_SIZE];
    int bytes_received;
    // Main client communication loop
    while ((bytes_received = recv(client->socket, buffer, BUFFER_SIZE, 0)) > 0) {
        on_read(client, &buffer, (size_t) bytes_received);
    }

    if (bytes_received == 0) {
        printf("Client disconnected: %s:%d\n", inet_ntoa(client->address.sin_addr), ntohs(client->address.sin_port));
    } else {
        perror("Recv failed");
        send_error(client, NULL, "Failed to receive message from the socket", 0x10);
    }

    // Clean up and close the client socket
    pthread_mutex_lock(&mutex);
    hashmap_delete_with_hash(clients, _transfer, client_hash);
    pthread_mutex_unlock(&mutex);
    release_socket(client);
    println("Client's size: %i", hashmap_count(clients));
    pthread_exit(NULL);
    return 0;
}

void handle_avail_prompts_req(Client* client, const uint8_t* buff, size_t len) {
    AiClient__AvailablePresetPromptsRequest* message = 
    ai_client__available_preset_prompts_request__unpack(NULL, len, buff);
    if(message == NULL) {
        perror("Failed to parse message");
        send_error(client, NULL, "Invalid message", 0x0);
        return;
    }
    AiServer__AvailablePresetPromptsResponse response;
    ai_server__available_preset_prompts_response__init(&response);
    ArrayList entries = arraylist(3);
    struct dirent *entry;
    DIR* dir = opendir("prompts");
    if(dir) {
        while((entry = readdir(dir))) {
            if(entry->d_type != DT_REG) continue; // Filtering actual files
            // fori
            arraylist_add(&entries, entry->d_name);
        }
    }    
    response.askid = message->askid;
    response.names = entries.element_data;
    response.n_names = entries.size;
    // freeing malloc allocated strings from str_replace
    send_protobuf(client, ai_server__available_preset_prompts_response__pack, &response);
    // for(size_t i = 0; i < entries.size; i++) {
    //     free(entries.element_data[i]);
    // }
    // freeing element data
    arraylist_free(&entries);
    if(dir) {
        closedir(dir); // freeing structs which were allocated
    }
    ai_client__available_preset_prompts_request__free_unpacked(message, NULL);
}

static void* outgoing_handlers[] = {ai_server__available_preset_prompts_response__pack, (void*) 0x4};
void send_protobuf(Client* client, size_t (*packer)(void* message, uint8_t* out), void* message) {
    uint8_t write_buff[sizeof(*message) + SMALL_BUFFER_SIZE];
    uint16_t packed = (uint16_t) packer(message, &write_buff[3]);
    if(packed <= 0) return;
    packed++;
    write_buff[0] = (packed >> 8) & 0xFF; // Higher byte (index 0)
    write_buff[1] = packed & 0xFF;
    for(uint8_t i = 0; i < sizeof(outgoing_handlers) / sizeof(void*); i += 2) {
        if(outgoing_handlers[i] != (void*) packer) continue;
        write_buff[2] = (uint8_t)outgoing_handlers[i+1];
        break;
    }
    send_message(client, &write_buff, packed + 3);
}

static inline void send_error(Client* client, const char* request_id, const char* error_message, int error_code) {
    AiServer__Error error;
    ai_server__error__init(&error);
    error.code = error_code;
    error.messagestring = error_message;
    error.requestid = request_id;
    send_protobuf(client, ai_server__error__pack, &error);
    println(
        "Sent error to %s:%d: %s (%p)", 
        inet_ntoa(client->address.sin_addr), 
        ntohs(client->address.sin_port), 
        error_message,
        (void*)error_code
    );
}

static inline void send_message(Client* client, uint8_t* message, size_t len) {
    send(client->socket, message, len, 0);
}

void shutdown_server() {
    size_t iter = 0;
    Client* item;
    while((item = hashmap_iter(clients, &iter, (void*) &item)) != NULL) {
        pthread_kill(item->tid, 0);
        release_socket(item);
    }
    printf("Server closed\n");
}

static void* handlers[] = {(void*)0x2, handle_avail_prompts_req};
void on_read(Client* client, uint8_t* buff, size_t bytes_received) {
    size_t data_read = bytes_received;
    while(data_read > sizeof(uint16_t)) {
        size_t index = bytes_received-data_read;
        uint16_t message_len = (buff[index] << 8) | buff[index+1];
        if(message_len <= 0) {
            send_error(client, NULL, "Message len is zero or less. Check your network.", 0x3);
            data_read -= 2;
            continue;
        }
        if(index + message_len >= bytes_received) break; // we expect more data
        int8_t packet_id = buff[index+2];
        printf("Received packet: %p\n", (void*) packet_id);

        for(uint8_t i = 0; i < sizeof(handlers) / sizeof(void*); i += 2) {
            if(packet_id != (uint8_t) handlers[i]) continue;
            if(setjmp(client->last_execution_point) == 0) {
                client->has_execution_point = true;
                ((void (*)(Client*, uint8_t*, size_t)) handlers[i+1])(client, &buff[index+3], message_len - 1);
                client->has_execution_point = false;
            } else {
                send_error(client, NULL, "Packet triggered server assertion. (Packet id doesn't correspond to message?)", 0x3);
            }
            goto removed;
        }
        send_error(client, NULL, "Invalid packet", 0x1);
        removed: 
            data_read -= message_len + 2;
    }
}

int compare_clients(const void* a, const void* b, void* udata) {
    Client* ac = (Client*) a;
    Client* ab = (Client*) b;
    return ac->socket - ab->socket;
}