#include "EffectRegistry.h"

#include <sstream>

namespace dlaw
{
const EffectInfo* findEffect (const std::string& id)
{
    for (const auto& e : getAllEffects())
        if (id == e.id)
            return &e;

    return nullptr;
}
int compareVersions (const std::string& a, const std::string& b)
{
    std::istringstream sa (a), sb (b);
    for (int part = 0; part < 3; ++part)
    {
        long x = 0, y = 0;
        char dot = 0;
        sa >> x; sb >> y;
        if (x != y)
            return x < y ? -1 : 1;
        sa >> dot; sb >> dot;
    }
    return 0;
}

} // namespace dlaw
