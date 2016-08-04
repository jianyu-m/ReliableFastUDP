//
// Created by jyjia on 2016/7/12.
//

#include <cstring>
#include "PackageConnectionCloseR.h"

PackageConnectionCloseR::PackageConnectionCloseR() {
    format->type = ConnectionCloseR;
    len = sizeof(PackageBaseFormat) + sizeof(PackageCloseRFormat);
}

PackageConnectionCloseR::PackageConnectionCloseR(PackageBase *base) {
    memcpy(bytes, base->get_buf(), base->length());
    len = base->length();
}