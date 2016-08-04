//
// Created by jyjia on 2016/7/12.
//

#ifndef RELIABLEFASTUDP_PACKAGEPING_H
#define RELIABLEFASTUDP_PACKAGEPING_H


#include "PackageBase.h"
typedef struct PackagePingFormat {
    /* to indicate if this ping should be sent back
     * if false, this message should be sent back*/
    bool ping_back = false;
}PackagePingFormat;

class PackagePing : public PackageBase {
public:
    PackagePingFormat *ping_format = (PackagePingFormat*)(bytes + sizeof(PackageBaseFormat));
    PackagePing();
    PackagePing(PackageBase *base);
};


#endif //RELIABLEFASTUDP_PACKAGEPING_H
