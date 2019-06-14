#ifndef UNI_DATA_HPP
#define UNI_DATA_HPP

#include <cstdint>
#include <cstddef>

typedef std::uint32_t uni_cp;

#define DEFINE_PROP(prop, fun_name) \
	extern const std::size_t UNI_##prop##_RANGES; \
	extern const uni_cp UNI_##prop[]; \
	bool fun_name(uni_cp cp) { \
		for(std::size_t i = 0; i < UNI_##prop##_RANGES; i++) { \
			if(UNI_##prop[2*i] <= cp && cp <= UNI_##prop[2*i+1]) \
				return true; \
		} \
		return false; \
	}

DEFINE_PROP(SPACE, is_space)
DEFINE_PROP(ID_START, is_id_start)
DEFINE_PROP(ID_CONTINUE, is_id_continue)

#endif