//
// Created by jyjia on 2016/7/12.
//

#include <cstring>
#include "PackageConnectionCreate.h"

PackageConnectionCreate::PackageConnectionCreate(int download_speed) {
    format->type = ConnectionCreate;
    create_format->download_speed = download_speed;
//    gettimeofday(&create_format->current_time, 0);
    len = sizeof(PackageBaseFormat) + sizeof(PackageCreateFormat);
}

PackageConnectionCreate::PackageConnectionCreate() {
    len = sizeof(PackageBaseFormat) + sizeof(PackageCreateFormat);
}

PackageConnectionCreate::PackageConnectionCreate(PackageBase *base) {
    memcpy(bytes, base->get_buf(), base->length());
    len = base->length();
}