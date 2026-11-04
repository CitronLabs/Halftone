module;
#include <string>
#include <utility>
#include <vector>

module com.citronlabs.Halftone.Scene;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Drawing;
import com.citronlabs.Halftone.Animation;

using namespace Halftone::Scene;
using namespace Halftone::Core;
namespace Draw = Halftone::Drawing;

// Small helpers so every field is one line in each direction.
namespace
{
    json V2(Vec2 v) { return json::array({ v.x, v.y }); }
    json V3(Vec3 v) { return json::array({ v.x, v.y, v.z }); }
    json V4(Vec4 v) { return json::array({ v.x, v.y, v.z, v.w }); }

    Vec2 ToV2(const json& j, Vec2 d) { return j.is_array() && j.size() == 2 ? Vec2 { j[0].get<f32>(), j[1].get<f32>() } : d; }
    Vec3 ToV3(const json& j, Vec3 d) { return j.is_array() && j.size() == 3 ? Vec3 { j[0].get<f32>(), j[1].get<f32>(), j[2].get<f32>() } : d; }
    Vec4 ToV4(const json& j, Vec4 d) { return j.is_array() && j.size() == 4 ? Vec4 { j[0].get<f32>(), j[1].get<f32>(), j[2].get<f32>(), j[3].get<f32>() } : d; }

    template<typename T>
    T Get(const json& j, const char* key, T fallback) {
        auto it = j.find(key);
        if (it == j.end() || it->is_null()) return fallback;
        return it->template get<T>();
    }

    json TransformToJson(const Transform& t) {
        return { { "Position", V3(t.Position) }, { "Rotation", V3(t.Rotation) }, { "Scale", V3(t.Scale) } };
    }
    Transform TransformFromJson(const json& j) {
        Transform t;
        if (!j.is_object()) return t;
        t.Position = ToV3(j.value("Position", json()), t.Position);
        t.Rotation = ToV3(j.value("Rotation", json()), t.Rotation);
        t.Scale    = ToV3(j.value("Scale",    json()), t.Scale);
        return t;
    }

    json StrokeToJson(const Draw::Stroke& s) {
        json pts = json::array();
        for (auto& p : s.Points) pts.push_back({ p.Position.x, p.Position.y, p.Pressure, p.TiltX, p.TiltY, p.Time });
        return { { "Color", V4(s.Color) }, { "Width", s.Width }, { "Brush", s.Brush }, { "Points", pts } };
    }
    Draw::Stroke StrokeFromJson(const json& j) {
        Draw::Stroke s;
        s.Color = ToV4(j.value("Color", json()), s.Color);
        s.Width = Get(j, "Width", s.Width);
        s.Brush = Get<ID>(j, "Brush", NullID);
        for (auto& p : j.value("Points", json::array()))
            if (p.is_array() && p.size() == 6)
                s.Points.push_back({ { p[0].get<f32>(), p[1].get<f32>() }, p[2].get<f32>(), p[3].get<f32>(), p[4].get<f32>(), p[5].get<f64>() });
        return s;
    }

    json LayerToJson(const Draw::Layer& l) {
        json drawings = json::array();
        for (auto& d : l.Drawings) {
            json strokes = json::array();
            for (auto& s : d.Strokes) strokes.push_back(StrokeToJson(s));
            drawings.push_back({ { "Id", d.Id }, { "Name", d.Name }, { "Strokes", strokes } });
        }
        json exposures = json::array();
        for (auto& e : l.Exposures.Exposures()) exposures.push_back({ e.Start, e.Drawing });
        const auto& cad = l.Exposures.GetCadence();
        return {
            { "Id", l.Id }, { "Name", l.Name }, { "Kind", int(l.Kind) }, { "Blend", int(l.Blend) },
            { "Opacity", l.Opacity }, { "Visible", l.Visible }, { "Locked", l.Locked },
            { "Depth", TransformToJson(l.DepthTransform) },
            { "Cadence", { cad.Step, cad.Offset } },
            { "Exposures", exposures }, { "Drawings", drawings },
        };
    }
    Draw::Layer LayerFromJson(const json& j, Draw::CanvasDocument& doc) {
        Draw::Layer l;
        l.Id      = Get<ID>(j, "Id", doc.NewId());
        l.Name    = Get<String>(j, "Name", l.Name);
        l.Kind    = Draw::LayerKind(Get<int>(j, "Kind", 0));
        l.Blend   = Draw::BlendMode(Get<int>(j, "Blend", 0));
        l.Opacity = Get(j, "Opacity", l.Opacity);
        l.Visible = Get(j, "Visible", l.Visible);
        l.Locked  = Get(j, "Locked", l.Locked);
        l.DepthTransform = TransformFromJson(j.value("Depth", json()));
        doc.ReserveId(l.Id);
        if (auto c = j.value("Cadence", json()); c.is_array() && c.size() == 2)
            l.Exposures.GetCadence() = { c[0].get<u32>(), c[1].get<Frame>() };
        for (auto& d : j.value("Drawings", json::array())) {
            Draw::Drawing drawing;
            drawing.Id   = Get<ID>(d, "Id", doc.NewId());
            drawing.Name = Get<String>(d, "Name", "");
            doc.ReserveId(drawing.Id);
            for (auto& s : d.value("Strokes", json::array())) drawing.Strokes.push_back(StrokeFromJson(s));
            l.Drawings.push_back(std::move(drawing));
        }
        for (auto& e : j.value("Exposures", json::array()))
            if (e.is_array() && e.size() == 2) l.Exposures.Expose(e[0].get<Frame>(), e[1].get<ID>());
        return l;
    }

    json MaterialToJson(const Material& m) {
        json paint = json::array();
        for (auto& p : m.PaintLayers)
            paint.push_back({ { "Id", p.Id }, { "Name", p.Name }, { "Blend", int(p.Blend) }, { "Opacity", p.Opacity }, { "Visible", p.Visible } });
        return {
            { "Name", m.Name }, { "BaseColor", V4(m.BaseColor) }, { "BaseColorTexture", m.BaseColorTexture },
            { "ShadowTint", V4(m.ShadowTint) }, { "ToonBands", m.ToonBands },
            { "Outline", m.Outline }, { "OutlineWidth", m.OutlineWidth }, { "PaintLayers", paint },
        };
    }
    Material MaterialFromJson(const json& j) {
        Material m;
        if (!j.is_object()) return m;
        m.Name             = Get<String>(j, "Name", m.Name);
        m.BaseColor        = ToV4(j.value("BaseColor", json()), m.BaseColor);
        m.BaseColorTexture = Get<String>(j, "BaseColorTexture", "");
        m.ShadowTint       = ToV4(j.value("ShadowTint", json()), m.ShadowTint);
        m.ToonBands        = Get(j, "ToonBands", m.ToonBands);
        m.Outline          = Get(j, "Outline", m.Outline);
        m.OutlineWidth     = Get(j, "OutlineWidth", m.OutlineWidth);
        for (auto& p : j.value("PaintLayers", json::array())) {
            TexturePaintLayer layer;
            layer.Id      = Get<ID>(p, "Id", NullID);
            layer.Name    = Get<String>(p, "Name", layer.Name);
            layer.Blend   = Draw::BlendMode(Get<int>(p, "Blend", 0));
            layer.Opacity = Get(p, "Opacity", layer.Opacity);
            layer.Visible = Get(p, "Visible", layer.Visible);
            m.PaintLayers.push_back(std::move(layer));
        }
        return m;
    }
}

json Halftone::Scene::SceneToJson(const Scene& scene)
{
    json nodes = json::array();
    for (const auto& n : scene.Nodes())
    {
        json j = {
            { "Id", n->Id() }, { "Kind", NodeKindName(n->Kind()) }, { "Name", n->Name() },
            { "Parent", n->Parent }, { "Visible", n->Visible }, { "Transform", TransformToJson(n->LocalTransform) },
        };

        switch (n->Kind())
        {
            case NodeKind::Camera: {
                auto& c = static_cast<const Camera&>(*n);
                j["Projection"] = int(c.Mode); j["FieldOfView"] = c.FieldOfView; j["OrthoHeight"] = c.OrthoHeight;
                j["NearClip"] = c.NearClip;    j["FarClip"] = c.FarClip;
                break;
            }
            case NodeKind::Light: {
                auto& l = static_cast<const Light&>(*n);
                j["Type"] = int(l.Type); j["Color"] = V3(l.Color); j["Intensity"] = l.Intensity;
                j["ToonBands"] = l.ToonBands; j["AffectsCanvases"] = l.AffectsCanvases; j["CastsShadows"] = l.CastsShadows;
                break;
            }
            case NodeKind::Canvas: {
                auto& c = static_cast<const Canvas&>(*n);
                json layers = json::array();
                for (auto& layer : c.Document.Layers()) layers.push_back(LayerToJson(layer));
                j["Role"] = int(c.Role); j["Facing"] = int(c.Facing); j["Mapping"] = int(c.Mapping);
                j["WorldSize"] = V2(c.WorldSize); j["Anchor"] = V2(c.Anchor); j["ProjectionCamera"] = c.ProjectionCamera;
                j["ReceiveLighting"] = c.ReceiveLighting; j["CastShadows"] = c.CastShadows;
                j["Document"] = { { "Size", V2(c.Document.Size) }, { "ActiveLayer", c.Document.ActiveLayer }, { "Layers", layers } };
                break;
            }
            case NodeKind::Model: {
                auto& m = static_cast<const Model&>(*n);
                j["Mesh"] = m.MeshAsset; j["Material"] = MaterialToJson(m.Surface);
                j["CastShadows"] = m.CastShadows; j["ReceiveShadows"] = m.ReceiveShadows;
                break;
            }
            case NodeKind::Group: break;
        }
        nodes.push_back(std::move(j));
    }

    const Camera* active = scene.ActiveCamera();
    return {
        { "Format", SceneFormatVersion },
        { "Name", scene.Name() },
        { "ActiveCamera", active ? active->Id() : NullID },
        { "Nodes", nodes },
    };
}

bool Halftone::Scene::SceneFromJson(const json& data, Scene& out, String* error)
{
    if (!data.is_object()) { if (error) *error = "scene data is not an object"; return false; }

    out.Clear();
    out.SetName(Get<String>(data, "Name", out.Name()));

    for (const auto& j : data.value("Nodes", json::array()))
    {
        const ID     id   = Get<ID>(j, "Id", NullID);
        const String kind = Get<String>(j, "Kind", "");
        const String name = Get<String>(j, "Name", "Node");
        if (id == NullID) continue;

        Node* node = nullptr;
        if (kind == "Camera") {
            auto& c = out.CreateWithId<Camera>(id, name);
            c.Mode        = Projection(Get<int>(j, "Projection", 0));
            c.FieldOfView = Get(j, "FieldOfView", c.FieldOfView);
            c.OrthoHeight = Get(j, "OrthoHeight", c.OrthoHeight);
            c.NearClip    = Get(j, "NearClip", c.NearClip);
            c.FarClip     = Get(j, "FarClip", c.FarClip);
            node = &c;
        } else if (kind == "Light") {
            auto& l = out.CreateWithId<Light>(id, name);
            l.Type            = LightKind(Get<int>(j, "Type", 0));
            l.Color           = ToV3(j.value("Color", json()), l.Color);
            l.Intensity       = Get(j, "Intensity", l.Intensity);
            l.ToonBands       = Get(j, "ToonBands", l.ToonBands);
            l.AffectsCanvases = Get(j, "AffectsCanvases", l.AffectsCanvases);
            l.CastsShadows    = Get(j, "CastsShadows", l.CastsShadows);
            node = &l;
        } else if (kind == "Canvas") {
            auto& c = out.CreateWithId<Canvas>(id, name);
            c.Role             = CanvasRole(Get<int>(j, "Role", 3));
            c.Facing           = CardFacing(Get<int>(j, "Facing", 0));
            c.Mapping          = CanvasMapping(Get<int>(j, "Mapping", 0));
            c.WorldSize        = ToV2(j.value("WorldSize", json()), c.WorldSize);
            c.Anchor           = ToV2(j.value("Anchor", json()), c.Anchor);
            c.ProjectionCamera = Get<ID>(j, "ProjectionCamera", NullID);
            c.ReceiveLighting  = Get(j, "ReceiveLighting", c.ReceiveLighting);
            c.CastShadows      = Get(j, "CastShadows", c.CastShadows);
            const json doc = j.value("Document", json::object());
            c.Document.Size = ToV2(doc.value("Size", json()), c.Document.Size);
            for (auto& layer : doc.value("Layers", json::array()))
                c.Document.Layers().push_back(LayerFromJson(layer, c.Document));
            c.Document.ActiveLayer = Get<ID>(doc, "ActiveLayer", NullID);
            node = &c;
        } else if (kind == "Model") {
            auto& m = out.CreateWithId<Model>(id, name);
            m.MeshAsset      = Get<String>(j, "Mesh", "");
            m.Surface        = MaterialFromJson(j.value("Material", json()));
            m.CastShadows    = Get(j, "CastShadows", m.CastShadows);
            m.ReceiveShadows = Get(j, "ReceiveShadows", m.ReceiveShadows);
            node = &m;
        } else {
            node = &out.CreateWithId<Group>(id, name);
        }

        node->Parent         = Get<ID>(j, "Parent", NullID);
        node->Visible        = Get(j, "Visible", true);
        node->LocalTransform = TransformFromJson(j.value("Transform", json()));
    }

    out.SetActiveCamera(Get<ID>(data, "ActiveCamera", NullID));
    return true;
}
