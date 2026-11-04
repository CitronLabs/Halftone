module;
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

module com.citronlabs.Halftone.Scene;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Drawing;

using namespace Halftone::Scene;
using namespace Halftone::Core;

const char* Halftone::Scene::NodeKindName(NodeKind kind)
{
    switch (kind) {
        case NodeKind::Group:  return "Group";
        case NodeKind::Camera: return "Camera";
        case NodeKind::Light:  return "Light";
        case NodeKind::Canvas: return "Canvas";
        case NodeKind::Model:  return "Model";
    }
    return "Node";
}

const char* Halftone::Scene::CanvasRoleName(CanvasRole role)
{
    switch (role) {
        case CanvasRole::Character:   return "Character";
        case CanvasRole::Environment: return "Environment";
        case CanvasRole::Prop:        return "Prop";
        case CanvasRole::Free:        return "Free";
    }
    return "?";
}

void Canvas::ApplyRoleDefaults(CanvasRole role)
{
    using Halftone::Drawing::LayerKind;
    Role = role;
    switch (role) {
        case CanvasRole::Character:
            Facing    = CardFacing::FaceCameraUpright;
            WorldSize = { 1.2f, 2.0f };
            Anchor    = { 0.5f, 0.0f };
            Document.Size = { 1080.0f, 1800.0f };
            Document.AddLayer("Color", LayerKind::Raster);
            Document.AddLayer("Line",  LayerKind::Vector);
            break;
        case CanvasRole::Environment:
            Facing    = CardFacing::Fixed;
            Mapping   = CanvasMapping::Card;
            WorldSize = { 16.0f, 9.0f };
            ReceiveLighting = false;
            CastShadows     = false;
            Document.Size = { 3840.0f, 2160.0f };
            Document.AddLayer("Far",  LayerKind::Raster).DepthTransform.Position.z = -6.0f;
            Document.AddLayer("Mid",  LayerKind::Raster).DepthTransform.Position.z = -3.0f;
            Document.AddLayer("Near", LayerKind::Raster);
            break;
        case CanvasRole::Prop:
            Facing    = CardFacing::FaceCamera;
            WorldSize = { 1.0f, 1.0f };
            Document.Size = { 1024.0f, 1024.0f };
            Document.AddLayer("Art", LayerKind::Raster);
            break;
        case CanvasRole::Free:
            Document.AddLayer("Layer 1", LayerKind::Raster);
            break;
    }
}

bool Scene::Remove(ID id)
{
    auto it = std::find_if(_Nodes.begin(), _Nodes.end(), [&](const Unique<Node>& n) { return n->Id() == id; });
    if (it == _Nodes.end()) return false;

    for (auto& node : _Nodes) if (node->Parent == id) node->Parent = NullID;
    if (_ActiveCamera == id) _ActiveCamera = NullID;

    _Nodes.erase(it);
    return true;
}

Node* Scene::Find(ID id)
{
    for (auto& node : _Nodes) if (node->Id() == id) return node.get();
    return nullptr;
}

const Node* Scene::Find(ID id) const
{
    for (auto& node : _Nodes) if (node->Id() == id) return node.get();
    return nullptr;
}

Scene Scene::CreateNew(String name)
{
    Scene scene(std::move(name));

    auto& camera = scene.Create<Camera>("Shot Camera");
    camera.LocalTransform.Position = { 0.0f, 1.6f, 8.0f };
    scene.SetActiveCamera(camera.Id());

    return scene;
}
