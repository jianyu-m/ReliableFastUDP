//
// Created by jyjia on 2016/7/12.
//

#include <cstring>
#include "PackageData.h"


PackageData::PackageData(unsigned char *bytes, int size) {
    memcpy(this->bytes + sizeof(PackageBaseFormat) + sizeof(PackageDataFormat), bytes, size);
    format->type = Data;
    len = size + sizeof(PackageBaseFormat) + sizeof(PackageDataFormat);
}

char* PackageData::get_data() {
    return this->bytes + sizeof(PackageBaseFormat) + sizeof(PackageDataFormat);
}

PackageData::PackageData(PackageBase *base) {
    memcpy(bytes, base->get_buf(), base->length());
    len = base->length();
}