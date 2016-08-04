//
// Created by maxxie on 16-7-12.
//

#ifndef RELIABLEFASTUDP_PACKAGEACK_H
#define RELIABLEFASTUDP_PACKAGEACK_H


#include <cstdint>
#include "PackageBase.h"

typedef struct PackageAckFormat {
    uint64_t package_id;
    uint64_t una;
} PackageAckFormat;

class PackageAck : public PackageBase {
public:
    PackageAckFormat *ack_format = (PackageAckFormat*)(bytes + sizeof(PackageBaseFormat));
    PackageAck(uint64_t id, uint64_t una);
    PackageAck(PackageBase *base);
};


#endif //RELIABLEFASTUDP_PACKAGEACK_H
