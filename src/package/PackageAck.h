//
// Created by maxxie on 16-7-12.
//

#ifndef RELIABLEFASTUDP_PACKAGEACK_H
#define RELIABLEFASTUDP_PACKAGEACK_H


#include <cstdint>
#include "PackageBase.h"

//Every ack contains multiple ack for package
typedef struct PackageAckFormat {
    uint64_t ack_count;
    uint64_t una;
} PackageAckFormat;

class PackageAck : public PackageBase {
public:
    PackageAckFormat *ack_format = (PackageAckFormat*)(bytes + sizeof(PackageBaseFormat));
	uint64_t *ack_list = (uint64_t*)(bytes + sizeof(PackageBaseFormat) + sizeof(PackageAckFormat));
	PackageAck(uint64_t max_ack, uint64_t una, netstruct::AckQueue ack_list);
    PackageAck(PackageBase *base);
	PackageAck();
	void parse(uint64_t max_ack, uint64_t una, netstruct::AckQueue ack_list);
};


#endif //RELIABLEFASTUDP_PACKAGEACK_H
