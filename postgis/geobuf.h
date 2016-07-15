#include <stdlib.h>
#include "postgres.h"
#include "executor/spi.h"
#include "geobuf.pb-c.h"
#include "lwgeom_pg.h"
#include "lwgeom_log.h"

void *geobuf_test(size_t* len, char *geom_name, int count);
void tupdesc_analyze(char ***keys, uint32_t **properties);
void row_to_feature(int row, Data__Feature *feature, uint32_t *properties);