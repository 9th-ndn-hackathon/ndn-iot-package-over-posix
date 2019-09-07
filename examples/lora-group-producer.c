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

char device[NDN_LORA_DEVICE_SIZE];
int baud;
ndn_name_t name_prefix;
uint8_t buf[NDN_LORA_BUFFER_SIZE];
ndn_lora_face_t *face;
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

int
on_interest(const uint8_t* interest, uint32_t interest_size, void* userdata)
{
  printf('Received interest\n');
  ndn_interest_t interest_pkt;
  ndn_interest_from_block(&interest_pkt, interest, interest_size);
  ndn_data_t data;
  ndn_encoder_t encoder;
  char * str = "I'm a Data packet.";

  printf("On interest\n");
  data.name = interest_pkt.name;
  ndn_data_set_content(&data, (uint8_t*)str, strlen(str));
  ndn_metainfo_init(&data.metainfo);
  ndn_metainfo_set_content_type(&data.metainfo, NDN_CONTENT_TYPE_BLOB);
  encoder_init(&encoder, buf, 4096);
  ndn_data_tlv_encode_digest_sign(&encoder, &data);
  ndn_forwarder_put_data(encoder.output_value, encoder.offset);

  return NDN_FWD_STRATEGY_SUPPRESS;
}

int
main(int argc, char *argv[])
{
  int ret;
  if ((ret = parseArgs(argc, argv)) != 0) {
    return ret;
  }

  ndn_forwarder_init();
  ndn_security_init();
  face = ndn_lora_multicast_face_construct(device, baud);

  ndn_encoder_t encoder;
  encoder_init(&encoder, buf, sizeof(buf));
  ndn_name_tlv_encode(&encoder, &name_prefix);
  ndn_forwarder_register_prefix(encoder.output_value, encoder.offset, on_interest, NULL);

  running = true;
  while (running) {
    ndn_forwarder_process();
    usleep(10000);
  }
  ndn_face_destroy(&face->intf);
  return 0;
}
