#ifndef UNI_UTIL_HPP
#define UNI_UTIL_HPP

#include <string>
#include <iterator>

#include "utf8.h"

#include "uni_data.hpp"

void appendCP(std::string& s, uni_cp cp);
std::string strFromCP(uni_cp cp);

bool isDigit(uni_cp cp);

#endif