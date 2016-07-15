#include <stdlib.h>
#include "postgres.h"
#include "executor/spi.h"
#include "geobuf.pb-c.h"

void *geobuf_test(size_t* len, int count);
void tupdesc_analyze(char **keys, uint32_t *properties);
void row_to_feature(int row, Data__Feature *feature, uint32_t *properties);