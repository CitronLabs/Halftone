module;
#include <memory>

export module com.citronlabs.Halftone.Editor:Shell;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Project;
import com.citronlabs.Halftone.Scene;
import com.citronlabs.Halftone.Graphics;
import :Context;
import :Screen;
import :SplashScreen;
import :ProjectBrowserScreen;
import :SceneEditorScreen;

export namespace Halftone::Editor
{
    using namespace Halftone::Core;

    // Owns the screens and applies navigation (open/close scene, mode
    // changes) between frames. The composition root calls Frame() once per
    // frame between ImGUI::NewFrame and ImGUI::EndFrame.
    class Shell {
    private:
        EditorContext&                  _Ctx;
        Unique<SplashScreen>            _Splash;
        Unique<ProjectBrowserScreen>    _Browser;
        Unique<SceneEditorScreen>       _Editor;
        Screen*                         _Current = nullptr;

        void SwitchTo(Screen& screen);
        void ApplyNavigation();
        void OpenScene(Halftone::Project::ProjectItem& item);
        void EnterMode(EditorModeKind mode, ID focus);

    public:
        explicit Shell(EditorContext& ctx);
        ~Shell();

        SplashScreen&   Splash()  { return *_Splash; }

        void Frame(f64 dt);

        bool                                ViewportActive() const;
        const Halftone::Graphics::RenderView& View()  const { return _Ctx.View; }
        Halftone::Scene::Scene*             Document()      { return _Ctx.Document(); }
        bool                                QuitRequested() const { return _Ctx.QuitRequested; }
    };
}
