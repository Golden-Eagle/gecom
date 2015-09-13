
/*

"#shader xxx" expands to "#define _WANT_XXX_"

append
'''
#if defined(_XXX_) && !defined(_WANT_XXX_)
#error _gecom_unwanted_shader_stage_
#endif
'''
to source for all XXX.

try compile shader for all #shader directives found.

if shader compilation errors, check error string for "_gecom_unwanted_shader_stage_".
if found, discard compilation attempt.

*/


#include <cassert>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <chrono>
#include <mutex>
#include <limits>

#include "Util.hpp"

#include "Shader.hpp"

namespace gecom {



}
