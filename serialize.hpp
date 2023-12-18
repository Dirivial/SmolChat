#include "pdu.hpp"

uint8_t *serializeMsgSend(struct MSG_SEND_PDU *toSerialize);
struct MSG_SEND_PDU *deserializeMsgSend(uint8_t *toDeserialize);

uint8_t *serializeMsgBroadcast(struct MSG_BROADCAST_PDU *toSerialize);
struct MSG_BROADCAST_PDU *deserializeMsgBroadcast(uint8_t *toDeserialize);

unsigned char *serializeNetJoin(struct NET_JOIN_PDU *toSerialize);
struct NET_JOIN_PDU *deserializeNetJoin(unsigned char *toDeserialize);
