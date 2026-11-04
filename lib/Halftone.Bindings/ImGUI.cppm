module;
#include <__Vendor/Vulkan/vulkan/vulkan.hpp>

export module com.citronlabs.Halftone.Bindings:ImGUI;

import com.citronlabs.Halftone.Core;
import :Vulkan;

export namespace Halftone::Bindings
{
    using namespace Halftone::Core;

    // Opaque handle to a GPU image registered for drawing in ImGui
    // (ImGui::Image(ImTextureID(handle), ...)). 0 = none.
    using UITexture = u64;

    // Dear ImGui context + its SDL3 platform and Vulkan renderer backends.
    // Panels call ImGui:: directly (include <__Vendor/ImGui/imgui.h> in the
    // implementation file); this binding only owns lifetime and frame plumbing.
    class ImGUI {
    private:
        bool _Initialized = false;
        f32  _Scale       = 1.0f;

        ImGUI()  = default;
        ~ImGUI();
    public:

        static ImGUI& GetBinding() {
            static ImGUI instance;
            return instance;
        }

        // Delete copy constructor and copy assignment operator
        ImGUI(const ImGUI&)             = delete;
        ImGUI& operator=(const ImGUI&)  = delete;

        // Delete move constructor and move assignment operator
        ImGUI(ImGUI&&)                  = delete;
        ImGUI& operator=(ImGUI&&)       = delete;

        // Requires the SDL and Vulkan bindings to be initialized.
        bool Initialize(f32 uiScale = 1.0f);
        void Shutdown();

        void NewFrame();                        // before any ImGui:: calls this frame
        void EndFrame();                        // ImGui::Render(); call before Vulkan::BeginFrame
        void Draw(vk::CommandBuffer cmd);       // inside the swapchain pass
        void OnSwapchainRebuilt();

        UITexture AddTexture(vk::Sampler sampler, vk::ImageView view, vk::ImageLayout layout);
        void      RemoveTexture(UITexture texture);
    };
}
