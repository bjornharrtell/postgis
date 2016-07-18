#include "geobuf.h"

void tupdesc_analyze(char*** keys, uint32_t** properties, char* geom_name) {
    int i, c = 0;
    int natts = SPI_tuptable->tupdesc->natts;
    *keys = malloc(sizeof (char *) * (natts - 1));
    *properties = malloc(sizeof (uint32_t) * (natts - 1) * 2);
    for (i = 0; i < natts; i++) {
        char* key = SPI_tuptable->tupdesc->attrs[i]->attname.data;
        if (strcmp(key, geom_name) == 0) continue;
        (*keys)[c] = key;
        (*properties)[c * 2] = c;
        (*properties)[c * 2 + 1] = c;
        c++;
    }
}

void encode_properties(int row, Data__Feature* feature, uint32_t* properties, char* geom_name) {
    int i, c;
    Data__Value** values;
    int natts = SPI_tuptable->tupdesc->natts;
    //HeapTuple tuple = SPI_tuptable->vals[row];
    values = malloc (sizeof (Data__Value *) * (natts - 1));
    c = 0;
    for (i = 0; i < natts; i++) {
        Data__Value* value;
        char* string_value;
        char* key;
        key = SPI_tuptable->tupdesc->attrs[i]->attname.data;
        if (strcmp(key, geom_name) == 0) continue;
        //bool isnull;
        //Datum datum;
        // datum = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, i + 1, &isnull);
        // TODO: Process datum depending on meta
        string_value = SPI_getvalue(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, i + 1);
        value = malloc (sizeof (Data__Value));
        data__value__init(value);
        value->value_type_case = DATA__VALUE__VALUE_TYPE_STRING_VALUE;
        value->string_value = string_value;
        values[c] = value;
        c++;
    }

    feature->n_values = natts - 1;
    feature->values = values;
    feature->n_properties = (natts - 1) * 2;
    feature->properties = properties;
}

Data__Geometry* encode_geometry(int row, char* geom_name) {
    //Datum datum;
    //bool isnull;
    
    // TODO: find out why PG_LWGEOM is unknown...
    //PG_LWGEOM * test;
    Data__Geometry* geometry;

    uint32_t lengths = 1;
    int64_t coord = 1;
    // datum = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, i + 1, &isnull);
    // lwgeom = (PG_LWGEOM *) PG_DETOAST_DATUM(datum);
    geometry = malloc (sizeof (Data__Geometry));
    data__geometry__init(geometry);
    geometry->type = DATA__GEOMETRY__TYPE__POINT;
    geometry->n_lengths = 1;
    geometry->lengths = &lengths;
    geometry->n_coords = 1;
    geometry->coords = &coord;
    return geometry;
}

Data__Feature* encode_feature(int row, char* geom_name, uint32_t* properties) {
    Data__Feature* feature;
    feature = malloc (sizeof (Data__Feature));
    data__feature__init(feature);
    feature->geometry = encode_geometry(row, geom_name);
    encode_properties(row, feature, properties, geom_name);
    return feature;
}

void *geobuf_test(size_t* len, char* geom_name, int count) {
    int i;
    void* buf;
    char** keys;
    uint32_t* properties;
    Data data = DATA__INIT;
    Data__FeatureCollection feature_collection = DATA__FEATURE_COLLECTION__INIT;
    
    tupdesc_analyze(&keys, &properties, geom_name);

    data.n_keys = SPI_tuptable->tupdesc->natts;
    data.keys = keys;
    data.data_type_case = DATA__DATA_TYPE_FEATURE_COLLECTION;
    data.feature_collection = &feature_collection;
    feature_collection.n_features = count;
    feature_collection.features = malloc (sizeof (Data__Feature*) * count);
    for (i = 0; i < count; i++) {
        feature_collection.features[i] = encode_feature(i, geom_name, properties);
    }

    *len = data__get_packed_size(&data);
    buf = malloc(*len);
    data__pack(&data, buf);
    return buf;
}