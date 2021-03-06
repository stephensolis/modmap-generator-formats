#ifndef _KAMERIS_FORMATS_HEADERS_
#define _KAMERIS_FORMATS_HEADERS_

#include <array>
#include <cstdint>

#include "element_type.hpp"

namespace kameris {
	struct repr_header {
		//0 at the end here is the version number
		static constexpr std::array<char, 7> signature = {'M', 'M', 'R', 'E', 'P', 'R', '\0'};

		bool /*(stored as uint8_t)*/ is_sparse;
		element_type key_type;
		element_type value_type;
		uint64_t count;
		uint64_t rows, cols;
	};
	constexpr std::array<char, 7> repr_header::signature; // NOLINT
	constexpr size_t repr_header_size = repr_header::signature.size() + sizeof(uint8_t) +
		sizeof repr_header().key_type + sizeof repr_header().value_type + sizeof repr_header().count +
		sizeof repr_header().rows + sizeof repr_header().cols;

	struct dist_header {
		//0 at the end here is the version number
		static constexpr std::array<char, 7> signature = {'M', 'M', 'D', 'I', 'S', 'T', '\0'};

		element_type value_type;
		uint64_t size;
	};
	constexpr std::array<char, 7> dist_header::signature; // NOLINT
	constexpr size_t dist_header_size =
		dist_header::signature.size() + sizeof dist_header().value_type + sizeof dist_header().size;
}

#endif
