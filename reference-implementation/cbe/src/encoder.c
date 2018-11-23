#include <memory.h>
#include "cbe/cbe.h"
#include "cbe_internal.h"


#define RETURN_FALSE_IF_NOT_ENOUGH_ROOM(CONTEXT, REQUIRED_BYTES) \
    if((size_t)((CONTEXT)->end - (CONTEXT)->pos) < (REQUIRED_BYTES)) return false
#define RETURN_FALSE_IF_NOT_ENOUGH_ROOM_TYPED(CONTEXT, REQUIRED_BYTES) \
    RETURN_FALSE_IF_NOT_ENOUGH_ROOM(CONTEXT, ((REQUIRED_BYTES) + sizeof(cbe_encoded_type_field)))

#define FITS_IN_INT_SMALL(VALUE)  ((VALUE) >= TYPE_SMALLINT_MIN && (VALUE) <= TYPE_SMALLINT_MAX)
#define FITS_IN_INT_8(VALUE)      ((VALUE) == (int8_t)(VALUE))
#define FITS_IN_INT_16(VALUE)     ((VALUE) == (int16_t)(VALUE))
#define FITS_IN_INT_32(VALUE)     ((VALUE) == (int32_t)(VALUE))
#define FITS_IN_INT_64(VALUE)     ((VALUE) == (int64_t)(VALUE))
#define FITS_IN_FLOAT_32(VALUE)   ((VALUE) == (double)(float)(VALUE))
#define FITS_IN_FLOAT_64(VALUE)   ((VALUE) == (double)(VALUE))
#define FITS_IN_DECIMAL_32(VALUE) ((VALUE) == (_Decimal32)(VALUE))
#define FITS_IN_DECIMAL_64(VALUE) ((VALUE) == (_Decimal64)(VALUE))

static inline unsigned int get_length_field_width(const uint64_t length)
{
    if(length <= MAX_VALUE_6_BIT)  return sizeof(uint8_t);
    if(length <= MAX_VALUE_14_BIT) return sizeof(uint16_t);
    if(length <= MAX_VALUE_30_BIT) return sizeof(uint32_t);
    return sizeof(uint64_t);
}

static inline void add_primitive_uint_8(cbe_encode_context* const context, const uint8_t value)
{
    *context->pos++ = value;
}
static inline void add_primitive_int_8(cbe_encode_context* const context, const int8_t value)
{
    add_primitive_uint_8(context, (uint8_t)value);
}
#define DEFINE_PRIMITIVE_ADD_FUNCTION(DATA_TYPE, DEFINITION_TYPE) \
    static inline void add_primitive_ ## DEFINITION_TYPE(cbe_encode_context* const context, const DATA_TYPE value) \
    { \
        /* Must clear memory first because the compiler may do a partial store where there are zero bytes in the source */ \
        memset(context->pos, 0, sizeof(value)); \
        safe_ ## DEFINITION_TYPE* safe = (safe_##DEFINITION_TYPE*)context->pos; \
        safe->contents = value; \
        context->pos += sizeof(value); \
    }
DEFINE_PRIMITIVE_ADD_FUNCTION(uint16_t,        uint_16)
DEFINE_PRIMITIVE_ADD_FUNCTION(uint32_t,        uint_32)
DEFINE_PRIMITIVE_ADD_FUNCTION(uint64_t,        uint_64)
DEFINE_PRIMITIVE_ADD_FUNCTION(int16_t,          int_16)
DEFINE_PRIMITIVE_ADD_FUNCTION(int32_t,          int_32)
DEFINE_PRIMITIVE_ADD_FUNCTION(int64_t,          int_64)
DEFINE_PRIMITIVE_ADD_FUNCTION(__int128,        int_128)
DEFINE_PRIMITIVE_ADD_FUNCTION(float,          float_32)
DEFINE_PRIMITIVE_ADD_FUNCTION(double,         float_64)
DEFINE_PRIMITIVE_ADD_FUNCTION(__float128,    float_128)
DEFINE_PRIMITIVE_ADD_FUNCTION(_Decimal32,   decimal_32)
DEFINE_PRIMITIVE_ADD_FUNCTION(_Decimal64,   decimal_64)
DEFINE_PRIMITIVE_ADD_FUNCTION(_Decimal128, decimal_128)
DEFINE_PRIMITIVE_ADD_FUNCTION(int64_t,            time)
static inline void add_primitive_bytes(cbe_encode_context* const context,
                                       const uint8_t* const bytes,
                                       const unsigned int byte_count)
{
    memcpy(context->pos, bytes, byte_count);
    context->pos += byte_count;
}

static inline void add_primitive_type(cbe_encode_context* const context, const cbe_type_field type)
{
    add_primitive_int_8(context, (int8_t)type);
}

static inline void add_primitive_length(cbe_encode_context* const context, const uint64_t length)
{
    if(length <= MAX_VALUE_6_BIT)
    {
        add_primitive_uint_8(context, (uint8_t)((length << 2) | LENGTH_FIELD_WIDTH_6_BIT));
        return;
    }
    if(length <= MAX_VALUE_14_BIT)
    {
        add_primitive_uint_16(context, (uint16_t)((length << 2) | LENGTH_FIELD_WIDTH_14_BIT));
        return;
    }
    if(length <= MAX_VALUE_30_BIT)
    {
        add_primitive_uint_32(context, (uint32_t)((length << 2) | LENGTH_FIELD_WIDTH_30_BIT));
        return;
    }
    add_primitive_uint_64(context, (length << 2) | LENGTH_FIELD_WIDTH_62_BIT);
}

static inline bool add_small(cbe_encode_context* const context, const int8_t value)
{
    RETURN_FALSE_IF_NOT_ENOUGH_ROOM_TYPED(context, 0);
    add_primitive_type(context, value);
    return true;
}

#define DEFINE_ADD_SCALAR_FUNCTION(DATA_TYPE, DEFINITION_TYPE, CBE_TYPE) \
    static inline bool add_ ## DEFINITION_TYPE(cbe_encode_context* const context, const DATA_TYPE value) \
    { \
        RETURN_FALSE_IF_NOT_ENOUGH_ROOM_TYPED(context, sizeof(value)); \
        add_primitive_type(context, CBE_TYPE); \
        add_primitive_ ## DEFINITION_TYPE(context, value); \
        return true; \
    }
DEFINE_ADD_SCALAR_FUNCTION(int16_t,          int_16, TYPE_INT_16)
DEFINE_ADD_SCALAR_FUNCTION(int32_t,          int_32, TYPE_INT_32)
DEFINE_ADD_SCALAR_FUNCTION(int64_t,          int_64, TYPE_INT_64)
DEFINE_ADD_SCALAR_FUNCTION(__int128,        int_128, TYPE_INT_128)
DEFINE_ADD_SCALAR_FUNCTION(float,          float_32, TYPE_FLOAT_32)
DEFINE_ADD_SCALAR_FUNCTION(double,         float_64, TYPE_FLOAT_64)
DEFINE_ADD_SCALAR_FUNCTION(__float128,    float_128, TYPE_FLOAT_128)
DEFINE_ADD_SCALAR_FUNCTION(_Decimal32,   decimal_32, TYPE_DECIMAL_32)
DEFINE_ADD_SCALAR_FUNCTION(_Decimal64,   decimal_64, TYPE_DECIMAL_64)
DEFINE_ADD_SCALAR_FUNCTION(_Decimal128, decimal_128, TYPE_DECIMAL_128)
DEFINE_ADD_SCALAR_FUNCTION(uint64_t,           time, TYPE_TIME)

static bool add_array(cbe_encode_context* const context,
                      const cbe_type_field array_type,
                      const uint8_t* const values,
                      const int entity_count,
                      const int entity_size)
{
    const uint8_t type = array_type;
    RETURN_FALSE_IF_NOT_ENOUGH_ROOM_TYPED(context,
                                          get_length_field_width(entity_count) +
                                          entity_count * entity_size);
    add_primitive_type(context, type);
    add_primitive_length(context, entity_count);
    add_primitive_bytes(context, values, entity_count * entity_size);
    return true;
}


cbe_encode_context cbe_new_encode_context(uint8_t* const buffer_start, uint8_t* const buffer_end)
{
    cbe_encode_context context =
    {
        .start = buffer_start,
        .pos = buffer_start,
        .end = buffer_end,
    };
    return context;
}

bool cbe_add_empty(cbe_encode_context* const context)
{
    RETURN_FALSE_IF_NOT_ENOUGH_ROOM_TYPED(context, 0);
    add_primitive_type(context, TYPE_EMPTY);
    return true;
}

bool cbe_add_boolean(cbe_encode_context* const context, const bool value)
{
    RETURN_FALSE_IF_NOT_ENOUGH_ROOM_TYPED(context, 0);
    add_primitive_type(context, value ? TYPE_TRUE : TYPE_FALSE);
    return true;
}

bool cbe_add_int(cbe_encode_context* const context, const int value)
{
    if(FITS_IN_INT_SMALL(value)) return add_small(context, value);
    if(FITS_IN_INT_16(value)) return add_int_16(context, value);
    if(FITS_IN_INT_32(value)) return add_int_32(context, value);
    if(FITS_IN_INT_64(value)) return add_int_64(context, value);
    return add_int_128(context, value);
}

bool cbe_add_int_8(cbe_encode_context* const context, int8_t const value)
{
    if(FITS_IN_INT_SMALL(value)) return add_small(context, value);
    return add_int_16(context, value);
}

bool cbe_add_int_16(cbe_encode_context* const context, int16_t const value)
{
    if(FITS_IN_INT_SMALL(value)) return add_small(context, value);
    return add_int_16(context, value);
}

bool cbe_add_int_32(cbe_encode_context* const context, int32_t const value)
{
    if(FITS_IN_INT_SMALL(value)) return add_small(context, value);
    if(FITS_IN_INT_16(value)) return add_int_16(context, value);
    return add_int_32(context, value);
}

bool cbe_add_int_64(cbe_encode_context* const context, int64_t const value)
{
    if(FITS_IN_INT_SMALL(value)) return add_small(context, value);
    if(FITS_IN_INT_16(value)) return add_int_16(context, value);
    if(FITS_IN_INT_32(value)) return add_int_32(context, value);
    return add_int_64(context, value);
}

bool cbe_add_int_128(cbe_encode_context* const context, const __int128 value)
{
    if(FITS_IN_INT_SMALL(value)) return add_small(context, value);
    if(FITS_IN_INT_16(value)) return add_int_16(context, value);
    if(FITS_IN_INT_32(value)) return add_int_32(context, value);
    if(FITS_IN_INT_64(value)) return add_int_64(context, value);
    return add_int_128(context, value);
}

bool cbe_add_float_32(cbe_encode_context* const context, const float value)
{
    return add_float_32(context, value);
}

bool cbe_add_float_64(cbe_encode_context* const context, const double value)
{
    if(FITS_IN_FLOAT_32(value)) return add_float_32(context, value);
    return add_float_64(context, value);
}

bool cbe_add_float_128(cbe_encode_context* const context, const __float128 value)
{
    if(FITS_IN_FLOAT_32(value)) return add_float_32(context, value);
    if(FITS_IN_FLOAT_64(value)) return add_float_64(context, value);
    return add_float_128(context, value);
}

bool cbe_add_decimal_32(cbe_encode_context* const context, const _Decimal32 value)
{
    return add_decimal_32(context, value);
}

bool cbe_add_decimal_64(cbe_encode_context* const context, const _Decimal64 value)
{
    if(FITS_IN_DECIMAL_32(value)) return add_decimal_32(context, value);
    return add_decimal_64(context, value);
}

bool cbe_add_decimal_128(cbe_encode_context* const context, const _Decimal128 value)
{
    if(FITS_IN_DECIMAL_32(value)) return add_decimal_32(context, value);
    if(FITS_IN_DECIMAL_64(value)) return add_decimal_64(context, value);
    return add_decimal_128(context, value);
}

int64_t cbe_new_time(int year, int day, int hour, int minute, int second, int microsecond)
{
    return ((int64_t)year    << TIME_BITSHIFT_YEAR)   |
           ((uint64_t)day    << TIME_BITSHIFT_DAY)    |
           ((uint64_t)hour   << TIME_BITSHIFT_HOUR)   |
           ((uint64_t)minute << TIME_BITSHIFT_MINUTE) |
           ((uint64_t)second << TIME_BITSHIFT_SECOND) |
           microsecond;
}

bool cbe_add_time(cbe_encode_context* const context, const int64_t value)
{
    return add_time(context, value);
}

bool cbe_start_list(cbe_encode_context* const context)
{
    RETURN_FALSE_IF_NOT_ENOUGH_ROOM_TYPED(context, sizeof(cbe_encoded_type_field));
    add_primitive_type(context, TYPE_LIST);
    return true;
}

bool cbe_start_map(cbe_encode_context* const context)
{
    RETURN_FALSE_IF_NOT_ENOUGH_ROOM_TYPED(context, sizeof(cbe_encoded_type_field));
    add_primitive_type(context, TYPE_MAP);
    return true;
}

bool cbe_end_container(cbe_encode_context* const context)
{
    RETURN_FALSE_IF_NOT_ENOUGH_ROOM_TYPED(context, 0);
    add_primitive_type(context, TYPE_END_CONTAINER);
    return true;
}

bool cbe_add_string(cbe_encode_context* const context, const char* const str)
{
    return cbe_add_substring(context, str, str + strlen(str));
}

bool cbe_add_substring(cbe_encode_context* const context, const char* const start, const char* const end)
{
    const int byte_count = end - start;
    const int small_length_max = TYPE_STRING_15 - TYPE_STRING_0;
    if(byte_count > small_length_max)
    {
        return add_array(context, TYPE_ARRAY_STRING, (const uint8_t* const)start, byte_count, sizeof(*start));
    }

    RETURN_FALSE_IF_NOT_ENOUGH_ROOM_TYPED(context, byte_count);
    const uint8_t type = TYPE_STRING_0 + byte_count;
    add_primitive_type(context, type);
    add_primitive_bytes(context, (uint8_t*)start, byte_count);
    return true;
}

bool cbe_add_array_int_8(cbe_encode_context* const context, const int8_t* const start, const int8_t* const end)
{
    return add_array(context, TYPE_ARRAY_INT_8, (const uint8_t* const)start, end - start, sizeof(*start));
}

bool cbe_add_array_int_16(cbe_encode_context* const context, const int16_t* const start, const int16_t* const end)
{
    return add_array(context, TYPE_ARRAY_INT_16, (const uint8_t* const)start, end - start, sizeof(*start));
}

bool cbe_add_array_int_32(cbe_encode_context* const context, const int32_t* const start, const int32_t* const end)
{
    return add_array(context, TYPE_ARRAY_INT_32, (const uint8_t* const)start, end - start, sizeof(*start));
}

bool cbe_add_array_int_64(cbe_encode_context* const context, const int64_t* const start, const int64_t* const end)
{
    return add_array(context, TYPE_ARRAY_INT_64, (const uint8_t* const)start, end - start, sizeof(*start));
}

bool cbe_add_array_int_128(cbe_encode_context* const context, const __int128* const start, const __int128* const end)
{
    return add_array(context, TYPE_ARRAY_INT_128, (const uint8_t* const)start, end - start, sizeof(*start));
}

bool cbe_add_array_float_32(cbe_encode_context* const context, const float* const start, const float* const end)
{
    return add_array(context, TYPE_ARRAY_FLOAT_32, (const uint8_t* const)start, end - start, sizeof(*start));
}

bool cbe_add_array_float_64(cbe_encode_context* const context, const double* const start, const double* const end)
{
    return add_array(context, TYPE_ARRAY_FLOAT_64, (const uint8_t* const)start, end - start, sizeof(*start));
}

bool cbe_add_array_float_128(cbe_encode_context* const context, const __float128* const start, const __float128* const end)
{
    return add_array(context, TYPE_ARRAY_FLOAT_128, (const uint8_t* const)start, end - start, sizeof(*start));
}

bool cbe_add_array_decimal_32(cbe_encode_context* const context, const _Decimal32* const start, const _Decimal32* const end)
{
    return add_array(context, TYPE_ARRAY_DECIMAL_32, (const uint8_t* const)start, end - start, sizeof(*start));
}

bool cbe_add_array_decimal_64(cbe_encode_context* const context, const _Decimal64* const start, const _Decimal64* const end)
{
    return add_array(context, TYPE_ARRAY_DECIMAL_64, (const uint8_t* const)start, end - start, sizeof(*start));
}

bool cbe_add_array_decimal_128(cbe_encode_context* const context, const _Decimal128* const start, const _Decimal128* const end)
{
    return add_array(context, TYPE_ARRAY_DECIMAL_128, (const uint8_t* const)start, end - start, sizeof(*start));
}

bool cbe_add_bitfield(cbe_encode_context* const context, const uint8_t* const packed_values, const int entity_count)
{
    const uint8_t type = TYPE_ARRAY_BOOLEAN;
    int byte_count = entity_count / 8;
    if(entity_count & 7)
    {
        byte_count++;
    }
    RETURN_FALSE_IF_NOT_ENOUGH_ROOM_TYPED(context,
                                          get_length_field_width(entity_count) +
                                          byte_count);
    add_primitive_type(context, type);
    add_primitive_length(context, entity_count);
    add_primitive_bytes(context, packed_values, byte_count);
    return true;
}

bool cbe_add_array_boolean(cbe_encode_context* const context, const bool* const start, const bool* const end)
{
    const int entity_count = end - start;
    const uint8_t type = TYPE_ARRAY_BOOLEAN;
    int byte_count = entity_count / 8;
    if(entity_count & 7)
    {
        byte_count++;
    }
    RETURN_FALSE_IF_NOT_ENOUGH_ROOM_TYPED(context,
                                          get_length_field_width(entity_count) +
                                          byte_count);
    add_primitive_type(context, type);
    add_primitive_length(context, entity_count);
    for(int i = 0; i < entity_count;)
    {
        uint8_t bit_pos = 1;
        uint8_t next_byte = 0;
        for(int j = 0; j < 8 && i < entity_count; j++)
        {
            if(start[i])
            {
                next_byte |= bit_pos;
            }
            bit_pos <<= 1;
            i++;
        }
        *context->pos++ = next_byte;
    }
    return true;
}
