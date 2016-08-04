//
// Created by jyjia on 2016/7/12.
//

#include "PackageConnectionClose.h"

PackageConnectionClose::PackageConnectionClose() {
    format->type = ConnectionClose;
    len = sizeof(PackageCloseFormat) + sizeof(PackageBaseFormat);
}