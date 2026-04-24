#include "../../include/lunalify/lunalify.hpp"
#include <shellapi.h>

namespace Lunalify {
    std::string Escape(const std::string& input) {
        std::string output = input;
        size_t pos = 0;
        while ((pos = output.find("'", pos)) != std::string::npos) {
            output.replace(pos, 1, "''");
            pos += 2;
        }
        return output;
    }
}
