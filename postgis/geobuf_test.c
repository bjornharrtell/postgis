#include <stdio.h>
#include <stdlib.h>
#include "geobuf.pb-c.h"

int main ()
{
    void *buf;
    size_t len;
    uint32_t lengths = 1;
    int64_t coord = 1;
    Data data = DATA__INIT;

    Data__Geometry geometry = DATA__GEOMETRY__INIT;
    geometry.type = DATA__GEOMETRY__TYPE__POINT;
    geometry.n_lengths = 1;
    geometry.lengths = &lengths;
    geometry.n_coords = 1;
    geometry.coords = &coord;
    data.data_type_case = DATA__DATA_TYPE_GEOMETRY;
    data.geometry = &geometry;

    len = data__get_packed_size(&data);
    buf = malloc(len);
    data__pack(&data, buf);
        
    printf ("Hello World! %d \n", (int) len);
}