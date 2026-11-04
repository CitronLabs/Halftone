module;

export module com.citronlabs.Halftone.App:Config;

import com.citronlabs.Halftone.Core;

export namespace Halftone::App {

    using namespace Halftone::Core;

    // User preferences persisted as JSON. Defaults live in
    // src/App/Config/Preferences.cpp; anything missing from the user's file is
    // filled from them, so new keys can be added without migrations.
    class Preferences {
    private:
        json   _Data;
        String _Path;

        Preferences();
    public:
        static Preferences& Get() {
            static Preferences instance;
            return instance;
        }

        Preferences(const Preferences&)             = delete;
        Preferences& operator=(const Preferences&)  = delete;
        Preferences(Preferences&&)                  = delete;
        Preferences& operator=(Preferences&&)       = delete;

        // Loads `path` (creating it from defaults if missing). Returns false
        // only if an existing file could not be parsed; defaults stay active.
        bool Load(StringView path);
        bool Save() const;

        const json& Data() const { return _Data; }
        json&       Data()       { return _Data; }

        // Typed lookup with fallback, e.g. Value("/Window/Width", 1280)
        template<typename T>
        T Value(StringView pointer, T fallback) const {
            auto ptr = json::json_pointer(String(pointer));
            if (not _Data.contains(ptr)) return fallback;
            const auto& v = _Data.at(ptr);
            if constexpr (requires { v.template get<T>(); }) {
                if (v.is_null()) return fallback;
                return v.template get<T>();
            }
            return fallback;
        }

        static const json& Defaults();
    };
};
