#include <stdlib.h>
#include "postgres.h"
#include "executor/spi.h"

#include "../postgis_config.h"

#include "liblwgeom.h"
#include "lwgeom_pg.h"
#include "lwgeom_log.h"

#include "geobuf.pb-c.h"

void *geobuf_test(size_t* len, char *geom_name, int count);
void tupdesc_analyze(char ***keys, uint32_t **properties, char* geom_name);
Data__Feature* encode_feature(int row, char* geom_name, uint32_t* properties);
Data__Geometry* encode_geometry(int row, char* geom_name);
void encode_properties(int row, Data__Feature *feature, uint32_t *properties, char* geom_name);
LWGEOM* get_lwgeom(int row);
Data__Geometry* encode_point(LWGEOM* lwgeom);