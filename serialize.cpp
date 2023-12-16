#include "serialize.hpp"
#include "pdu.hpp"

unsigned char *serializeNetJoin(struct NET_JOIN_PDU *toSerialize) {

  unsigned char *serialized =
      (unsigned char *)malloc(sizeof(struct NET_JOIN_PDU));

  unsigned char *p = serialized;

  *p = toSerialize->type;
  p += 1;

  memcpy(p, &toSerialize->src_address, sizeof(uint32_t));
  p += 4;

  memcpy(p, &toSerialize->src_port, sizeof(uint16_t));
  p += 2;
  *p = toSerialize->name_length;
  p++;

  memcpy(p, &toSerialize->name, sizeof(uint32_t));

  return serialized;
}

struct NET_JOIN_PDU *deserializeNetJoin(unsigned char *toDeserialize) {

  struct NET_JOIN_PDU *deserialized =
      (struct NET_JOIN_PDU *)malloc(sizeof(struct NET_JOIN_PDU));
  unsigned char *p = toDeserialize;

  deserialized->type = *p;
  p += 1;

  memcpy(&deserialized->src_address, p, sizeof(uint32_t));
  p += 4;

  memcpy(&deserialized->src_port, p, sizeof(uint16_t));
  p += 2;

  deserialized->name_length = *p;
  p++;

  memcpy(&deserialized->name, p, sizeof(uint32_t));

  return deserialized;
}

unsigned char *serializeMsgSend(struct MSG_SEND_PDU *toSerialize) {

  unsigned char *serialized =
      (unsigned char *)malloc(sizeof(struct MSG_SEND_PDU));

  unsigned char *p = serialized;

  *p = toSerialize->type;
  p += 1;

  memcpy(p, &toSerialize->msg_length, sizeof(uint16_t));
  p += 2;

  memcpy(p, &toSerialize->msg, sizeof(uint32_t));

  return serialized;
}

struct MSG_SEND_PDU *deserializeMsgSend(unsigned char *toDeserialize) {

  struct MSG_SEND_PDU *deserialized =
      (struct MSG_SEND_PDU *)malloc(sizeof(struct MSG_SEND_PDU));
  unsigned char *p = toDeserialize;

  deserialized->type = *p;
  p += 1;

  memcpy(&deserialized->msg_length, p, sizeof(uint16_t));
  p += 2;

  memcpy(&deserialized->msg, p, sizeof(uint32_t));

  return deserialized;
}
