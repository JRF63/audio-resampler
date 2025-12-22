#include <godot_cpp/classes/os.hpp>

#include <exception>

namespace boost {
void throw_exception(std::exception const &e) {
	godot::String msg = "Boost Exception: ";
	msg += e.what();

	// Boost expects this function not to return
	godot::OS::get_singleton()->crash(msg);
}
} //namespace boost