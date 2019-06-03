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


#define DEBUG DEBUG_PRINT
#include "net/ipv6/uip-debug.h"
#include "net/routing/routing.h"

//CoAP configuration

//#define SERVER_EP "coap://[fe80::201:1:1:1]"
#define SERVER_EP "coap://[ff03::fc]"
#define TOGGLE_INTERVAL 10


//Multicast configuration
/* Start sending messages START_DELAY secs after we start so that routing can
 * converge */
#define START_DELAY 60
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

      /* prepare request, TID is set by COAP_BLOCKING_REQUEST() */
      coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
      coap_set_header_uri_path(request, "test/hello");

      const char msg[] = "Oh hi mark!";

      coap_set_payload(request, (uint8_t *)msg, sizeof(msg) - 1);

      //LOG_INFO_COAP_EP(&server_ep);
      //LOG_INFO_("\n");

      COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);

      printf("\n--Done--\n");

      etimer_set(&et, TOGGLE_INTERVAL * CLOCK_SECOND);
    }
  }

  PROCESS_END();
}
