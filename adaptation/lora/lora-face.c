/*
 * Copyright (C) 2019 Kangheng Wu, Kent
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v3.0. See the file LICENSE in the top level
 * directory for more details.
 */

#include <sys/ioctl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "lora-face.h"
#include "ndn-lite/ndn-error-code.h"
#include "ndn-lite/ndn-constants.h"

/*khwu*/
#include <wiringPi.h>
#include <wiringSerial.h>

static int
ndn_lora_face_up(struct ndn_face_intf* self);

static int
ndn_lora_face_down(struct ndn_face_intf* self);

static void
ndn_lora_face_destroy(ndn_face_intf_t* self);

static int
ndn_lora_face_send(ndn_face_intf_t* self, const uint8_t* packet, uint32_t size);

static ndn_lora_face_t*
ndn_lora_face_construct(
  const char *device,
  const int baud);

static void
ndn_lora_face_recv(void *self, size_t param_len, void *param);

/////////////////////////// /////////////////////////// ///////////////////////////

static int
ndn_lora_face_up(struct ndn_face_intf* self){
  ndn_lora_face_t* ptr = container_of(self, ndn_lora_face_t, intf);

  if(self->state == NDN_FACE_STATE_UP){
    return NDN_SUCCESS;
  }

  if(wiringPiSetup() < 0){
    return NDN_LORA_FACE_SOCKET_ERROR;
  }
  
  ptr->fd = serialOpen(ptr->device, ptr->baud);
  if(ptr->fd < 0){
    ndn_face_down(self);
    return NDN_LORA_FACE_SOCKET_ERROR;
  }
  
  ptr->process_event = ndn_msgqueue_post(ptr, ndn_lora_face_recv, 0, NULL);
  if(ptr->process_event == NULL){
    ndn_face_down(self);
    return NDN_FWD_MSGQUEUE_FULL;
  }
  
  self->state = NDN_FACE_STATE_UP;
  return NDN_SUCCESS;
}

static int
ndn_lora_face_down(struct ndn_face_intf* self){
  ndn_lora_face_t* ptr = (ndn_lora_face_t*)self;
  self->state = NDN_FACE_STATE_DOWN;

  if(ptr->fd != -1){
    serialClose(ptr->fd);
    ptr->fd  = -1;
  }

  if(ptr->process_event != NULL){
    ndn_msgqueue_cancel(ptr->process_event);
    ptr->process_event = NULL;
  }

  return NDN_SUCCESS;
}

static void
ndn_lora_face_destroy(ndn_face_intf_t* self){
  ndn_face_down(self);
  ndn_forwarder_unregister_face(self);
  free(self);
}

static int
ndn_lora_face_send(ndn_face_intf_t* self, const uint8_t* packet, uint32_t size){
  ndn_lora_face_t* ptr = (ndn_lora_face_t*)self;
  ssize_t ret;
  ret = write (ptr->fd, packet, size);
  //khwu
  if (size > 0) {
    printf("send packet size: %d\n", size);
  }
    
  if(ret != size){
    return NDN_LORA_FACE_SOCKET_ERROR;
  }else{
    return NDN_SUCCESS;
  } 
}

static ndn_lora_face_t*
ndn_lora_face_construct(
  const char *device,
  const int baud)
{
  ndn_lora_face_t* ret;
  int iret;

  ret = (ndn_lora_face_t*)malloc(sizeof(ndn_lora_face_t));
  if(!ret){
    return NULL;
  }

  ret->intf.face_id = NDN_INVALID_ID;
  iret = ndn_forwarder_register_face(&ret->intf);
  if(iret != NDN_SUCCESS){
    free(ret);
    return NULL;
  }

  ret->intf.type = NDN_FACE_TYPE_NET;
  ret->intf.state = NDN_FACE_STATE_DOWN;
  ret->intf.up = ndn_lora_face_up;
  ret->intf.down = ndn_lora_face_down;
  ret->intf.send = ndn_lora_face_send;
  ret->intf.destroy = ndn_lora_face_destroy;

  memset(ret->device, 0, sizeof(ret->device));
  strcpy(ret->device, device);
  ret->baud = baud;
  
  ret->fd = -1;
  ret->process_event = NULL;
  ndn_face_up(&ret->intf);

  return ret;
}

ndn_lora_face_t*
ndn_lora_multicast_face_construct(
  const char *device,
  const int baud)
{
  return ndn_lora_face_construct(device, baud);
}

//khwu
static ssize_t recvfrom_lora (ndn_lora_face_t* ptr) {
    int buffPos = 0;
    while (serialDataAvail(ptr->fd) > 0) {
        ptr->buf[buffPos] = serialGetchar(ptr->fd);
        buffPos++;
        // usleep(10000);
    }

    return buffPos;
}

static void
ndn_lora_face_recv(void *self, size_t param_len, void *param){
  ssize_t size;
  int ret;
  ndn_lora_face_t* ptr = (ndn_lora_face_t*)self;
  
  while(true){
    //size = recvfrom(ptr->sock, ptr->buf, sizeof(ptr->buf), 0,
    //                (struct sockaddr*)&client_addr, &addr_len);
    size = recvfrom_lora(ptr);
    if (size > 0) {
      printf("recv packet size: %d\n", size);
    }
    
    if(size > 0){
      // A packet recved
      ret = ndn_forwarder_receive(&ptr->intf, ptr->buf, size);
    }else if(size == 0){
      // No more packet
      //usleep(1000000);
      break;
    }else{
      ndn_face_down(&ptr->intf);
      return;
    }
  }  

  ptr->process_event = ndn_msgqueue_post(self, ndn_lora_face_recv, param_len, param);
}
