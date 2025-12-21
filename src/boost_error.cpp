#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/builtin_types.hpp>

#include <exception>

namespace boost {
void throw_exception(std::exception const &e) {
	godot::String msg = "Boost Exception: ";
	msg += e.what();
	ERR_PRINT_ED(msg);

	// Boost expects this function not to return.
	std::terminate();
}
} //namespace boost