module;
#include <memory>

export module com.citronlabs.Halftone.App.Editor:Shell;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Project;
import com.citronlabs.Halftone.Scene;
import com.citronlabs.Halftone.Graphics;

import com.citronlabs.Halftone.App.UI;
import com.citronlabs.Halftone.App.Editor.Modes;
import com.citronlabs.Halftone.App.Editor.Panels;
import com.citronlabs.Halftone.App.Editor.Screens;


export namespace Halftone::App::Editor
{
    using namespace Halftone::Core;

    // Owns the screens and applies navigation (open/close scene, mode
    // changes) between frames. The composition root calls Frame() once per
    // frame between ImGUI::NewFrame and ImGUI::EndFrame.
    class Shell {
    private:
        Context&                        _Ctx;
        Unique<Screens::Splash>         _Splash;
        Unique<Screens::ProjectBrowser> _Browser;
        Unique<Screens::SceneEditor>    _Editor;
        Screen*                         _Current = nullptr;

        void SwitchTo(Screen& screen);
        void ApplyNavigation();
        void OpenScene(Halftone::Project::Item& item);
        void EnterMode(EditorModeKind mode, ID focus);

    public:
        explicit Shell(Context& ctx);
        ~Shell();

        SplashScreen&   Splash()  { return *_Splash; }

        void Frame(f64 dt);

        bool                                  ViewportActive() const;
        const Halftone::Graphics::RenderView& View()           const { return _Ctx.View; }
        Halftone::Scene::Scene*               Document()             { return _Ctx.Document(); }
        bool                                  QuitRequested()  const { return _Ctx.QuitRequested; }
    };
}
