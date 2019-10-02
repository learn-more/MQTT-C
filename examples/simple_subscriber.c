
/**
 * @file
 * A simple program that subscribes to a topic.
 */
#if !defined(WIN32)
#include <unistd.h>
#else
#define sleep(sec)		Sleep((sec) * 1000)
#define usleep(usec)	Sleep((usec) / 1000)
#endif

#include <stdlib.h>
#include <stdio.h>

#include <mqtt.h>

#if !defined(WIN32)
#include "templates/posix_sockets.h"
#include "templates/posix_threads.h"
#else
#include "templates/winapi_sockets.h"
#include "templates/winapi_threads.h"
#endif


/**
 * @brief The function will be called whenever a PUBLISH message is received.
 */
void publish_callback(void** unused, struct mqtt_response_publish *published);

/**
 * @brief The client's refresher. This function triggers back-end routines to 
 *        handle ingress/egress traffic to the broker.
 * 
 * @note All this function needs to do is call \ref __mqtt_recv and 
 *       \ref __mqtt_send every so often. I've picked 100 ms meaning that 
 *       client ingress/egress traffic will be handled every 100 ms.
 */
thread_return_type client_refresher(void* client);

/**
 * @brief Safelty closes the \p sockfd and cancels the \p client_daemon before \c exit. 
 */
void exit_example(int status, mqtt_pal_socket_handle sockfd, thread_handle_type *client_daemon);

int main(int argc, const char *argv[]) 
{
    const char* addr;
    const char* port;
    const char* topic;

    /* get address (argv[1] if present) */
    if (argc > 1) {
        addr = argv[1];
    } else {
        addr = "test.mosquitto.org";
    }

    /* get port number (argv[2] if present) */
    if (argc > 2) {
        port = argv[2];
    } else {
        port = "1883";
    }

    /* get the topic name to publish */
    if (argc > 3) {
        topic = argv[3];
    } else {
        topic = "datetime";
    }

    if (init_nb_socket()) {
        exit_example(EXIT_FAILURE, MQTT_INVALID_SOCKET_HANDLE, NULL);
    }

    /* open the non-blocking TCP socket (connecting to the broker) */
    mqtt_pal_socket_handle sockfd = open_nb_socket(addr, port);

    if (sockfd == MQTT_INVALID_SOCKET_HANDLE) {
        perror("Failed to open socket: ");
        exit_example(EXIT_FAILURE, sockfd, NULL);
    }

    /* setup a client */
    struct mqtt_client client;
    uint8_t sendbuf[2048]; /* sendbuf should be large enough to hold multiple whole mqtt messages */
    uint8_t recvbuf[1024]; /* recvbuf should be large enough any whole mqtt message expected to be received */
    mqtt_init(&client, sockfd, sendbuf, sizeof(sendbuf), recvbuf, sizeof(recvbuf), publish_callback);
    mqtt_connect(&client, "subscribing_client", NULL, NULL, 0, NULL, NULL, 0, 400);

    /* check that we don't have any errors */
    if (client.error != MQTT_OK) {
        fprintf(stderr, "error: %s\n", mqtt_error_str(client.error));
        exit_example(EXIT_FAILURE, sockfd, NULL);
    }

    /* start a thread to refresh the client (handle egress and ingree client traffic) */
    thread_handle_type client_daemon;
    if (create_thread(&client_daemon, client_refresher, &client)) {
        fprintf(stderr, "Failed to start client daemon.\n");
        exit_example(EXIT_FAILURE, sockfd, NULL);

    }

    /* subscribe */
    mqtt_subscribe(&client, topic, 0);

    /* start publishing the time */
    printf("%s listening for '%s' messages.\n", argv[0], topic);
    printf("Press CTRL-D to exit.\n\n");
    
    /* block */
    while(fgetc(stdin) != EOF); 
    
    /* disconnect */
    printf("\n%s disconnecting from %s\n", argv[0], addr);
    sleep(1);

    /* exit */ 
    exit_example(EXIT_SUCCESS, sockfd, &client_daemon);
}

void exit_example(int status, mqtt_pal_socket_handle sockfd, thread_handle_type *client_daemon)
{
    if (sockfd != MQTT_INVALID_SOCKET_HANDLE) close_nb_socket(sockfd);
    if (client_daemon != NULL) cancel_thread(*client_daemon);

    shutdown_nb_socket();
    exit(status);
}



void publish_callback(void** unused, struct mqtt_response_publish *published) 
{
    /* note that published->topic_name is NOT null-terminated (here we'll change it to a c-string) */
    char* topic_name = (char*) malloc(published->topic_name_size + 1);
    memcpy(topic_name, published->topic_name, published->topic_name_size);
    topic_name[published->topic_name_size] = '\0';

    printf("Received publish('%s'): %s\n", topic_name, (const char*) published->application_message);

    free(topic_name);
}

thread_return_type client_refresher(void* client)
{
    while(1) 
    {
        mqtt_sync((struct mqtt_client*) client);
        usleep(100000U);
    }
#if !defined(WIN32)
    return NULL;
#endif
}