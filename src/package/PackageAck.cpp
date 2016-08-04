//
// Created by maxxie on 16-7-12.
//

#include <cstring>
#include "PackageAck.h"

PackageAck::PackageAck(uint64_t id, uint64_t una) {
    ack_format->package_id = id;
    ack_format->una = una;
    format->type = Ack;
    len = sizeof(PackageBaseFormat) + sizeof(PackageAckFormat);
}

PackageAck::PackageAck(PackageBase *base) {
    memcpy(bytes, base->get_buf(), base->length());
    len = base->length();
}