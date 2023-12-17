#include <coap3/coap.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>


int token = 0;
int updateTimeInterval = 1;
int getSensordata();
void* updateSensorData(void *params);

struct updateSensorDataParams {
    coap_context_t *ctx;
    coap_session_t *session;
    coap_uri_t uri;
};
int main(void) {
    coap_context_t *ctx = NULL;
    coap_session_t *session = NULL;
    coap_pdu_t *request = NULL;
    coap_address_t server;
    coap_uri_t uri;
    struct in_addr ip_address = { htonl(0x7f000001) };
    const char *server_uri = "coap://127.0.0.1:5683/example_data"; 
    pthread_t sensorDataUpdateThreadID;

    // Seed for random number generation
    srand(time(NULL));

    // Initialize libcoap context
    coap_startup();
    ctx = coap_new_context(NULL);

    // Parse the server URI
    if (coap_split_uri(server_uri, strlen(server_uri), &uri) == -1) {
        fprintf(stderr, "Invalid URI\n");
        return -1;
    }

    coap_address_init(&server);
    server.addr.sa.sa_family = AF_INET;
    server.addr.sin.sin_addr = ip_address;
    server.addr.sin.sin_port = htons (5683);

    // Create CoAP session
    session = coap_new_client_session(ctx, NULL, &server, COAP_PROTO_UDP);

    if (!session) {
        fprintf(stderr, "Failed to create session\n");
        return -1;
    }

    struct updateSensorDataParams params = {
        .ctx = ctx,
        .session = session,
        .uri = uri
    };

    if (pthread_create(&sensorDataUpdateThreadID, NULL, updateSensorData, &params) != 0) {
        fprintf(stderr, "Failed to create sensor data update thread\n");
        return -1;
    }

    // char sensorData[100];
    // while (1) {
    //     memset(sensorData, 0, 100);
    //     sprintf(sensorData, "%d", getSensordata());
    //     // Create a CoAP PUT request
        // request = coap_pdu_init(COAP_MESSAGE_CON, COAP_REQUEST_PUT,
        // coap_new_message_id(session), coap_session_max_pdu_size(session));
        // coap_add_option(request, COAP_OPTION_URI_PATH, uri.path.length, uri.path.s);


    //     // printf("Random value: %d\n", random_value);
    //     coap_add_data(request, strlen(sensorData), (const uint8_t *)&sensorData);

    //     // Send the PUT request
    //     coap_send(session, request);
    //     // Wait for the response
    //     coap_io_process(ctx, 1000);
    //     sleep(1);
    // }
    sleep(20);

    // Clean up
    pthread_cancel(sensorDataUpdateThreadID);
    coap_delete_pdu(request);
    coap_session_release(session);
    coap_free_context(ctx);
    coap_cleanup();

    return 0;
}

int getSensordata() {
    return rand() % 100;
}

void* updateSensorData(void *params) {
    char sensorData[100];
    coap_context_t *ctx = ((struct updateSensorDataParams *)params)->ctx;
    coap_session_t *session = ((struct updateSensorDataParams *)params)->session;
    coap_pdu_t *request = NULL;
    coap_uri_t uri = ((struct updateSensorDataParams *)params)->uri;

    
    while (1) {
        memset(sensorData, 0, 100);
        sprintf(sensorData, "%d", getSensordata());

        request = coap_pdu_init(COAP_MESSAGE_CON, COAP_REQUEST_PUT,
        coap_new_message_id(session), coap_session_max_pdu_size(session));
        coap_add_option(request, COAP_OPTION_URI_PATH, uri.path.length, uri.path.s);

        coap_add_data(request, strlen(sensorData), (const uint8_t *)&sensorData);
        coap_send(session, request);
        coap_io_process(ctx, 0);
        sleep(updateTimeInterval);
    }
}