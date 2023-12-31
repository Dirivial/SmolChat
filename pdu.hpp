#include <inttypes.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>

#ifndef PDU_DEF
#define PDU_DEF

enum : uint8_t {
  NET_ALIVE = 0,
  NET_JOIN = 1,
  NET_JOIN_RESPONSE = 2,
  NET_MSG_SEND = 3,
  NET_MSG_RECV = 4,
  NET_MSG_BROADCAST = 5,
  NET_LEAVE = 6,
};

struct NET_ALIVE_PDU {
  uint8_t type;
};

struct NET_JOIN_PDU {
  uint8_t type;
  uint8_t name_length;
  uint8_t *name;
};

struct NET_JOIN_RESPONSE_PDU {
  uint8_t type;
};

struct MSG_SEND_PDU {
  uint8_t type;
  uint16_t msg_length;
  uint8_t *msg;
};

struct MSG_RECV_PDU {
  uint8_t type;
  uint16_t time_length;
  uint8_t *time;
};

struct MSG_BROADCAST_PDU {
  uint8_t type;
  uint16_t msg_length;
  uint8_t *msg;
  uint8_t name_length;
  uint8_t *name;
  uint8_t time_length;
  uint8_t *time;
};
#endif
