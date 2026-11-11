module;
#include <string>
#include <utility>

export module com.citronlabs.Halftone.Scene:Node;

import com.citronlabs.Halftone.Core;

export namespace Halftone::Scene
{
    using namespace Halftone::Core;

    enum class NodeKind : u8 {
        Group,
        Camera,
        Light,
        Canvas,     // 2D drawing surface with a layer stack, placed in 3D
        Model,      // 3D mesh with a stylised material
    };

    const char* NodeKindName(NodeKind kind);

    // Base of everything placed in a scene. Concrete node types add plain
    // data members; behaviour lives in the systems that consume them
    // (render engines, animation, editor tools).
    class Node {
    private:
        NodeKind    _Kind;
        ID          _Id;
        String      _Name;

    public:
        Transform   LocalTransform;
        ID          Parent  = NullID;
        bool        Visible = true;

        Node(NodeKind kind, ID id, String name) : _Kind(kind), _Id(id), _Name(std::move(name)) {}
        virtual ~Node() = default;

        Node(const Node&)            = delete;
        Node& operator=(const Node&) = delete;

        NodeKind        Kind() const { return _Kind; }
        ID              Id()   const { return _Id; }
        const String&   Name() const { return _Name; }
        void            SetName(String name) { _Name = std::move(name); }
    };

    class Group : public Node {
    public:
        static constexpr NodeKind StaticKind = NodeKind::Group;
        Group(ID id, String name) : Node(StaticKind, id, std::move(name)) {}
    };
}
