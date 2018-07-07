#ifndef _MMG_FORMATS_REPR_READER_
#define _MMG_FORMATS_REPR_READER_

#include <fstream>
#include <ios>
#include <memory>
#include <mutex>
#include <numeric>
#include <string>
#include <utility>

#include <boost/mpl/back_inserter.hpp>
#include <boost/mpl/copy.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/variant.hpp>

#include <libkameris/utils/matrix_vector_adapters.hpp>
#include <libkameris/utils/types.hpp>

#include "common/binary_collection_io.hpp"
#include "common/binary_io.hpp"

#include "headers.hpp"

namespace kameris {
	class repr_reader {
	 private:
		std::ifstream _file;
		const repr_header _header;
		std::unique_ptr<uint64_t[]> _header_sizes;
		std::mutex _lock;

		//boost::mpl needs some help to work with template template and variadic template args
		template <typename Key, typename Value>
		struct SparseVectorAdapterTypeWrapper {
			using map_type = decltype(read_map_binary<Key, Value>(std::declval<std::istream &>(), size_t()));
			using type = decltype(make_sparse_vector_adapter(std::declval<map_type>(), size_t()));
		};

		repr_header read_header() {
			const std::unique_ptr<char[]> file_signature(read_array_binary<char>(_file, repr_header::signature.size()));
			for (size_t i = 0; i < repr_header::signature.size(); ++i) {
				if (file_signature[i] != repr_header::signature[i]) {
					throw std::invalid_argument("The given file is not a valid mm-repr file");
				}
			}

			repr_header result{};
			uint8_t is_sparse_val;
			element_underlying_t key_val = 0, value_val = 0;

			read_binary(_file, is_sparse_val);
			result.is_sparse = static_cast<bool>(is_sparse_val);
			read_binary(_file, key_val);
			result.key_type = static_cast<element_type>(key_val);
			read_binary(_file, value_val);
			result.value_type = static_cast<element_type>(value_val);
			read_binary(_file, result.count);
			read_binary(_file, result.rows);
			read_binary(_file, result.cols);

			return result;
		}

	 public:
		explicit repr_reader(const std::string &filename) : _file(filename, std::ios::binary), _header(read_header()) {
			if (_header.is_sparse) {
				_header_sizes.reset(read_array_binary<uint64_t>(_file, _header.count));
			}
		}

		const repr_header &header() const {
			return _header;
		}

		auto read_matrix(size_t index) {
			using regular_matrix_types =
				typename boost::mpl::transform<element_type_types, MatrixAdapter<boost::mpl::_1>>::type;
			using sparse_vector_types = typename boost::mpl::transform<element_type_types,
				SparseVectorAdapterTypeWrapper<uint64_t, boost::mpl::_1>>::type;
			typename boost::make_variant_over<boost::mpl::copy<sparse_vector_types,
				boost::mpl::back_inserter<regular_matrix_types>>::type>::type result;

			_lock.lock();
			dispatch_on_element_type(_header.value_type, [&](auto dummy_val = 0) {
				using Value = decltype(dummy_val);

				if (_header.is_sparse) {
					dispatch_on_element_type(_header.key_type, [&](auto dummy_key = 0) {
						using Key = decltype(dummy_key);

						_file.seekg(repr_header_size + (sizeof(uint64_t) * _header.count) +
							std::accumulate(&_header_sizes[0], &_header_sizes[index], uint64_t(0)));
						result = make_sparse_vector_adapter<uint64_t, Value>(
							read_map_binary<Key, Value>(_file, _header_sizes[index]), _header.rows * _header.cols);
					});
				} else {
					_file.seekg(repr_header_size + (index * _header.rows * _header.cols * sizeof(Value)));
					result = make_matrix_adapter<Value>(
						read_array_binary<Value>(_file, _header.rows * _header.cols), _header.rows, _header.cols, true);
				}
			});
			_lock.unlock();
			return result;
		}
	};
}

#endif
