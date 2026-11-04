module;
#include <__Vendor/SDL3/SDL.h>
#include <string>

module com.citronlabs.Halftone.Bindings;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App;


using namespace Halftone::Bindings;
using namespace Halftone::Core;

SDL::~SDL() {
    Shutdown();
}

bool SDL::Initialize(const WindowDesc& desc) {

    SDL_SetAppMetadata(Halftone::App::Info::Name, Halftone::App::Info::Version, "com.citronlabs.halftone");

    if (not SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        Log::Fatal("SDL", "SDL_Init failed: {}", SDL_GetError());
        return false;
    }

    _MainWindow = SDL_CreateWindow(
        desc.Title, int(desc.Width), int(desc.Height),
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );

    if (not _MainWindow) {
        Log::Fatal("SDL", "SDL_CreateWindow failed: {}", SDL_GetError());
        return false;
    }

    Log::Info("SDL", "Initialized SDL {}.{}.{} ({})",
        SDL_VERSIONNUM_MAJOR(SDL_GetVersion()),
        SDL_VERSIONNUM_MINOR(SDL_GetVersion()),
        SDL_VERSIONNUM_MICRO(SDL_GetVersion()),
        SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "?");
    return true;
}

void SDL::Shutdown() {

    _EventHooks.clear();

    if (_MainWindow) {
        SDL_DestroyWindow(_MainWindow);
        _MainWindow = nullptr;
        SDL_Quit();
    }
}

bool SDL::PumpEvents() {

    SDL_Event event;
    while (SDL_PollEvent(&event)) {

        for (auto& hook : _EventHooks) hook(event);

        switch (event.type) {
            case SDL_EVENT_QUIT:
                _QuitRequested = true;
                break;

            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                if (SDL_GetWindowFromID(event.window.windowID) == _MainWindow) _QuitRequested = true;
                break;

            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            case SDL_EVENT_WINDOW_RESIZED:
                _WindowResized = true;
                break;

            default: break;
        }
    }
    return not _QuitRequested;
}

bool SDL::IsMinimized() const {
    return _MainWindow and (SDL_GetWindowFlags(_MainWindow) & SDL_WINDOW_MINIMIZED);
}

void SDL::PixelSize(u32& width, u32& height) const {
    int w = 0, h = 0;
    if (_MainWindow) SDL_GetWindowSizeInPixels(_MainWindow, &w, &h);
    width  = u32(w);
    height = u32(h);
}

String SDL::PreferencesDirectory() const {
    char* path = SDL_GetPrefPath(Halftone::App::Info::Organization, Halftone::App::Info::Name);
    if (not path) return String(".");
    String result(path);
    SDL_free(path);
    return result;
}
