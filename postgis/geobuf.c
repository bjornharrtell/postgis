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

LWGEOM* get_lwgeom(int row) {
    Datum datum;
    GSERIALIZED *geom;
    bool isnull;
    LWGEOM* lwgeom;
    datum = SPI_getbinval(SPI_tuptable->vals[row], SPI_tuptable->tupdesc, 2, &isnull);
    geom = (GSERIALIZED *) PG_DETOAST_DATUM(datum);
    lwgeom = lwgeom_from_gserialized(geom);
    return lwgeom;
}

Data__Geometry* encode_point(LWPOINT* lwpoint) {
    int i, npoints;
    Data__Geometry* geometry;
    uint32_t lengths;
    int64_t* coord;
    POINTARRAY *pa;

    geometry = malloc (sizeof (Data__Geometry));
    data__geometry__init(geometry);
    geometry->type = DATA__GEOMETRY__TYPE__POINT;

    pa = lwpoint->point;
    npoints = pa->npoints;

    if (npoints == 0) return geometry;

    coord = malloc(sizeof (int64_t) * npoints * 2);
    // TODO: find out why pa->npoints does not work (seems to be 1?!)
    //int num = npoints;
    for (i = 0; i < npoints; i++) {
        const POINT2D *pt;
        pt = getPoint2d_cp(pa, i);
        coord[i * 2] = pt->x * 10e5;
        coord[i * 2 + 1] = pt->y * 10e5;
    }

    //lengths = 2;
    //geometry->n_lengths = 1;
    //geometry->lengths = &lengths;
    geometry->n_coords = npoints * 2;
    geometry->coords = coord;

    return geometry;
}

Data__Geometry* encode_line(LWLINE* lwline) {
    int i, c, npoints;
    Data__Geometry* geometry;
    uint32_t lengths;
    int64_t* coord;
    POINTARRAY *pa;

    geometry = malloc (sizeof (Data__Geometry));
    data__geometry__init(geometry);
    geometry->type = DATA__GEOMETRY__TYPE__LINESTRING;

    pa = lwline->points;
    npoints = pa->npoints;

    if (npoints == 0) return geometry;

    coord = malloc(sizeof (int64_t) * npoints * 2);
    // TODO: find out why pa->npoints does not work (seems to be 1?!)
    //int num = npoints;
    c = 0;
    int64_t sx = 0;
    int64_t sy = 0;
    for (i = 0; i < npoints; i++) {
        const POINT2D *pt;
        pt = getPoint2d_cp(pa, i);
        sx += coord[c++] = pt->x * 10e5 - sx;
        sy += coord[c++] = pt->y * 10e5 - sy;
    }

    //lengths = 1;
    //geometry->n_lengths = 1;
    //geometry->lengths = &lengths;
    geometry->n_coords = npoints * 2;
    geometry->coords = coord;

    return geometry;
}

Data__Geometry* encode_geometry(int row, char* geom_name) {
    LWGEOM* lwgeom = get_lwgeom(row);
    int type = lwgeom->type;
    switch (type)
	{
	case POINTTYPE:
		return encode_point((LWPOINT*)lwgeom);
	case LINETYPE:
		return encode_line((LWLINE*)lwgeom);
	case POLYGONTYPE:
		return encode_point((LWPOINT*)lwgeom);
	case MULTIPOINTTYPE:
		return encode_point((LWPOINT*)lwgeom);
	case MULTILINETYPE:
		return encode_point((LWPOINT*)lwgeom);
	case MULTIPOLYGONTYPE:
		return encode_point((LWPOINT*)lwgeom);
	case COLLECTIONTYPE:
		return encode_point((LWPOINT*)lwgeom);
	default:
		lwerror("lwgeom_to_geojson: '%s' geometry type not supported",
		        lwtype_name(type));
	}
    return NULL;
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