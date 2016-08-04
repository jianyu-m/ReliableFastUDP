//
// Created by jyjia on 2016/7/12.
//

#ifndef RELIABLEFASTUDP_PACKAGECONNECTIONCLOSE_H
#define RELIABLEFASTUDP_PACKAGECONNECTIONCLOSE_H


#include "PackageBase.h"

typedef struct PackageCloseFormat {

}PackageCloseFormat;

class PackageConnectionClose : public PackageBase{
public:
    PackageCloseFormat *close_format = (PackageCloseFormat*)(bytes + sizeof(PackageBaseFormat));
    PackageConnectionClose();
};


#endif //RELIABLEFASTUDP_PACKAGECONNECTIONCLOSE_H
