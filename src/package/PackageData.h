//
// Created by jyjia on 2016/7/12.
//

#ifndef RELIABLEFASTUDP_RELIABLEPACKAGE_H
#define RELIABLEFASTUDP_RELIABLEPACKAGE_H

#include <cstdint>
#include "package.h"
#include "PackageBase.h"

typedef struct PackageDataFormat {
    uint64_t package_id;
    /* The biggest id of package received. */
    uint64_t una;
} PackageDataFormat;

class PackageData : public PackageBase {
public:
    PackageDataFormat *data_format = (PackageDataFormat*)(bytes + sizeof(PackageBaseFormat));
    int resend_count = 0;
    PackageData(unsigned char bytes[], int len);
    char* get_data();
    PackageData(PackageBase *base);
    int data_length() { return len - sizeof(PackageBaseFormat) - sizeof(PackageDataFormat); }
};


#endif //RELIABLEFASTUDP_RELIABLEPACKAGE_H
