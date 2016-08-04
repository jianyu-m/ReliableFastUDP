//
// Created by jyjia on 2016/7/12.
//

#include <cstring>
#include "PackagePing.h"
PackagePing::PackagePing() {
    format->type = Ping;
    len = sizeof(PackageBaseFormat) + sizeof(PackagePingFormat);
}

PackagePing::PackagePing(PackageBase *base) {
    memcpy(bytes, base->get_buf(), base->length());
    len = base->length();
}