#include "EffectRegistry.h"

namespace dlaw
{
const EffectInfo* findEffect (const std::string& id)
{
    for (const auto& e : getAllEffects())
        if (id == e.id)
            return &e;

    return nullptr;
}
} // namespace dlaw
