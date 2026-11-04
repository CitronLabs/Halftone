module;
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

module com.citronlabs.Halftone.Drawing;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Animation;

using namespace Halftone::Drawing;
using namespace Halftone::Core;

const char* Halftone::Drawing::BlendModeName(BlendMode mode)
{
    switch (mode) {
        case BlendMode::Normal:    return "Normal";
        case BlendMode::Multiply:  return "Multiply";
        case BlendMode::Screen:    return "Screen";
        case BlendMode::Overlay:   return "Overlay";
        case BlendMode::Add:       return "Add";
        case BlendMode::Darken:    return "Darken";
        case BlendMode::Lighten:   return "Lighten";
        case BlendMode::SoftLight: return "Soft Light";
    }
    return "?";
}

const char* Halftone::Drawing::LayerKindName(LayerKind kind)
{
    return kind == LayerKind::Raster ? "Raster" : "Vector";
}

const char* Halftone::Drawing::DrawingModeName(DrawingMode mode)
{
    switch (mode) {
        case DrawingMode::Brush:     return "Brush";
        case DrawingMode::Pencil:    return "Pencil";
        case DrawingMode::VectorPen: return "Vector Pen";
        case DrawingMode::Eraser:    return "Eraser";
        case DrawingMode::Fill:      return "Fill";
        case DrawingMode::Lasso:     return "Lasso";
        case DrawingMode::Transform: return "Transform";
    }
    return "?";
}

// --- Layer --------------------------------------------------------------------

Halftone::Drawing::Drawing* Layer::FindDrawing(ID id)
{
    for (auto& d : Drawings) if (d.Id == id) return &d;
    return nullptr;
}

Halftone::Drawing::Drawing* Layer::DrawingAt(Core::Frame frame)
{
    return FindDrawing(Exposures.DrawingAt(frame));
}

// --- CanvasDocument -----------------------------------------------------------

Layer& CanvasDocument::AddLayer(String name, LayerKind kind)
{
    Layer layer;
    layer.Id   = NewId();
    layer.Name = std::move(name);
    layer.Kind = kind;

    // Every new layer starts with one blank drawing exposed from frame 1.
    Halftone::Drawing::Drawing first;
    first.Id   = NewId();
    first.Name = "1";
    layer.Exposures.Expose(1, first.Id);
    layer.Drawings.push_back(std::move(first));

    auto it = std::find_if(_Layers.begin(), _Layers.end(), [&](const Layer& l) { return l.Id == ActiveLayer; });
    auto at = it == _Layers.end() ? _Layers.end() : it + 1;
    auto inserted = _Layers.insert(at, std::move(layer));
    ActiveLayer = inserted->Id;
    return *inserted;
}

bool CanvasDocument::RemoveLayer(ID id)
{
    auto it = std::find_if(_Layers.begin(), _Layers.end(), [&](const Layer& l) { return l.Id == id; });
    if (it == _Layers.end()) return false;
    usize index = usize(it - _Layers.begin());
    _Layers.erase(it);
    if (ActiveLayer == id)
        ActiveLayer = _Layers.empty() ? NullID : _Layers[std::min(index, _Layers.size() - 1)].Id;
    return true;
}

bool CanvasDocument::MoveLayer(ID id, i32 delta)
{
    auto it = std::find_if(_Layers.begin(), _Layers.end(), [&](const Layer& l) { return l.Id == id; });
    if (it == _Layers.end()) return false;
    i64 from = i64(it - _Layers.begin());
    i64 to   = std::clamp<i64>(from + delta, 0, i64(_Layers.size()) - 1);
    if (from == to) return false;
    std::swap(_Layers[usize(from)], _Layers[usize(to)]);
    return true;
}

Layer* CanvasDocument::FindLayer(ID id)
{
    for (auto& l : _Layers) if (l.Id == id) return &l;
    return nullptr;
}
