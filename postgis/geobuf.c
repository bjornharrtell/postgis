#include "geobuf.h"

void *geobuf_test(size_t* len) {
    void *buf;
    uint32_t lengths = 1;
    int64_t coord = 1;
    Data data = DATA__INIT;

    Data__Geometry geometry = DATA__GEOMETRY__INIT;
    geometry.type = DATA__GEOMETRY__TYPE__POINT;
    geometry.n_lengths = 1;
    geometry.lengths = &lengths;
    geometry.n_coords = 1;
    geometry.coords = &coord;
    data.geometry = &geometry;

    *len = data__get_packed_size(&data);
    buf = malloc(*len);
    data__pack(&data, buf);
    return buf;
}