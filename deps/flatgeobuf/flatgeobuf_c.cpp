#include "flatgeobuf_c.h"
#include "feature_generated.h"
#include "geometrywriter.h"
#include "geometryreader.h"

using namespace flatbuffers;
using namespace FlatGeobuf;

typedef flatgeobuf_ctx ctx;

int flatgeobuf_encode_header(ctx *ctx)
{
    FlatBufferBuilder fbb;

    // inspect first geometry
	if (ctx->lwgeom != NULL) {
        if (lwgeom_has_srid(ctx->lwgeom))
            ctx->srid = ctx->lwgeom->srid;
		ctx->has_z = lwgeom_has_z(ctx->lwgeom);
		ctx->has_m = lwgeom_has_m(ctx->lwgeom);
		ctx->geometry_type = (uint8_t) GeometryWriter::get_geometrytype(ctx->lwgeom);
	} else {
		LWDEBUG(2, "ctx->lwgeom is null");
		ctx->geometry_type = 0;
	}

    LWDEBUGF(2, "ctx->geometry_type %d", ctx->geometry_type);

    std::vector<flatbuffers::Offset<FlatGeobuf::Column>> columns;
    std::vector<flatbuffers::Offset<FlatGeobuf::Column>> *pColumns = nullptr;

    if (ctx->columns_size > 0) {
        for (uint16_t i = 0; i < ctx->columns_size; i++) {
            auto c = ctx->columns[i];
            columns.push_back(CreateColumnDirect(fbb, c->name, (ColumnType) c->type));
        }
    }

    if (columns.size() > 0)
        pColumns = &columns;

    flatbuffers::Offset<Crs> crs = 0;
    if (ctx->srid > 0)
        crs = CreateCrsDirect(fbb, nullptr, ctx->srid);

    const auto header = CreateHeaderDirect(
        fbb, ctx->name, 0, (GeometryType) ctx->geometry_type, ctx->has_z, ctx->has_m, ctx->has_t, ctx->has_tm, pColumns, ctx->features_count, 16, crs);
    fbb.FinishSizePrefixed(header);
    const auto buffer = fbb.GetBufferPointer();
    const auto size = fbb.GetSize();

    LWDEBUGF(2, "header size %d (with size prefix)", size);

    Verifier verifier(buffer, size - sizeof(uoffset_t));
    if (VerifySizePrefixedHeaderBuffer(verifier)) {
        lwerror("buffer did not pass verification");
        return -1;
    }

    ctx->buf = (uint8_t *) lwrealloc(ctx->buf, ctx->offset + size);
	LWDEBUGF(2, "copying to ctx->buf at offset %ld", ctx->offset);
	memcpy(ctx->buf + ctx->offset, buffer, size);

	ctx->offset += size;

    return 0;
}

static const std::vector<uint8_t> encode_properties(ctx *ctx, FlatBufferBuilder &fbb)
{
    std::vector<uint8_t> properties;
    properties.reserve(1024 * 4);
    return properties;
}

int flatgeobuf_encode_feature(ctx *ctx)
{
    FlatBufferBuilder fbb;
    Offset<Geometry> geometry = 0;
    Offset<Vector<uint8_t>> properties = 0;

    if (ctx->lwgeom != NULL) {
        LWDEBUGG(3, ctx->lwgeom, "GeometryWriter input LWGEOM");
        GeometryWriter writer(fbb, ctx->lwgeom, (GeometryType) ctx->geometry_type, ctx->has_z, ctx->has_m);
        geometry = writer.write(0);
        LWDEBUGF(3, "WENDS: %ld", writer.m_ends.size());
        for (size_t i = 0; i < writer.m_ends.size(); i++) {
            LWDEBUGF(3, "WENDS: %d", writer.m_ends[i]);
        }
    }
    if (ctx->properties_len > 0)
        properties = fbb.CreateVector<uint8_t>(ctx->properties, ctx->properties_len);
    FeatureBuilder builder(fbb);
    builder.add_geometry(geometry);
    builder.add_properties(properties);
    auto feature = builder.Finish();
    fbb.FinishSizePrefixed(feature);
    const auto buffer = fbb.GetBufferPointer();
    const auto size = fbb.GetSize();

    LWDEBUGF(3, "encode_feature size %ld", size);

    Verifier verifier(buffer, size - sizeof(uoffset_t));
    if (VerifySizePrefixedFeatureBuffer(verifier)) {
        lwerror("buffer did not pass verification");
        return -1;
    }

	LWDEBUGF(3, "reallocating ctx->buf to size %ld", ctx->offset + size);
	ctx->buf = (uint8_t * ) lwrealloc(ctx->buf, ctx->offset + size);
	LWDEBUGF(3, "copying feature to ctx->buf at offset %ld", ctx->offset);
	memcpy(ctx->buf + ctx->offset, buffer, size);

	ctx->offset += size;
	ctx->features_count++;

    return 0;
}

int flatgeobuf_decode_feature(ctx *ctx)
{
    LWDEBUGF(2, "reading size prefix at %ld", ctx->offset);
    auto size = flatbuffers::GetPrefixedSize(ctx->buf + ctx->offset);
	LWDEBUGF(2, "size is %ld (without size prefix)", size);

    Verifier verifier(ctx->buf + ctx->offset, size);
	if (VerifySizePrefixedFeatureBuffer(verifier)) {
        lwerror("buffer did not pass verification");
        return -1;
    }

    ctx->offset += sizeof(uoffset_t);

    auto feature = GetFeature(ctx->buf + ctx->offset);
    ctx->offset += size;

    const auto geometry = feature->geometry();
    if (geometry != nullptr) {
        if (geometry->ends() != nullptr) {
            LWDEBUGF(3, "ENDS: %d", geometry->ends()->size());
            for (uint32_t i = 0; i < geometry->ends()->size(); i++) {
                LWDEBUGF(3, "ENDS: %d", geometry->ends()->Get(i));
            }
        }

        LWDEBUGF(3, "Constructing GeometryReader with geometry_type %d has_z %d haz_m %d", ctx->geometry_type, ctx->has_z, ctx->has_m);
        GeometryReader reader(geometry, (GeometryType) ctx->geometry_type, ctx->has_z, ctx->has_m);
        ctx->lwgeom = reader.read();
        if (ctx->srid > 0)
            lwgeom_set_srid(ctx->lwgeom, ctx->srid);
        LWDEBUGG(3, ctx->lwgeom, "GeometryReader output LWGEOM");
    } else {
        ctx->lwgeom = NULL;
    }
    if (feature->properties() != nullptr && feature->properties()->size() != 0) {
        ctx->properties = (uint8_t *) feature->properties()->data();
        ctx->properties_len = feature->properties()->size();
    } else {
        ctx->properties_len = 0;
    }

    return 0;
}

int flatgeobuf_decode_header(ctx *ctx)
{
    LWDEBUGF(2, "reading size prefix at %ld", ctx->offset);
    auto size = flatbuffers::GetPrefixedSize(ctx->buf + ctx->offset);
	LWDEBUGF(2, "size is %ld (without size prefix)", size);

    Verifier verifier(ctx->buf + ctx->offset, size);
	if (VerifySizePrefixedHeaderBuffer(verifier)) {
        lwerror("buffer did not pass verification");
        return -1;
    }

    ctx->offset += sizeof(uoffset_t);

    LWDEBUGF(2, "reading header at %ld with size %ld", ctx->offset, size);
    auto header = GetHeader(ctx->buf + ctx->offset);
	ctx->offset += size;

	ctx->geometry_type = (uint8_t) header->geometry_type();
	ctx->has_z = header->has_z();
    ctx->has_m = header->has_m();
    ctx->has_t = header->has_t();
    ctx->has_tm = header->has_tm();
    auto crs = header->crs();
    if (crs != nullptr)
        ctx->srid = crs->code();
    auto columns = header->columns();
    if (columns != nullptr) {
        auto size = columns->size();
        ctx->columns = (flatgeobuf_column **) lwalloc(sizeof(flatgeobuf_column *) * size);
        ctx->columns_size = size;
        for (uint32_t i = 0; i < size; i++) {
            auto column = columns->Get(i);
            ctx->columns[i] = (flatgeobuf_column *) lwalloc(sizeof(flatgeobuf_column));
            memset(ctx->columns[i], 0, sizeof(flatgeobuf_column));
            ctx->columns[i]->name = column->name()->c_str();
            ctx->columns[i]->type = (uint8_t) column->type();
        }
    }

	LWDEBUGF(2, "ctx->geometry_type: %d", ctx->geometry_type);
	LWDEBUGF(2, "ctx->columns_len: %d", ctx->columns_size);

    return 0;
}