//
// Created by jyjia on 2016/7/12.
//

#ifndef RELIABLEFASTUDP_PACKAGECONNECTIONCLOSER_H
#define RELIABLEFASTUDP_PACKAGECONNECTIONCLOSER_H

#include "PackageBase.h"

typedef struct PackageCloseRFormat {

}PackageCloseRFormat;

class PackageConnectionCloseR : public PackageBase {
public:
    PackageCloseRFormat *close_r_format = (PackageCloseRFormat*)(bytes + sizeof(PackageBaseFormat));
    PackageConnectionCloseR();
    PackageConnectionCloseR(PackageBase *base);
};


#endif //RELIABLEFASTUDP_PACKAGECONNECTIONCLOSER_H
