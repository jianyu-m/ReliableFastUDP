//
// Created by maxxie on 16-7-12.
//

#include <cstring>
#include "PackageAck.h"

PackageAck::PackageAck(uint64_t max_ack, uint64_t una, netstruct::AckQueue acklist) {
    ack_format->ack_count = min(max_ack, acklist.size());
    ack_format->una = una;
    format->type = Ack;
    len = sizeof(PackageBaseFormat) + sizeof(PackageAckFormat) + sizeof(uint64_t) * ack_format->ack_count;
	for (int i = 0; i < ack_format->ack_count; i++) {
		ack_list[i] = acklist.pop();
	}
}

PackageAck::PackageAck() {
	ack_format->ack_count = 0;
	format->type = Ack;
	len = sizeof(PackageBaseFormat) + sizeof(PackageAckFormat);
}

PackageAck::PackageAck(PackageBase *base) {
    memcpy(bytes, base->get_buf(), base->length());
    len = base->length();
}

void PackageAck::parse(uint64_t max_ack, uint64_t una, netstruct::AckQueue acklist) {
	ack_format->ack_count = min(max_ack, acklist.size());
	ack_format->una = una;
	len = sizeof(PackageBaseFormat) + sizeof(PackageAckFormat) + sizeof(uint64_t) * ack_format->ack_count;
	for (int i = 0; i < ack_format->ack_count; i++) {
		ack_list[i] = acklist.pop();
	}
}