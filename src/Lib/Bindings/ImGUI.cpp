module;
#include <__Vendor/Vulkan/vulkan/vulkan.hpp>
#include <__Vendor/SDL3/SDL.h>
#include <__Vendor/ImGui/imgui.h>
#include <__Vendor/ImGui/Backends/imgui_impl_sdl3.h>
#include <__Vendor/ImGui/Backends/imgui_impl_vulkan.h>

module com.citronlabs.Halftone.Bindings;

import com.citronlabs.Halftone.Core;


using namespace Halftone::Bindings;
using namespace Halftone::Core;

namespace
{
    void CheckVkResult(VkResult err)
    {
        if (err != VK_SUCCESS) Log::Error("ImGUI", "Vulkan backend error: {}", i32(err));
    }

    void ApplyHalftoneStyle(f32 scale)
    {
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding    = 4.0f;
        style.FrameRounding     = 3.0f;
        style.GrabRounding      = 3.0f;
        style.WindowBorderSize  = 1.0f;
        style.ScaleAllSizes(scale);
        style.FontScaleMain     = scale;
    }
}

ImGUI::~ImGUI() {
    Shutdown();
}

bool ImGUI::Initialize(f32 uiScale) {

    auto& sdl    = SDL::GetBinding();
    auto& vulkan = Vulkan::GetBinding();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io     = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename  = "imgui.ini";

    _Scale = uiScale;
    ApplyHalftoneStyle(_Scale);

    if (not ImGui_ImplSDL3_InitForVulkan(sdl.MainWindow())) {
        Log::Fatal("ImGUI", "ImGui_ImplSDL3_InitForVulkan failed");
        return false;
    }

    VkFormat colorFormat = static_cast<VkFormat>(vulkan.SwapchainFormat());

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.ApiVersion         = VK_API_VERSION_1_3;
    initInfo.Instance           = vulkan.Instance();
    initInfo.PhysicalDevice     = vulkan.PhysicalDevice();
    initInfo.Device             = vulkan.Device();
    initInfo.QueueFamily        = vulkan.QueueFamily();
    initInfo.Queue              = vulkan.Queue();
    initInfo.DescriptorPool     = vulkan.DescriptorPool();
    initInfo.MinImageCount      = vulkan.MinImageCount();
    initInfo.ImageCount         = vulkan.ImageCount();
    initInfo.UseDynamicRendering = true;
    initInfo.CheckVkResultFn    = &CheckVkResult;

    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat;
    initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    if (not ImGui_ImplVulkan_Init(&initInfo)) {
        Log::Fatal("ImGUI", "ImGui_ImplVulkan_Init failed");
        return false;
    }

    // Raw SDL events flow to ImGui through the SDL binding's hook list.
    sdl.AddEventHook([](const SDL_Event& event) { ImGui_ImplSDL3_ProcessEvent(&event); });

    _Initialized = true;
    Log::Info("ImGUI", "Dear ImGui {} ready", ImGui::GetVersion());
    return true;
}

void ImGUI::Shutdown() {
    if (not _Initialized) return;

    // Textures released through DeferDestroy still call back into ImGui.
    Vulkan::GetBinding().FlushDeferred();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    _Initialized = false;
}

void ImGUI::NewFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void ImGUI::EndFrame() {
    ImGui::Render();
}

void ImGUI::Draw(vk::CommandBuffer cmd) {
    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData) ImGui_ImplVulkan_RenderDrawData(drawData, cmd);
}

void ImGUI::OnSwapchainRebuilt() {
    ImGui_ImplVulkan_SetMinImageCount(Vulkan::GetBinding().MinImageCount());
}

UITexture ImGUI::AddTexture(vk::Sampler sampler, vk::ImageView view, vk::ImageLayout layout) {
    VkDescriptorSet set = ImGui_ImplVulkan_AddTexture(sampler, view, static_cast<VkImageLayout>(layout));
    return reinterpret_cast<UITexture>(set);
}

void ImGUI::RemoveTexture(UITexture texture) {
    if (_Initialized and texture) ImGui_ImplVulkan_RemoveTexture(reinterpret_cast<VkDescriptorSet>(texture));
}
