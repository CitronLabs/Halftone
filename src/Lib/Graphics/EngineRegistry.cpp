module;
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <memory>
#include <utility>

module com.citronlabs.Halftone.Graphics;

import com.citronlabs.Halftone.Core;

using namespace Halftone::Graphics;
using namespace Halftone::Core;

void EngineRegistry::Register(StringView name, Factory factory)
{
    for (auto& entry : _Entries)
        if (entry.Name == name) { entry.Create = std::move(factory); return; }

    _Entries.push_back(Entry { .Name = String(name), .Create = std::move(factory) });
}

bool EngineRegistry::Contains(StringView name) const
{
    return std::any_of(_Entries.begin(), _Entries.end(), [&](const Entry& e) { return e.Name == name; });
}

Unique<RenderEngine> EngineRegistry::Create(StringView name) const
{
    for (auto& entry : _Entries)
        if (entry.Name == name) return entry.Create();
    return nullptr;
}

List<String> EngineRegistry::Names() const
{
    List<String> names;
    for (auto& entry : _Entries) names.push_back(entry.Name);
    return names;
}
