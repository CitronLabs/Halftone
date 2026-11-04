module;
#include <__Vendor/SDL3/SDL.h>
#include <utility>

export module com.citronlabs.Halftone.Bindings:SDL;

import com.citronlabs.Halftone.Core;

export namespace Halftone::Bindings
{
    using namespace Halftone::Core;

    struct WindowDesc {
        const char* Title   = "Halftone";
        u32         Width   = 1600;
        u32         Height  = 900;
    };

    // Owns SDL initialisation, the main window and the OS event pump.
    // Other bindings (ImGUI) and future input code (pen/tablet for the brush
    // engine) observe raw events through AddEventHook rather than owning the
    // pump themselves.
    class SDL {
    public:
        using EventHook = Function<void(const SDL_Event&)>;

    private:
        SDL_Window*     _MainWindow     = nullptr;
        List<EventHook> _EventHooks;
        bool            _QuitRequested  = false;
        bool            _WindowResized  = false;

        SDL()  = default;
        ~SDL();
    public:

        static SDL& GetBinding() {
            static SDL instance;
            return instance;
        }

        // Delete copy constructor and copy assignment operator
        SDL(const SDL&)             = delete;
        SDL& operator=(const SDL&)  = delete;

        // Delete move constructor and move assignment operator
        SDL(SDL&&)                  = delete;
        SDL& operator=(SDL&&)       = delete;

        bool Initialize(const WindowDesc& desc);
        void Shutdown();

        // Drains the OS queue, dispatching to hooks. Returns false once the
        // user has asked to quit.
        bool PumpEvents();
        void AddEventHook(EventHook hook) { _EventHooks.push_back(std::move(hook)); }

        // Returns true once per resize (and clears the flag).
        bool ConsumeResized() { bool r = _WindowResized; _WindowResized = false; return r; }
        bool IsMinimized() const;
        void PixelSize(u32& width, u32& height) const;

        // Per-user writable directory, e.g. %APPDATA%/CitronLabs/Halftone/
        String PreferencesDirectory() const;

        SDL_Window* MainWindow() { return _MainWindow; };
    };
}
