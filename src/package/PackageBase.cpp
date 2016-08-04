//
// Created by jyjia on 2016/7/12.
//

#include <cstring>
#include "PackageBase.h"
PackageBase::PackageBase() {
    len = sizeof(PackageBaseFormat);
}

PackageBase::PackageBase(unsigned char *bytes, int len) {
    memcpy(this->bytes + sizeof(PackageBaseFormat), bytes, len);
    this->len = len + sizeof(PackageBaseFormat);
}

void PackageBase::copy_from_base(PackageBase *base) {
    memcpy(this->bytes, base->get_buf(), base->length());
    len = base->length();
}