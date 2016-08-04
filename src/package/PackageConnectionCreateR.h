//
// Created by jyjia on 2016/7/12.
//

#ifndef RELIABLEFASTUDP_PACKAGECONNECTIONCREATE_R_H
#define RELIABLEFASTUDP_PACKAGECONNECTIONCREATE_R_H

#include "PackageBase.h"
typedef struct PackageCreateRFormat {
    int port;
    int download_speed;
} PackageCreateRFormat;

class PackageConnectionCreateR : public PackageBase {
public:
    PackageCreateRFormat *create_r_format = (PackageCreateRFormat*)(bytes + sizeof(PackageBaseFormat));
    PackageConnectionCreateR(int port, int down_speed);
    PackageConnectionCreateR();
    PackageConnectionCreateR(PackageBase *base);
};

#endif //RELIABLEFASTUDP_PACKAGECONNECTIONCREATE_R_H
