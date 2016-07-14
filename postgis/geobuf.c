#include "geobuf.h"

void *geobuf_test(size_t* len) {
    void *buf;
    uint32_t lengths = 1;
    uint32_t properties[2];
    int64_t coord = 1;
    Data data = DATA__INIT;
    Data__Geometry geometry = DATA__GEOMETRY__INIT;
    Data__Feature feature = DATA__FEATURE__INIT;
    Data__Value **values;
    Data__Value value = DATA__VALUE__INIT;

    values = malloc (sizeof (Data__Value*));
    values[0] = &value;
    properties[0] = 1;
    properties[1] = 2;

    geometry.type = DATA__GEOMETRY__TYPE__POINT;
    geometry.n_lengths = 1;
    geometry.lengths = &lengths;
    geometry.n_coords = 1;
    geometry.coords = &coord;
    // data.data_type_case = DATA__DATA_TYPE_GEOMETRY;
    data.data_type_case = DATA__DATA_TYPE_FEATURE;
    // data.geometry = &geometry;
    feature.geometry = &geometry;
    value.value_type_case = DATA__VALUE__VALUE_TYPE_STRING_VALUE;
    value.string_value = "mooo";
    feature.n_values = 1;
    feature.values = values;
    feature.n_properties = 2;
    feature.properties = properties;
    data.feature = &feature;
    

    *len = data__get_packed_size(&data);
    buf = malloc(*len);
    data__pack(&data, buf);
    return buf;
}