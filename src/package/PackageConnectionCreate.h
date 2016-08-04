//
// Created by jyjia on 2016/7/12.
//

#ifndef RELIABLEFASTUDP_PACKAGECONNECTIONCREATE_H
#define RELIABLEFASTUDP_PACKAGECONNECTIONCREATE_H

#include "PackageBase.h"
#include <cstdint>

typedef struct PackageCreateFormat {
    int download_speed;
    uint32_t ping_id;
}PackageCreateFormat;

class PackageConnectionCreate : public PackageBase {
public:
    PackageCreateFormat *create_format = (PackageCreateFormat*)(bytes + sizeof(PackageBaseFormat));
    PackageConnectionCreate(int download_speed);
    PackageConnectionCreate();
    PackageConnectionCreate(PackageBase *base);
};


#endif //RELIABLEFASTUDP_PACKAGECONNECTIONCREATE_H
