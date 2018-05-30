#include "test_helpers.h"

static std::vector<uint8_t> make_bytes_with_length(int length)
{
	std::vector<uint8_t> bytes;
	for(int i = 0; i < length; i++)
	{
		bytes.push_back((uint8_t)(i & 0x7f));
	}
	return bytes;
}

static void test_inferred_bytes_write(int length)
{
	std::vector<uint8_t> bytes = make_bytes_with_length(length);
	std::vector<uint8_t> expected_memory(bytes);
	uint8_t type = TYPE_BYTES_0 + length;
	expected_memory.insert(expected_memory.begin(), type);
	expect_memory_after_write(bytes, expected_memory);
}

static void test_bytes_write(int length, std::vector<uint8_t> length_field_values)
{
	std::vector<uint8_t> bytes = make_bytes_with_length(length);
	std::vector<uint8_t> expected_memory(bytes);
	uint8_t type = TYPE_BYTES;
	expected_memory.insert(expected_memory.begin(), length_field_values.begin(), length_field_values.end());
	expected_memory.insert(expected_memory.begin(), type);
	expect_memory_after_write(bytes, expected_memory);
}

#define DEFINE_INFERRED_BYTES_WRITE_TEST(LENGTH) \
TEST(BytesTest, inferred_length_ ## LENGTH) \
{ \
    test_inferred_bytes_write(LENGTH); \
}

#define DEFINE_BYTES_WRITE_TEST(LENGTH, ...) \
TEST(BytesTest, length_ ## LENGTH) \
{ \
    test_bytes_write(LENGTH, __VA_ARGS__); \
}

DEFINE_INFERRED_BYTES_WRITE_TEST(0)
DEFINE_INFERRED_BYTES_WRITE_TEST(1)
DEFINE_INFERRED_BYTES_WRITE_TEST(2)
DEFINE_INFERRED_BYTES_WRITE_TEST(3)
DEFINE_INFERRED_BYTES_WRITE_TEST(4)
DEFINE_INFERRED_BYTES_WRITE_TEST(5)
DEFINE_INFERRED_BYTES_WRITE_TEST(6)
DEFINE_INFERRED_BYTES_WRITE_TEST(7)
DEFINE_INFERRED_BYTES_WRITE_TEST(8)
DEFINE_INFERRED_BYTES_WRITE_TEST(9)
DEFINE_INFERRED_BYTES_WRITE_TEST(10)
DEFINE_INFERRED_BYTES_WRITE_TEST(11)
DEFINE_INFERRED_BYTES_WRITE_TEST(12)
DEFINE_INFERRED_BYTES_WRITE_TEST(13)
DEFINE_INFERRED_BYTES_WRITE_TEST(14)
DEFINE_INFERRED_BYTES_WRITE_TEST(15)

DEFINE_BYTES_WRITE_TEST( 16, {16});
DEFINE_BYTES_WRITE_TEST( 17, {17});
DEFINE_BYTES_WRITE_TEST(251, {251});
DEFINE_BYTES_WRITE_TEST(252, {252});
DEFINE_BYTES_WRITE_TEST(253, {LENGTH_16BIT, 0xfd, 0x00});
DEFINE_BYTES_WRITE_TEST(254, {LENGTH_16BIT, 0xfe, 0x00});
DEFINE_BYTES_WRITE_TEST(255, {LENGTH_16BIT, 0xff, 0x00});
DEFINE_BYTES_WRITE_TEST(256, {LENGTH_16BIT, 0x00, 0x01});
DEFINE_BYTES_WRITE_TEST(65535, {LENGTH_16BIT, 0xff, 0xff});
DEFINE_BYTES_WRITE_TEST(65536, {LENGTH_32BIT, 0x00, 0x00, 0x01, 0x00});
