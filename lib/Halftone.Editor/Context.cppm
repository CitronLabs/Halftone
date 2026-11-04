module;
#include <memory>
#include <vector>

export module com.citronlabs.Halftone.Editor:Context;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Project;
import com.citronlabs.Halftone.Scene;
import com.citronlabs.Halftone.Animation;
import com.citronlabs.Halftone.Drawing;
import com.citronlabs.Halftone.Graphics;

export namespace Halftone::Editor
{
    using namespace Halftone::Core;

    // Which part of a scene the artist is "honed into".
    enum class EditorModeKind : u8 {
        Scene,      // 3D layout of the whole scene
        Canvas2D,   // one canvas as a 2D animation workspace (layers, brushes)
        Canvas3D,   // one canvas with its layers placed in 3D space
        Model,      // one model: material + texture painting
    };
    inline constexpr u32 EditorModeCount = 4;
    const char* EditorModeName(EditorModeKind mode);
    Halftone::Graphics::ViewKind ViewKindFor(EditorModeKind mode);

    enum class SceneTool : u8 { Select, Move, Rotate, Scale };
    enum class ModelTool : u8 { Select, TexturePaint };
    const char* SceneToolName(SceneTool tool);
    const char* ModelToolName(ModelTool tool);

    // A scene file that is loaded and being edited.
    struct OpenScene {
        Halftone::Project::ProjectItem* Item = nullptr;
        Halftone::Scene::Scene          Document;
        bool                            Dirty = false;
        u32                             DefaultCadence = 2;    // from the scene's effective "DefaultCadence" config
    };

    // Orbit / pan state behind the viewport camera. One per mode, so going
    // back to the scene restores the view the artist left.
    struct ViewNavigator {
        Vec3    Target      = { 0.0f, 1.5f, -2.0f };
        f32     Yaw         = 0.55f;
        f32     Pitch       = 0.32f;
        f32     Distance    = 17.0f;
        Vec2    PageCenter  = { 960.0f, 540.0f };   // CanvasFlat: page px at viewport centre
        f32     PageHeight  = 1300.0f;              // CanvasFlat: page px visible vertically

        Halftone::Graphics::ViewCamera Camera(Halftone::Graphics::ViewKind kind) const;
        void Orbit(f32 dx, f32 dy);
        void Pan(f32 dx, f32 dy, f32 viewportHeight, bool flat);
        void Zoom(f32 steps, bool flat);
        void FrameCanvas(Vec2 pageSize);            // fit a page in CanvasFlat
    };

    // Navigation is requested during the frame and applied by the Shell at
    // the end of it, so no screen is torn down while it is being drawn.
    struct Navigation {
        Halftone::Project::ProjectItem* OpenScene  = nullptr;
        bool                            CloseScene = false;
        bool                            SaveScene  = false;
        bool                            ChangeMode = false;
        EditorModeKind                  Mode       = EditorModeKind::Scene;
        ID                              Focus      = NullID;
    };

    // Shared state for every screen and panel. Owned by the composition root
    // (src/App/Main.cpp); panels must not cache pointers beyond a frame.
    struct EditorContext {
        Halftone::Project::ProjectLibrary&  Library;
        Halftone::Graphics::Renderer&       Renderer;
        Halftone::Animation::Timeline&      Timeline;

        Unique<OpenScene>                   Open;               // null outside the scene editor

        EditorModeKind                      Mode        = EditorModeKind::Scene;
        ID                                  Focus       = NullID;   // canvas/model being edited
        ID                                  Selection   = NullID;

        SceneTool                           ActiveSceneTool   = SceneTool::Select;
        Halftone::Drawing::DrawingMode      ActiveDrawingMode = Halftone::Drawing::DrawingMode::Brush;
        ModelTool                           ActiveModelTool   = ModelTool::Select;
        Halftone::Drawing::BrushPreset      Brush;

        // Interactive drawing tools, one per DrawingMode (registered by the
        // composition root). Empty in the backbone: input reaches
        // Viewport -> ToolFor(mode) and stops there until tools exist.
        List<Unique<Halftone::Drawing::Tool>> DrawingTools;

        ViewNavigator                       Navigators[EditorModeCount];
        Halftone::Graphics::RenderView      View;

        Navigation                          Nav;
        bool                                QuitRequested = false;

        Halftone::Scene::Scene*     Document()          { return Open ? &Open->Document : nullptr; }
        Halftone::Scene::Canvas*    FocusedCanvas();
        Halftone::Scene::Model*     FocusedModel();
        ViewNavigator&              Navigator()         { return Navigators[u32(Mode)]; }
        Halftone::Drawing::Tool*    ToolFor(Halftone::Drawing::DrawingMode mode);
        void                        MarkDirty()         { if (Open) Open->Dirty = true; }

        void RequestOpenScene(Halftone::Project::ProjectItem& scene) { Nav.OpenScene = &scene; }
        void RequestCloseScene()    { Nav.CloseScene = true; }
        void RequestSave()          { Nav.SaveScene = true; }
        void RequestMode(EditorModeKind mode, ID focus = NullID) { Nav.ChangeMode = true; Nav.Mode = mode; Nav.Focus = focus; }
    };

    // Menu items for adding scene elements (used by the Add menu and the
    // Outliner's "+" button). Must be called inside an open ImGui menu/popup.
    void AddNodeMenuItems(EditorContext& ctx);

    // Applies scene-scoped config defaults (e.g. DefaultCadence) to new content.
    void ApplySceneDefaults(EditorContext& ctx, Halftone::Drawing::CanvasDocument& doc);
    void ApplySceneDefaults(EditorContext& ctx, Halftone::Drawing::Layer& layer);

    // ImGui helper for editing a Core::String in place.
    bool InputString(const char* label, String& value, bool multiline = false);
}
