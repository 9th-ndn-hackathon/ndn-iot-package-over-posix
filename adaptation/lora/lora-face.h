/*
 * Copyright (C) 2019 Kangheng Wu, Kent
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v3.0. See the file LICENSE in the top level
 * directory for more details.
 */

#ifndef NDN_LORA_FACE_H_
#define NDN_LORA_FACE_H_

#include <netinet/in.h>
#include "ndn-lite/forwarder/forwarder.h"
#include "ndn-lite/util/msg-queue.h"
#include "../adapt-consts.h"

#ifdef __cplusplus
extern "C" {
#endif

// This face is different because we can create multiple faces safely

// Generally MTU < 2048
// Given that we don't cache
#define NDN_LORA_BUFFER_SIZE 4096

#define NDN_LORA_DEVICE_SIZE 64

/**
 * Lora face
 */
typedef struct ndn_lora_face {
  /**
   * The inherited interface.
   */
  ndn_face_intf_t intf;

  char device[NDN_LORA_DEVICE_SIZE];
  int baud;
  struct ndn_msg* process_event;
  int fd;
  uint8_t buf[NDN_LORA_BUFFER_SIZE];
} ndn_lora_face_t;

ndn_lora_face_t*
ndn_lora_multicast_face_construct(
  const char *device,
  const int baud);

#ifdef __cplusplus
}
#endif

#endif
