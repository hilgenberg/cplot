#include <stdexcept>
#include <cassert>
#include "FileVersions.h"
#include "../Utility/StringFormatting.h"

#define NEEDS_VERSION(v, thing) throw std::runtime_error(format("Cannot save " thing " prior to version %s", version_string(v)))
#define CHECK_VERSION(v, thing) if (s.version() < v) throw std::runtime_error(format("Cannot save " thing " prior to version %s", version_string(v)))

#define INCONSISTENCY(desc) do{ assert(false); throw std::logic_error(desc); }while(0)
#define CHECK_CONSISTENCY(what, desc) do{ assert(what); if (!(what)) throw std::logic_error(desc); }while(0)
