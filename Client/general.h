#include <coap3/coap.h>
#include <pthread.h>

#ifndef GENERAL_H
#define GENERAL_H

typedef struct updateSensorDataParams {
    coap_context_t *ctx;
    coap_session_t *session;
    coap_uri_t uri;
};

pthread_t* update_data_thread(updateSensorDataParams* params, char* (*operation)(void*), void* getDataParam);



#endif