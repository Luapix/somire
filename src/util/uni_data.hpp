#ifndef UNI_DATA_HPP
#define UNI_DATA_HPP

#include <cstdint>
#include <cstddef>

typedef std::uint32_t uni_cp;

#define DECLARE_PROP(fun_name) bool fun_name(uni_cp cp);
#define DEFINE_PROP(fun_name, prop) \
	bool fun_name(uni_cp cp) { \
		for(std::size_t i = 0; i < UNI_##prop##_RANGES; i++) { \
			if(UNI_##prop[2*i] <= cp && cp <= UNI_##prop[2*i+1]) \
				return true; \
		} \
		return false; \
	}

DECLARE_PROP(isSpace)
DECLARE_PROP(isIdStart)
DECLARE_PROP(isIdContinue)
DECLARE_PROP(isGraphic)

#endif