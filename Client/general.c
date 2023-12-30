#include <coap3/coap.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>


size_t tracked_token_count = 0  ;
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

// static coap_response_t observeResponseHandle(coap_context_t *ctx, coap_session_t *session,
// coap_pdu_t *sent, coap_pdu_t *received, const coap_mid_t mid) {
//     char* data;
//     coap_bin_const_t rcv_token = coap_pdu_get_token(received);
//     uint8_t rcv_code = coap_pdu_get_code(received);
//     uint8_t rcv_type = coap_pdu_get_type(received);
    
    

//     if (rcv_token != token) {
//     /* drop if this was just some message, or send RST in case of notification */
//         if (!sent && (rcv_type == COAP_MESSAGE_CON ||
//                     rcv_type == COAP_MESSAGE_NON)) {
//         /* Cause a CoAP RST to be sent */
//             return COAP_RESPONSE_FAIL;
//         }
//         return COAP_RESPONSE_OK;
//     }

//     if (COAP_RESPONSE_CLASS(rcv_code) == 2) {
//         coap_get_data(received, 2, &data);
//         printf("Received data %s\n", data);
//     }
// }

// static void
// track_new_token(size_t tokenlen, uint8_t *token) {
//   track_token *new_list = realloc(tracked_tokens,
//                                   (tracked_tokens_count + 1) * sizeof(tracked_tokens[0]));
//   if (!new_list) {
//     coap_log_info("Unable to track new token\n");
//     return;
//   }
//   tracked_tokens = new_list;
//   tracked_tokens[tracked_tokens_count].token = coap_new_binary(tokenlen);
//   if (!tracked_tokens[tracked_tokens_count].token)
//     return;
//   memcpy(tracked_tokens[tracked_tokens_count].token->s, token, tokenlen);
//   tracked_tokens[tracked_tokens_count].observe = doing_observe;
//   tracked_tokens_count++;
// }

// static int
// track_check_token(coap_bin_const_t *token) {
//   size_t i;

//   for (i = 0; i < tracked_tokens_count; i++) {
//     if (coap_binary_equal(token, tracked_tokens[i].token)) {
//       return 1;
//     }
//   }
//   return 0;
// }

// static void
// track_flush_token(coap_bin_const_t *token, int force) {
//   size_t i;

//   for (i = 0; i < tracked_tokens_count; i++) {
//     if (coap_binary_equal(token, tracked_tokens[i].token)) {
//       if (force || !tracked_tokens[i].observe || !obs_started) {
//         /* Only remove if not Observing */
//         coap_delete_binary(tracked_tokens[i].token);
//         if (tracked_tokens_count-i > 1) {
//           memmove(&tracked_tokens[i],
//                   &tracked_tokens[i+1],
//                   (tracked_tokens_count-i-1) * sizeof(tracked_tokens[0]));
//         }
//         tracked_tokens_count--;
//       }
//       break;
//     }
//   }
// }

static coap_response_t
message_handler(coap_session_t *session COAP_UNUSED,
                const coap_pdu_t *sent,
                const coap_pdu_t *received,
                const coap_mid_t id COAP_UNUSED) {

  coap_opt_t *block_opt;
  coap_opt_iterator_t opt_iter;
  size_t len;
  const uint8_t *databuf;
  size_t offset;
  size_t total;
  coap_pdu_code_t rcv_code = coap_pdu_get_code(received);
  coap_pdu_type_t rcv_type = coap_pdu_get_type(received);
  coap_bin_const_t token = coap_pdu_get_token(received);

  coap_log_debug("** process incoming %d.%02d response:\n",
                 COAP_RESPONSE_CLASS(rcv_code), rcv_code & 0x1F);
  if (coap_get_log_level() < COAP_LOG_DEBUG)
    coap_show_pdu(COAP_LOG_INFO, received);

  /* check if this is a response to our original request */
  if (!track_check_token(&token)) {
    /* drop if this was just some message, or send RST in case of notification */
    if (!sent && (rcv_type == COAP_MESSAGE_CON ||
                  rcv_type == COAP_MESSAGE_NON)) {
      /* Cause a CoAP RST to be sent */
      return COAP_RESPONSE_FAIL;
    }
    return COAP_RESPONSE_OK;
  }

  if (rcv_type == COAP_MESSAGE_RST) {
    coap_log_info("got RST\n");
    return COAP_RESPONSE_OK;
  }

  /* output the received data, if any */
  if (COAP_RESPONSE_CLASS(rcv_code) == 2) {

    /* set obs timer if we have successfully subscribed a resource */
    if (doing_observe && !obs_started &&
        coap_check_option(received, COAP_OPTION_OBSERVE, &opt_iter)) {
      coap_log_debug("observation relationship established, set timeout to %d\n",
                     obs_seconds);
      obs_started = 1;
      obs_ms = obs_seconds * 1000;
      obs_ms_reset = 1;
    }

    if (coap_get_data_large(received, &len, &databuf, &offset, &total)) {
      append_to_output(databuf, len);
      if ((len + offset == total) && add_nl)
        append_to_output((const uint8_t *)"\n", 1);
    }

    /* Check if Block2 option is set */
    block_opt = coap_check_option(received, COAP_OPTION_BLOCK2, &opt_iter);
    if (!single_block_requested && block_opt) { /* handle Block2 */

      /* TODO: check if we are looking at the correct block number */
      if (coap_opt_block_num(block_opt) == 0) {
        /* See if observe is set in first response */
        ready = doing_observe ? coap_check_option(received,
                                                  COAP_OPTION_OBSERVE, &opt_iter) == NULL : 1;
      }
      if (COAP_OPT_BLOCK_MORE(block_opt)) {
        doing_getting_block = 1;
      } else {
        doing_getting_block = 0;
        if (!is_mcast)
          track_flush_token(&token, 0);
      }
      return COAP_RESPONSE_OK;
    }
  } else {      /* no 2.05 */
    /* check if an error was signaled and output payload if so */
    if (COAP_RESPONSE_CLASS(rcv_code) >= 4) {
      fprintf(stderr, "%d.%02d", COAP_RESPONSE_CLASS(rcv_code),
              rcv_code & 0x1F);
      if (coap_get_data_large(received, &len, &databuf, &offset, &total)) {
        fprintf(stderr, " ");
        while (len--) {
          fprintf(stderr, "%c", isprint(*databuf) ? *databuf : '.');
          databuf++;
        }
      }
      fprintf(stderr, "\n");
      track_flush_token(&token, 1);
    }

  }
  if (!is_mcast)
    track_flush_token(&token, 0);

  /* our job is done, we can exit at any time */
  ready = doing_observe ? coap_check_option(received,
                                            COAP_OPTION_OBSERVE, &opt_iter) == NULL : 1;
  return COAP_RESPONSE_OK;
}

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

    while (1) {
        coap_io_process(ctx, 0);
    }

    // Clean up
    pthread_cancel(sensorDataUpdateThreadID);
    coap_delete_pdu(request);
    coap_session_release(session);
    coap_free_context(ctx);
    coap_cleanup();

    return 0;
}