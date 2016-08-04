//
// Created by jyjia on 2016/7/12.
//

#ifndef RELIABLEFASTUDP_PACKAGEBASE_H
#define RELIABLEFASTUDP_PACKAGEBASE_H


#include "../ReliableFastUDP.h"
#include "package.h"

typedef struct PackageBaseFormat {
    PackageType type;
}PackageBaseFormat;

class PackageBase {
protected:
    int len;
    char bytes[MAX_MTU];
    PackageBaseFormat *format = (PackageBaseFormat*)bytes;
public:
    PackageBase();
    PackageBase(unsigned char bytes[], int len);
    char *get_buf() {return bytes;}
    int length() {return len;}
    void set_len(int size) { len = size;}
    ~PackageBase() {}
    PackageType get_type() { return format->type; }
    void copy_from_base(PackageBase *base);
};


#endif //RELIABLEFASTUDP_PACKAGEBASE_H
