module;
#include <string>
#include <vector>
#include <functional>

export module com.citronlabs.Halftone.Editor:SplashScreen;

import com.citronlabs.Halftone.Core;
import :Context;
import :Screen;

export namespace Halftone::Editor
{
    using namespace Halftone::Core;

    // One unit of startup work. Returns false on failure (the splash then
    // shows the error and a Quit button).
    struct LoadStep {
        String              Label;
        Function<bool()>    Run;
    };

    // Shown while startup work runs, one step per frame so progress is
    // visible. Add steps from the composition root.
    class SplashScreen final : public Screen {
    private:
        List<LoadStep>  _Steps;
        usize           _Next     = 0;
        f64             _Elapsed  = 0.0;
        bool            _Failed   = false;
        String          _FailedStep;

    public:
        static constexpr f64 MinimumSeconds = 1.2;   // long enough to read, short enough not to annoy

        void AddStep(String label, Function<bool()> run) { _Steps.push_back({ std::move(label), std::move(run) }); }

        bool Finished() const { return not _Failed and _Next >= _Steps.size() and _Elapsed >= MinimumSeconds; }
        bool Failed()   const { return _Failed; }

        ScreenKind  Kind() const override { return ScreenKind::Splash; }
        void        Update(EditorContext& ctx, f64 dt) override;
        void        Draw(EditorContext& ctx) override;
    };
}
