#include "geobuf.h"

void *geobuf_test(size_t* len) {
    void *buf;
    Data data = DATA__INIT;
    *len = data__get_packed_size(&data);
    buf = malloc(&len);
    data__pack(&data, buf);
    return buf;
}