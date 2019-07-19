/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *      Example resource
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "sys/cc.h"
#include "coap.h"
#include "coap-engine.h"
#include "coap-transactions.h"
#include "aes.h"

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "coap"
#define LOG_LEVEL  LOG_LEVEL_COAP

#define SERVER_EP "coap://[fe80::201:1:1:1]"

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

/*
 * A handler function named [resource name]_handler must be implemented for each RESOURCE.
 * A buffer for the response payload is provided through the buffer pointer. Simple resources can ignore
 * preferred_size and offset, but must respect the REST_MAX_CHUNK_SIZE limit for the buffer.
 * If a smaller block size is requested for CoAP, the REST framework automatically splits the data.
 */
RESOURCE(res_hello,
         "title=\"Hello world: ?len=0..\";rt=\"Text\"",
         res_get_handler,
         NULL,
         NULL,
         NULL);

static void
res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  printf("Entered resource\n");
  const uint8_t *content = NULL;
  int len  = coap_get_payload(request, &content);
  //printf("%" PRIu8 "\n", content);
  //printf("Value: %d\n", content);
  printf("Length: %d\n", len);
  printf("Value as payload: %s\n", content);

  // Convert the hex back to a string.
  char *hex = (char *)content;
  char msg[len / 2];

  for (int i = 0, j = 0; j < len; ++i, j += 2) {
    int val[1];
    sscanf(hex + j, "%2x", val);
    msg[i] = val[0];
    msg[i + 1] = '\0';
  }
  printf("Unhexed: %s\n", msg);

  uint8_t key[] = "1234567890123456";
  uint8_t iv[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
  struct AES_ctx ctx;
  AES_init_ctx_iv(&ctx, key, iv);
  AES_CBC_decrypt_buffer(&ctx, (uint8_t*)msg, sizeof(msg) - 1);
  printf("CBC decrypt: %s\n", msg);

  if( strcmp(msg, "RequestData55555") == 0)
  {

    //Request to server
    static coap_endpoint_t server_ep;
    coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &server_ep);

    uint16_t mid = coap_get_mid();
    coap_transaction_t *trans = coap_new_transaction(mid, &server_ep);

    static coap_message_t serverResponse[1];

    //Encrypt on master key
    uint8_t key[] = "6543210987654321";
    uint8_t iv[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
    uint8_t msg[] = "measure:15460000";
    int len = sizeof(msg) - 1;
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_encrypt_buffer(&ctx, msg, len);
    printf("CBC encrypt: %s\n", msg);
    printf("Size of msg: %d\n", sizeof(msg) - 1);

    //Encrypt on gateway PSK
    uint8_t gkey[] = "1234567890123456";
    uint8_t giv[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
    struct AES_ctx gctx;
    AES_init_ctx_iv(&gctx, gkey, giv);
    AES_CBC_encrypt_buffer(&gctx, msg, len);
    printf("CBC encrypt: %s\n", msg);
    printf("Size of msg: %d\n", sizeof(msg) - 1);

    char ghex[len*2];
    // Convert text to hex.
    for (int i = 0, j = 0; i < len ; ++i, j += 2) {
      sprintf(ghex + j, "%02x", msg[i] & 0xff);
    }
    printf("Gateway Hexed: %s\n", ghex);


    coap_init_message(serverResponse, COAP_TYPE_CON, COAP_GET, mid);
    coap_set_header_uri_path(serverResponse, "test/gateway");
    coap_set_payload(serverResponse, ghex, len*2);

    trans->message_len = coap_serialize_message(serverResponse, trans->message);
    coap_send_transaction(trans);

  }

  //Response
  char const *const message = "OK";
  int length = 2;

  memcpy(buffer, message, length);

  coap_set_header_content_format(response, TEXT_PLAIN);
  coap_set_header_etag(response, (uint8_t *)&length, 1);
  coap_set_payload(response, buffer, length);
  printf("Sending response\n");
}
