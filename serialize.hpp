#include "pdu.hpp"

unsigned char *serializeMsgSend(struct MSG_SEND_PDU *toSerialize);
struct MSG_SEND_PDU *deserializeMsgSend(unsigned char *toDeserialize);

unsigned char *serializeNetJoin(struct NET_JOIN_PDU *toSerialize);
struct NET_JOIN_PDU *deserializeNetJoin(unsigned char *toDeserialize);
