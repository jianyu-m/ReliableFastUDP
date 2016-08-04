#include <cstring>
#include "PackageConnectionCreateR.h"

PackageConnectionCreateR::PackageConnectionCreateR(int port, int down_speed) {
    format->type = ConnectionCreateR;
    create_r_format->port = port;
    create_r_format->download_speed = down_speed;
    len = sizeof(PackageBaseFormat) + sizeof(PackageCreateRFormat);
}

PackageConnectionCreateR::PackageConnectionCreateR() {
    len = sizeof(PackageBaseFormat) + sizeof(PackageCreateRFormat);
}

PackageConnectionCreateR::PackageConnectionCreateR(PackageBase *base) {
    memcpy(bytes, base->get_buf(), base->length());
    len = base->length();
}