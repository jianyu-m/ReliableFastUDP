//
// Created by jyjia on 2016/7/12.
//

#ifndef RELIABLEFASTUDP_PACKAGE_H
#define RELIABLEFASTUDP_PACKAGE_H
typedef enum PackageType {
    ConnectionCreate,
    ConnectionCreateR,
    ConnectionClose,
    ConnectionCloseR,
    Ping,
    Data,
    Ack
} PackageType;
#endif //RELIABLEFASTUDP_PACKAGE_H
