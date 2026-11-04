module;
#include <string>
#include <memory>
#include <utility>
#include <vector>

export module com.citronlabs.Halftone.Scene:Scene;

import com.citronlabs.Halftone.Core;
import :Node;
import :Camera;

export namespace Halftone::Scene
{
    using namespace Halftone::Core;

    // Flat node store. Hierarchy is expressed through Node::Parent so the
    // container stays trivially serialisable.
    class Scene {
    private:
        String              _Name;
        List<Unique<Node>>  _Nodes;
        ID                  _NextId       = 1;
        ID                  _ActiveCamera = NullID;

    public:
        explicit Scene(String name = "Untitled") : _Name(std::move(name)) {}

        template<typename T>
        T& Create(String name) { return CreateWithId<T>(_NextId, std::move(name)); }

        // Used when loading: keeps the stored id and bumps the counter past it.
        template<typename T>
        T& CreateWithId(ID id, String name) {
            if (id >= _NextId) _NextId = id + 1;
            auto node = MakeUnique<T>(id, std::move(name));
            T&   ref  = *node;
            _Nodes.push_back(std::move(node));
            return ref;
        }

        void Clear() { _Nodes.clear(); _NextId = 1; _ActiveCamera = NullID; }

        bool        Remove(ID id);
        Node*       Find(ID id);
        const Node* Find(ID id) const;

        template<typename T>
        T* FindAs(ID id) {
            Node* node = Find(id);
            return node and node->Kind() == T::StaticKind ? static_cast<T*>(node) : nullptr;
        }

        template<typename T>
        const T* FindAs(ID id) const {
            const Node* node = Find(id);
            return node and node->Kind() == T::StaticKind ? static_cast<const T*>(node) : nullptr;
        }

        const List<Unique<Node>>& Nodes() const { return _Nodes; }

        Camera*         ActiveCamera()                { return FindAs<Camera>(_ActiveCamera); }
        const Camera*   ActiveCamera() const          { return FindAs<Camera>(_ActiveCamera); }
        void            SetActiveCamera(ID camera)    { _ActiveCamera = camera; }

        const String&   Name() const                  { return _Name; }
        void            SetName(String name)          { _Name = std::move(name); }

        // What a freshly created scene file contains: just the shot camera,
        // so artists start from an empty 3D view.
        static Scene CreateNew(String name);
    };
}
