module;
#include <string>
#include <vector>

export module com.citronlabs.Halftone.Graphics.Drawing:Canvas;

import com.citronlabs.Halftone.Core;
import :Layer;

export namespace Halftone::Drawing
{
    using namespace Halftone::Core;

    // The editable contents of a canvas: page size plus an ordered layer
    // stack (index 0 = bottom). Owned by Scene::Canvas.
    class CanvasDocument {
    private:
        List<Layer>     _Layers;
        ID              _NextId = 1;

    public:
        Vec2            Size        = { 1920.0f, 1080.0f };     // page size in pixels
        ID              ActiveLayer = NullID;

        Layer&  AddLayer(String name, LayerKind kind);          // placed above the active layer
        bool    RemoveLayer(ID id);
        bool    MoveLayer(ID id, i32 delta);                    // +1 = up the stack
        Layer*  FindLayer(ID id);
        Layer*  Active()                    { return FindLayer(ActiveLayer); }

        List<Layer>&        Layers()        { return _Layers; }
        const List<Layer>&  Layers() const  { return _Layers; }

        ID      NewId()                     { return _NextId++; }
        void    ReserveId(ID id)            { if (id >= _NextId) _NextId = id + 1; }   // used when loading
    };
}
