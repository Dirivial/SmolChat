#include "serialize.hpp"
#include "pdu.hpp"

unsigned char *serializeNetJoin(struct NET_JOIN_PDU *pdu) {
  uint8_t *buffer =
      (uint8_t *)malloc(1 + sizeof(pdu->name_length) + pdu->name_length);
  buffer[0] = pdu->type;

  // Copy msg_length as a 16-bit value to the buffer (little-endian)
  std::memcpy(buffer + 1, &pdu->name_length, sizeof(pdu->name_length));

  // Copy msg to the buffer
  std::memcpy(buffer + 1 + sizeof(pdu->name_length), pdu->name,
              pdu->name_length);
  return buffer;
}

struct NET_JOIN_PDU *deserializeNetJoin(unsigned char *buffer) {

  struct NET_JOIN_PDU *pdu =
      (struct NET_JOIN_PDU *)malloc(sizeof(struct NET_JOIN_PDU));
  pdu->type = buffer[0];

  // Extract msg_length from the buffer (little-endian)
  std::memcpy(&pdu->name_length, buffer + 1, sizeof(pdu->name_length));

  // Allocate memory for msg and copy it from the buffer
  pdu->name = (uint8_t *)std::malloc(sizeof(uint8_t) * pdu->name_length);
  std::memcpy(pdu->name, buffer + 1 + sizeof(pdu->name_length),
              pdu->name_length);

  return pdu;
}

uint8_t *serializeMsgSend(struct MSG_SEND_PDU *pdu) {

  uint8_t *buffer =
      (uint8_t *)malloc(1 + sizeof(pdu->msg_length) + pdu->msg_length);
  buffer[0] = pdu->type;

  // Copy msg_length as a 16-bit value to the buffer (little-endian)
  std::memcpy(buffer + 1, &pdu->msg_length, sizeof(pdu->msg_length));

  // Copy msg to the buffer
  std::memcpy(buffer + 1 + sizeof(pdu->msg_length), pdu->msg, pdu->msg_length);
  return buffer;
}

MSG_SEND_PDU *deserializeMsgSend(uint8_t *buffer) {
  MSG_SEND_PDU *pdu =
      (struct MSG_SEND_PDU *)malloc(sizeof(struct MSG_SEND_PDU));
  pdu->type = buffer[0];

  // Extract msg_length from the buffer (little-endian)
  std::memcpy(&pdu->msg_length, buffer + 1, sizeof(pdu->msg_length));

  // Allocate memory for msg and copy it from the buffer
  pdu->msg = (uint8_t *)std::malloc(sizeof(uint8_t) * pdu->msg_length);
  std::memcpy(pdu->msg, buffer + 1 + sizeof(pdu->msg_length), pdu->msg_length);

  return pdu;
}
uint8_t *serializeMsgBroadcast(struct MSG_BROADCAST_PDU *pdu) {

  uint8_t *buffer = (uint8_t *)malloc(
      1 + sizeof(pdu->msg_length) + pdu->msg_length + sizeof(pdu->name_length) +
      pdu->name_length + sizeof(pdu->time_length) + pdu->time_length);
  buffer[0] = pdu->type;
  int index = 1;

  // Copy msg_length as a 16-bit value to the buffer (little-endian)
  std::memcpy(buffer + index, &pdu->msg_length, sizeof(pdu->msg_length));
  index += sizeof(pdu->msg_length);

  // Copy msg to the buffer
  std::memcpy(buffer + index, pdu->msg, pdu->msg_length);
  index += pdu->msg_length;

  // Copy name_length as a 16-bit value to the buffer (little-endian)
  std::memcpy(buffer + index, &pdu->name_length, sizeof(pdu->name_length));
  index += sizeof(pdu->name_length);

  // Copy name to the buffer
  std::memcpy(buffer + index, pdu->name, pdu->name_length);
  index += pdu->name_length;

  // Copy name_length as a 16-bit value to the buffer (little-endian)
  std::memcpy(buffer + index, &pdu->time_length, sizeof(pdu->time_length));
  index += sizeof(pdu->time_length);

  // Copy name to the buffer
  std::memcpy(buffer + index, pdu->time, pdu->time_length);

  return buffer;
}

MSG_BROADCAST_PDU *deserializeMsgBroadcast(uint8_t *buffer) {
  MSG_BROADCAST_PDU *pdu =
      (struct MSG_BROADCAST_PDU *)malloc(sizeof(struct MSG_BROADCAST_PDU));
  pdu->type = buffer[0];
  int index = 1;

  // Extract msg_length from the buffer (little-endian)
  std::memcpy(&pdu->msg_length, buffer + index, sizeof(pdu->msg_length));
  index += sizeof(pdu->msg_length);

  // Allocate memory for msg and copy it from the buffer
  pdu->msg = (uint8_t *)std::malloc(sizeof(uint8_t) * pdu->msg_length);
  std::memcpy(pdu->msg, buffer + index, pdu->msg_length);
  index += pdu->msg_length;

  // Extract name_length from the buffer (little-endian)
  std::memcpy(&pdu->name_length, buffer + index, sizeof(pdu->name_length));
  index += sizeof(pdu->name_length);

  // Allocate memory for name and copy it from the buffer
  pdu->name = (uint8_t *)std::malloc(sizeof(uint8_t) * pdu->name_length);
  std::memcpy(pdu->name, buffer + index, pdu->name_length);
  index += pdu->name_length;

  // Extract time_length from the buffer (little-endian)
  std::memcpy(&pdu->time_length, buffer + index, sizeof(pdu->time_length));
  index += sizeof(pdu->time_length);

  // Allocate memory for time and copy it from the buffer
  pdu->time = (uint8_t *)std::malloc(sizeof(uint8_t) * pdu->time_length);
  std::memcpy(pdu->time, buffer + index, pdu->time_length);

  return pdu;
}
