module;
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <memory>

export module com.citronlabs.Halftone.Graphics:EngineRegistry;

import com.citronlabs.Halftone.Core;
import :RenderEngine;

export namespace Halftone::Graphics
{
    using namespace Halftone::Core;

    // Name -> factory table of available render engines.
    class EngineRegistry {
    public:
        using Factory = Function<Unique<RenderEngine>()>;

    private:
        struct Entry {
            String  Name;
            Factory Create;
        };
        List<Entry> _Entries;

    public:
        void                Register(StringView name, Factory factory);
        bool                Contains(StringView name) const;
        Unique<RenderEngine> Create(StringView name) const;
        List<String>        Names() const;

        template<typename T>
        void Register(StringView name) { Register(name, [] { return Unique<RenderEngine>(MakeUnique<T>()); }); }
    };
}
