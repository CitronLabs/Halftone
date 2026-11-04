module;
#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <string>
#include <string_view>
#include <memory>
#include <optional>
#include <span>
#include <functional>
#include <unordered_map>
#include <utility>

export module com.citronlabs.Halftone.Core:Types;


export namespace Halftone::Core
{
    using u8    = uint8_t;
    using u16   = uint16_t;
    using u32   = uint32_t;
    using u64   = uint64_t;

    using i8    = int8_t;
    using i16   = int16_t;
    using i32   = int32_t;
    using i64   = int64_t;

    using f32   = float;
    using f64   = double;

    using usize = size_t;

    template<typename T>
    using List      = std::vector<T>;

    template<typename K, typename V>
    using Map       = std::unordered_map<K, V>;

    template<typename T>
    using Span      = std::span<T>;

    template<typename T>
    using Optional  = std::optional<T>;

    template<typename Signature>
    using Function  = std::function<Signature>;

    using String     = std::string;
    using StringView = std::string_view;

    // Ownership
    template<typename T>
    using Unique    = std::unique_ptr<T>;

    template<typename T>
    using Shared    = std::shared_ptr<T>;

    template<typename T, typename... Args>
    Unique<T> MakeUnique(Args&&... args) { return std::make_unique<T>(std::forward<Args>(args)...); }

    template<typename T, typename... Args>
    Shared<T> MakeShared(Args&&... args) { return std::make_shared<T>(std::forward<Args>(args)...); }

    // Stable identifier for any document object (scene nodes, drawings, ...).
    // 0 is always "none".
    using ID = u64;
    inline constexpr ID NullID = 0;
}
