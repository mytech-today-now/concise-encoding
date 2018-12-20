#pragma once

#include <functional>
#include <cbe/cbe.h>
#include "encoding.h"

class cbe_encoder: public enc::encoder
{
private:
	std::vector<uint8_t> _buffer;
	std::vector<uint8_t> _encoded_data;
	cbe_encode_process* _process;
	std::function<bool(uint8_t* data_start, int64_t length)> _on_data_ready;
	bool _process_has_ended = false;

protected:
	bool flush_buffer();

public:
	// Internal functions
	cbe_encode_status encode(enc::number_encoding<int8_t>& e);
	cbe_encode_status encode(enc::number_encoding<int16_t>& e);
	cbe_encode_status encode(enc::number_encoding<int32_t>& e);
	cbe_encode_status encode(enc::number_encoding<int64_t>& e);
	cbe_encode_status encode(enc::int128_encoding& e);
	cbe_encode_status encode(enc::number_encoding<float>& e);
	cbe_encode_status encode(enc::number_encoding<double>& e);
	cbe_encode_status encode(enc::number_encoding<__float128>& e);
	cbe_encode_status encode(enc::dfp_encoding<_Decimal32>& e);
	cbe_encode_status encode(enc::dfp_encoding<_Decimal64>& e);
	cbe_encode_status encode(enc::dfp_encoding<_Decimal128>& e);
	cbe_encode_status encode(enc::boolean_encoding& e);
	cbe_encode_status encode(enc::time_encoding& e);
	cbe_encode_status encode(enc::empty_encoding& e);
	cbe_encode_status encode(enc::list_encoding& e);
	cbe_encode_status encode(enc::map_encoding& e);
	cbe_encode_status encode(enc::padding_encoding& e);
	cbe_encode_status stream_array(const std::vector<uint8_t>& data);
	cbe_encode_status encode(enc::string_encoding& e);
	cbe_encode_status encode(enc::binary_encoding& e);
	cbe_encode_status encode(enc::end_container_encoding& e);

public:
	cbe_encoder(int64_t buffer_size=10000,
		std::function<bool(uint8_t* data_start, int64_t length)> on_data_ready =
			[](uint8_t* data_start, int64_t length){(void)data_start; (void)length; return true;});

	~cbe_encoder();

	// Encode an encoding object and all linked objects.
	cbe_encode_status encode(std::shared_ptr<enc::encoding> enc);

	// Get the complete raw encoded data.
	std::vector<uint8_t>& encoded_data() {return _encoded_data;}
};