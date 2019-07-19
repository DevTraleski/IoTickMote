#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"
#include "net/ipv6/multicast/uip-mcast6.h"
#include "resources/aes.h"


#define DEBUG DEBUG_PRINT
#include "net/ipv6/uip-debug.h"
#include "net/routing/routing.h"

//CoAP configuration

//#define SERVER_EP "coap://[fe80::202:2:2:2]"
#define SERVER_EP "coap://[ff03::fc]"
#define TOGGLE_INTERVAL 10


//Multicast configuration
/* Start sending messages START_DELAY secs after we start so that routing can
 * converge */
#define START_DELAY 100
#define MCAST_SINK_UDP_PORT 3001 /* Host byte order */
#define SEND_INTERVAL CLOCK_SECOND /* clock ticks */
#define ITERATIONS 100 /* messages */
#define MAX_PAYLOAD_LEN 120


extern coap_resource_t
  res_gateway;

static struct etimer et;

void
client_chunk_handler(coap_message_t *response)
{
  printf("Server chunk handler\n");
  const uint8_t *chunk;

  int len = coap_get_payload(response, &chunk);

  printf("|%.*s", len, (char *)chunk);
}

PROCESS(client, "CoAP Client");
PROCESS(server, "CoAP Server");
AUTOSTART_PROCESSES(&server, &client);

PROCESS_THREAD(server, ev, data)
{
  PROCESS_BEGIN();

  //LOG_INFO("Starting Gateway CoAP Server\n");
  coap_engine_init();
  coap_activate_resource(&res_gateway, "test/gateway");

  PRINTF("Multicast Engine Starting\n");
  NETSTACK_ROUTING.root_start();

  while(1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}

PROCESS_THREAD(client, ev, data)
{
  static coap_endpoint_t server_ep;
  PROCESS_BEGIN();

  static coap_message_t request[1];      /* This way the packet can be treated as pointer as usual. */

  coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &server_ep);

  etimer_set(&et, START_DELAY * CLOCK_SECOND);

  while(1) {
    PROCESS_YIELD();

    if(etimer_expired(&et)) {
      printf("--Send--\n");

      uint8_t key[] = "1234567890123456";
      uint8_t iv[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
      uint8_t msg[] = "RequestData55555";
      int len = sizeof(msg) - 1;
      struct AES_ctx ctx;
      AES_init_ctx_iv(&ctx, key, iv);
      AES_CBC_encrypt_buffer(&ctx, msg, len);
      printf("CBC encrypt: %s\n", msg);

      char msgHex[32];
      // Convert text to hex.
      for (int i = 0, j = 0; i < len ; ++i, j += 2) {
        sprintf(msgHex + j, "%02x", msg[i] & 0xff);
      }
      printf("Hexed: %s\n", msgHex);

      // prepare request, TID is set by COAP_BLOCKING_REQUEST()
      coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
      coap_set_header_uri_path(request, "test/hello");
      coap_set_payload(request, (uint8_t *)msgHex, len*2);

      COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);

      printf("\n--Done--\n");

      etimer_set(&et, TOGGLE_INTERVAL * CLOCK_SECOND);
    }

  }

  PROCESS_END();
}
