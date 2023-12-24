#include <coap3/coap.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>


uint8_t token = 0;
int updateTimeInterval = 1;
int getSensordata();
void* updateSensorData(void *params);
void observeRequest(coap_context_t *context, coap_session_t *session, const char* observe_uri, uint8_t* token);
static coap_response_t observeResponseHandle(coap_context_t *ctx, coap_session_t *session,
    coap_pdu_t *sent, coap_pdu_t *received, const coap_mid_t mid);

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
    const char* observe_uri = "coap://127.0.0.1:5683/example_data";
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

    observeRequest(ctx, session, observe_uri, &token);
    coap_register_response_handler(ctx, observeResponseHandle);

    // while (1) {
    //     coap_io_process(ctx, 0);
    // }

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

void observeRequest(coap_context_t *context, coap_session_t *session, const char* observe_uri, uint8_t* token) {
     coap_pdu_t *pdu;
     coap_uri_t uri;

    /* Create the pdu with the appropriate options */
    pdu = coap_pdu_init( COAP_MESSAGE_CON, COAP_REQUEST_GET, coap_new_message_id(session),
                        coap_session_max_pdu_size(session));
    if (!pdu)
        return NULL;

    /*
    * Create uniqueness token for this request for handling unsolicited /
    * delayed responses
    */
    token++;
    if (!coap_add_token(pdu, sizeof(token), (unsigned char*)&token)) {
        coap_log_debug("cannot add token to request\n");
        goto error;
    }

    coap_split_uri(observe_uri, strlen(observe_uri), &uri);

    coap_optlist_t **options = NULL;
    coap_insert_optlist(&options, coap_new_optlist(COAP_OPTION_OBSERVE, COAP_OBSERVE_ESTABLISH, NULL));
    coap_insert_optlist(&options, coap_new_optlist(COAP_OPTION_URI_PATH, uri.path.length, uri.path.s));

    /* Add in all the options (after internal sorting) to the pdu */
    if (!coap_add_optlist_pdu(pdu, options))
        goto error;

    coap_send(session, pdu);
    coap_io_process(context, 0);

    return COAP_RESPONSE_OK;

error:
    coap_delete_pdu(pdu);
    return NULL;
}

static coap_response_t observeResponseHandle(coap_context_t *ctx, coap_session_t *session,
coap_pdu_t *sent, coap_pdu_t *received, const coap_mid_t mid) {
    char* data;
    coap_bin_const_t rcv_token = coap_pdu_get_token(received);
    uint8_t rcv_code = coap_pdu_get_code(received);
    uint8_t rcv_type = coap_pdu_get_type(received);
    
    

    if (rcv_token != token) {
    /* drop if this was just some message, or send RST in case of notification */
        if (!sent && (rcv_type == COAP_MESSAGE_CON ||
                    rcv_type == COAP_MESSAGE_NON)) {
        /* Cause a CoAP RST to be sent */
            return COAP_RESPONSE_FAIL;
        }
        return COAP_RESPONSE_OK;
    }

    if (COAP_RESPONSE_CLASS(rcv_code) == 2) {
        coap_get_data(received, 2, &data);
        printf("Received data %s\n", data);
    }
}