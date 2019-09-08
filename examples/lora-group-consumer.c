/*
 * Copyright (C) 2019 Xinyu Ma, Zhiyi Zhang
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v3.0. See the file LICENSE in the top level
 * directory for more details.
 *
 * See AUTHORS.md for complete list of NDN IOT PKG authors and contributors.
 */
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
/*#include "../adaptation/udp/udp-face.h"*/
#include "../adaptation/lora/lora-face.h"

#include "ndn-lite/forwarder/forwarder.h"
#include "ndn-lite/encode/data.h"
#include "ndn-lite/encode/interest.h"
#include "ndn-lite/encode/encoder.h"

char device[NDN_LORA_DEVICE_SIZE];
int baud;
ndn_name_t name_prefix;
uint8_t buf[NDN_LORA_BUFFER_SIZE];
bool running;

int
parseArgs(int argc, char *argv[])
{
  char *sz_device, *sz_baud;

  if (argc < 2) {
    fprintf(stderr, "ERROR: wrong arguments.\n");
    printf("Usage: <name-prefix> <device>=/dev/ttyS0 <baud>=2400\n");
    return 1;
  }
  if (ndn_name_from_string(&name_prefix, argv[1], strlen(argv[1])) != NDN_SUCCESS) {
    fprintf(stderr, "ERROR: wrong name.\n");
    return 4;
  }
  
  strcpy(device, "/dev/ttyS0");
  baud = 2400;
  
  if (argc >= 3) {
    sz_device = argv[2];
    if (strlen(sz_device) <= 0) {
      fprintf(stderr, "ERROR: wrong arguments.\n");
      return 1;
    }      
    strcpy(device, sz_device);
  }
  
  if (argc >= 4) {
    sz_baud = argv[3];
    if (strlen(sz_baud) <= 0) {
      fprintf(stderr, "ERROR: wrong arguments.\n");
      return 1;
    }
    baud = strtol(sz_baud, NULL, 10);
  }
  return 0;
}

void
on_data(const uint8_t* rawdata, uint32_t data_size, void* userdata)
{
  for (int i = 0; i < data_size; ++i)
    printf("%02x ", rawdata[i]);
  printf("\n");

  ndn_data_t data;
  printf("On data\n");
  if (ndn_data_tlv_decode_digest_verify(&data, rawdata, data_size)) {
    printf("Decoding failed.\n");
    return;
  }
  printf("It says: %s\n", data.content_value);
}

void
on_timeout(void* userdata)
{
  printf("On timeout\n");
  running = false;
}

int
main(int argc, char *argv[])
{
  ndn_lora_face_t *face;
  ndn_interest_t interest;
  ndn_encoder_t encoder;
  int ret;

  if((ret = parseArgs(argc, argv)) != 0){
    return ret;
  }

  ndn_forwarder_init();
  ndn_security_init();
  face = ndn_lora_multicast_face_construct(device, baud);

  encoder_init(&encoder, buf, NDN_LORA_BUFFER_SIZE);
  ndn_name_tlv_encode(&encoder, &name_prefix);
  ndn_forwarder_add_route(&face->intf, buf, encoder.offset);

  ndn_interest_from_name(&interest, &name_prefix);
  encoder_init(&encoder, buf, NDN_LORA_BUFFER_SIZE);
  ndn_interest_tlv_encode(&encoder, &interest);
  
  ndn_forwarder_express_interest(encoder.output_value, encoder.offset, on_data, on_timeout, NULL);

  running = true;
  while (running) {
    ndn_forwarder_process();
    usleep(10000);
  }
  ndn_face_destroy(&face->intf);
  return 0;
}
