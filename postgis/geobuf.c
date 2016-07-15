#include "geobuf.h"

void *geobuf_test(size_t* len, int count) {
    int i;
    void *buf;
    //uint32_t lengths = 1;
    //int64_t coord = 1;
    char **keys = NULL;
    uint32_t *properties = NULL;
    Data data = DATA__INIT;
    Data__FeatureCollection feature_collection = DATA__FEATURE_COLLECTION__INIT;
    //Data__Feature *features;
    
    /*
    Data__Geometry geometry = DATA__GEOMETRY__INIT;
    Data__Feature feature = DATA__FEATURE__INIT;
    Data__Value **values;
    Data__Value value = DATA__VALUE__INIT;
    */

    tupdesc_analyze(keys, properties);
    data.keys = keys;
    data.data_type_case = DATA__DATA_TYPE_FEATURE_COLLECTION;
    data.feature_collection = &feature_collection;
    feature_collection.features = malloc (sizeof (Data__Feature*) * count);
    for (i = 0; i < count; i++) {
        Data__Feature feature = DATA__FEATURE__INIT;
        row_to_feature(i, &feature, properties);
        feature_collection.features[i] = &feature;
    }

    // char* data = SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1);

    /*
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
    data.feature = &feature;*/

    *len = data__get_packed_size(&data);
    buf = malloc(*len);
    data__pack(&data, buf);
    return buf;
}

void tupdesc_analyze(char **keys, uint32_t *properties) {
    int i;
    int natts = SPI_tuptable->tupdesc->natts;
    keys = malloc(sizeof (char*) * natts);
    properties = malloc(sizeof (uint32_t) * natts * 2);
    for (i = 0; i < natts; i++) {
        keys[i] = SPI_tuptable->tupdesc->attrs[i]->attname.data;
        properties[i * 2] = i;
        properties[i * 2 + 1] = i;
    }
}

void row_to_feature(int row, Data__Feature *feature, uint32_t *properties) {
    int i;
    Data__Value **values;
    //HeapTuple tuple = SPI_tuptable->vals[row];
    int natts = SPI_tuptable->tupdesc->natts;
    values = malloc (sizeof (Data__Value*) * natts);
    for (i = 0; i < natts; i++) {
        //bool isnull;
        //Datum datum;
        // datum = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, i + 1, &isnull);
        // TODO: Process datum depending on meta
        char * string_value = SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, i + 1);
        Data__Value value = DATA__VALUE__INIT;
        value.string_value = string_value;
        values[i] = &value;
    }
    feature->n_values = natts;
    feature->values = values;
    feature->n_properties = natts * 2;
    feature->properties = properties;
}