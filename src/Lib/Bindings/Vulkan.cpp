module;
#include <__Vendor/Vulkan/vulkan/vulkan.hpp>
#include <__Vendor/SDL3/SDL.h>
#include <__Vendor/SDL3/SDL_vulkan.h>
#include <algorithm>
#include <cstring>
#include <utility>
#include <array>
#include <functional>

module com.citronlabs.Halftone.Bindings;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.App;


using namespace Halftone::Bindings;
using namespace Halftone::Core;
using namespace Halftone::App;

namespace
{
    constexpr const char* ValidationLayer = "VK_LAYER_KHRONOS_validation";

    VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
        VkDebugUtilsMessageTypeFlagsEXT             /*type*/,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void*                                       /*user*/)
    {
        if      (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)   Log::Error("Vulkan", "{}", data->pMessage);
        else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) Log::Warn ("Vulkan", "{}", data->pMessage);
        else                                                                 Log::Trace("Vulkan", "{}", data->pMessage);
        return VK_FALSE;
    }

    bool HasLayer(const char* name)
    {
        auto [result, layers] = vk::enumerateInstanceLayerProperties();
        if (result != vk::Result::eSuccess) return false;
        return std::any_of(layers.begin(), layers.end(),
            [&](const vk::LayerProperties& l) { return std::strcmp(l.layerName, name) == 0; });
    }
}

Vulkan::~Vulkan()
{
    Shutdown();
}

bool Vulkan::Initialize(const VulkanDesc& desc)
{
    _Desc   = desc;
    _Window = SDL::GetBinding().MainWindow();
    if (not _Window) {
        Log::Fatal("Vulkan", "SDL binding must be initialized before Vulkan");
        return false;
    }

    if (not CreateInstance()) return false;

    if (not SDL_Vulkan_CreateSurface(_Window, _Instance, nullptr, reinterpret_cast<VkSurfaceKHR*>(&_Surface))) {
        Log::Fatal("Vulkan", "SDL_Vulkan_CreateSurface failed: {}", SDL_GetError());
        return false;
    }

    if (
        not SelectPhysicalDevice()        or
        not CreateLogicalDevice()         or
        not CreateSwapchain()             or
        not CreateSyncAndCommandObjects() or
        not CreateDescriptorPool()
    )
    return false;

    auto props = _PhysicalDevice.getProperties();
    Log::Info("Vulkan", "Device: {} (API {}.{}.{}), swapchain {}x{} x{}",
        static_cast<const char*>(props.deviceName),
        vk::apiVersionMajor(props.apiVersion), vk::apiVersionMinor(props.apiVersion), vk::apiVersionPatch(props.apiVersion),
        _SwapchainExtent.width, _SwapchainExtent.height, _ImageCount);
    return true;
}

void Vulkan::Shutdown()
{
    if (not _Instance) return;

    if (_Device) {
        FlushDeferred();

        for (auto& frame : _Frames) {
            if (frame.ImageAvailable) _Device.destroySemaphore(frame.ImageAvailable);
            if (frame.InFlight)       _Device.destroyFence(frame.InFlight);
            if (frame.CommandPool)    _Device.destroyCommandPool(frame.CommandPool);
            frame = {};
        }

        if (_ImmediatePool)  _Device.destroyCommandPool(_ImmediatePool);
        DestroySwapchain(false);
        if (_DescriptorPool) _Device.destroyDescriptorPool(_DescriptorPool);
        _Device.destroy();
    }

    if (_Surface) _Instance.destroySurfaceKHR(_Surface);

    if (_DebugMessenger) {
        auto destroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(_Instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (destroy) destroy(_Instance, _DebugMessenger, nullptr);
    }

    _Instance.destroy();

    _Instance = VK_NULL_HANDLE; _PhysicalDevice = VK_NULL_HANDLE; _Device = VK_NULL_HANDLE;
    _Queue = VK_NULL_HANDLE; _Surface = VK_NULL_HANDLE; _DescriptorPool = VK_NULL_HANDLE;
    _ImmediatePool = VK_NULL_HANDLE; _DebugMessenger = VK_NULL_HANDLE;
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------

bool Vulkan::CreateInstance()
{
    Uint32 sdlExtensionCount = 0;
    auto   sdlExtensions     = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

    if (not sdlExtensions) {
        Log::Fatal("Vulkan", "SDL_Vulkan_GetInstanceExtensions failed: {}", SDL_GetError());
        return false;
    }

    List<const char*> extensions(sdlExtensions, sdlExtensions + sdlExtensionCount);
    List<const char*> layers;

    if (_Desc.Validation) {
        if (HasLayer(ValidationLayer)) {
            layers.push_back(ValidationLayer);
            extensions.push_back(vk::EXTDebugUtilsExtensionName);
        } else {
            Log::Warn("Vulkan", "Validation requested but {} is not installed", ValidationLayer);
            _Desc.Validation = false;
        }
    }

    vk::ApplicationInfo appInfo
    {
        .pApplicationName   = Info::Name,
        .applicationVersion = vk::makeApiVersion(0u, Info::VersionMajor, Info::VersionMinor, Info::VersionPatch),
        .pEngineName        = Info::Engine,
        .engineVersion      = vk::makeApiVersion(0u, Info::VersionMajor, Info::VersionMinor, Info::VersionPatch),
        .apiVersion         = vk::ApiVersion13
    };

    vk::InstanceCreateInfo createInfo
    {
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = u32(layers.size()),
        .ppEnabledLayerNames     = layers.data(),
        .enabledExtensionCount   = u32(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };

    auto [result, instance] = vk::createInstance(createInfo, nullptr);
    if (result != vk::Result::eSuccess) {
        Log::Fatal("Vulkan", "vkCreateInstance failed: {}", vk::to_string(result));
        return false;
    }
    _Instance = instance;

    if (_Desc.Validation) {
        VkDebugUtilsMessengerCreateInfoEXT messengerInfo = {};
        messengerInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        messengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        messengerInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        messengerInfo.pfnUserCallback = &DebugCallback;

        auto create = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(_Instance, "vkCreateDebugUtilsMessengerEXT"));
        if (create) create(_Instance, &messengerInfo, nullptr, &_DebugMessenger);
        Log::Info("Vulkan", "Validation layers enabled");
    }

    return true;
}

bool Vulkan::SelectPhysicalDevice()
{
    auto [result, physicalDevices] = _Instance.enumeratePhysicalDevices();

    if (result != vk::Result::eSuccess or physicalDevices.empty()) {
        Log::Fatal("Vulkan", "No Vulkan-capable GPU found");
        return false;
    }

    // Score candidates: must support 1.3 + graphics/present queue + swapchain;
    // prefer discrete GPUs.
    i32 bestScore = -1;

    for (auto device : physicalDevices)
    {
        auto props = device.getProperties();
        if (props.apiVersion < vk::ApiVersion13) continue;

        auto [extResult, extensions] = device.enumerateDeviceExtensionProperties();
        bool hasSwapchain = extResult == vk::Result::eSuccess and std::any_of(extensions.begin(), extensions.end(),
            [](const vk::ExtensionProperties& e) { return std::strcmp(e.extensionName, vk::KHRSwapchainExtensionName) == 0; });
        if (not hasSwapchain) continue;

        auto queueFamilies = device.getQueueFamilyProperties();
        i32  family        = -1;

        for (u32 index = 0; index < queueFamilies.size(); index++)
        {
            if (not (queueFamilies[index].queueFlags & vk::QueueFlagBits::eGraphics)) continue;

            auto [supportResult, presentSupport] = device.getSurfaceSupportKHR(index, _Surface);
            if (supportResult == vk::Result::eSuccess and presentSupport) {
                family = i32(index);
                break;
            }
        }
        if (family < 0) continue;

        i32 score = 1;
        if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)   score += 1000;
        if (props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) score += 100;

        if (score > bestScore) {
            bestScore       = score;
            _PhysicalDevice = device;
            _QueueFamily    = u32(family);
        }
    }

    if (bestScore < 0) {
        Log::Fatal("Vulkan", "No GPU supports Vulkan 1.3 with a graphics+present queue");
        return false;
    }
    return true;
}

bool Vulkan::CreateLogicalDevice()
{
    float queuePriority = 1.0f;

    vk::DeviceQueueCreateInfo queueInfo {
        .queueFamilyIndex = _QueueFamily,
        .queueCount       = 1,
        .pQueuePriorities = &queuePriority
    };

    // Use StructureChain to securely link pNext extensions
    vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDeviceVulkan13Features> createChain;

    auto& deviceInfo = createChain.get<vk::DeviceCreateInfo>();
    auto& features13 = createChain.get<vk::PhysicalDeviceVulkan13Features>();

    features13.setDynamicRendering(vk::True)
              .setSynchronization2(vk::True);

    const char* deviceExtensions[] = { vk::KHRSwapchainExtensionName };

    deviceInfo.setQueueCreateInfoCount(1)
              .setPQueueCreateInfos(&queueInfo)
              .setEnabledExtensionCount(1)
              .setPpEnabledExtensionNames(deviceExtensions);

    auto [result, device] = _PhysicalDevice.createDevice(deviceInfo);
    if (result != vk::Result::eSuccess) {
        Log::Fatal("Vulkan", "vkCreateDevice failed: {}", vk::to_string(result));
        return false;
    }
    _Device = device;

    _Queue = _Device.getQueue(_QueueFamily, 0);
    return true;
}

bool Vulkan::CreateSwapchain()
{
    auto [capsResult, caps] = _PhysicalDevice.getSurfaceCapabilitiesKHR(_Surface);
    if (capsResult != vk::Result::eSuccess) return false;

    // Format: prefer BGRA8 UNORM / sRGB-nonlinear (what ImGui and the
    // compositor expect), otherwise take the first offered.
    auto [fmtResult, formats] = _PhysicalDevice.getSurfaceFormatsKHR(_Surface);
    if (fmtResult != vk::Result::eSuccess or formats.empty()) return false;

    vk::SurfaceFormatKHR chosen = formats[0];
    for (auto& f : formats)
        if ((f.format == vk::Format::eB8G8R8A8Unorm or f.format == vk::Format::eR8G8B8A8Unorm)
            and f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) { chosen = f; break; }

    _SwapchainFormat     = chosen.format;
    _SwapchainColorSpace = chosen.colorSpace;

    // Present mode: FIFO is always available (vsync). Mailbox when vsync off.
    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
    if (not _Desc.VSync) {
        auto [pmResult, modes] = _PhysicalDevice.getSurfacePresentModesKHR(_Surface);
        if (pmResult == vk::Result::eSuccess) {
            if      (std::find(modes.begin(), modes.end(), vk::PresentModeKHR::eMailbox)   != modes.end()) presentMode = vk::PresentModeKHR::eMailbox;
            else if (std::find(modes.begin(), modes.end(), vk::PresentModeKHR::eImmediate) != modes.end()) presentMode = vk::PresentModeKHR::eImmediate;
        }
    }

    // Extent: the surface tells us, unless it leaves it to us (0xFFFFFFFF).
    if (caps.currentExtent.width != 0xFFFFFFFFu) {
        _SwapchainExtent = caps.currentExtent;
    } else {
        u32 w = 0, h = 0;
        SDL::GetBinding().PixelSize(w, h);
        _SwapchainExtent = vk::Extent2D {
            .width  = std::clamp(w, caps.minImageExtent.width,  caps.maxImageExtent.width),
            .height = std::clamp(h, caps.minImageExtent.height, caps.maxImageExtent.height)
        };
    }
    if (_SwapchainExtent.width == 0 or _SwapchainExtent.height == 0) return false;   // minimised

    _MinImageCount = std::max(2u, caps.minImageCount);
    u32 requested  = _MinImageCount + 1;
    if (caps.maxImageCount > 0) requested = std::min(requested, caps.maxImageCount);

    vk::SwapchainKHR oldSwapchain = _Swapchain;

    vk::SwapchainCreateInfoKHR swapchainInfo {
        .surface                = _Surface,
        .minImageCount          = requested,
        .imageFormat            = _SwapchainFormat,
        .imageColorSpace        = _SwapchainColorSpace,
        .imageExtent            = _SwapchainExtent,
        .imageArrayLayers       = 1,
        .imageUsage             = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
        .imageSharingMode       = vk::SharingMode::eExclusive,
        .preTransform           = caps.currentTransform,
        .compositeAlpha         = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode            = presentMode,
        .clipped                = true,
        .oldSwapchain           = oldSwapchain,
    };

    auto [resSwap, swapchain] = _Device.createSwapchainKHR(swapchainInfo);
    if (resSwap != vk::Result::eSuccess) {
        Log::Error("Vulkan", "vkCreateSwapchainKHR failed: {}", vk::to_string(resSwap));
        return false;
    }
    if (oldSwapchain) _Device.destroySwapchainKHR(oldSwapchain);
    _Swapchain = swapchain;

    auto [resImg, images] = _Device.getSwapchainImagesKHR(_Swapchain);
    if (resImg != vk::Result::eSuccess) return false;
    _SwapchainImages = images;
    _ImageCount      = u32(_SwapchainImages.size());

    _SwapchainImageViews.resize(_ImageCount);
    _RenderFinishedSemaphores.resize(_ImageCount);

    for (u32 i = 0; i < _ImageCount; i++) {
        vk::ImageViewCreateInfo viewInfo {
            .image      = _SwapchainImages[i],
            .viewType   = vk::ImageViewType::e2D,
            .format     = _SwapchainFormat,
            .components = {
                .r = vk::ComponentSwizzle::eIdentity,
                .g = vk::ComponentSwizzle::eIdentity,
                .b = vk::ComponentSwizzle::eIdentity,
                .a = vk::ComponentSwizzle::eIdentity
            },
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .levelCount = 1,
                .layerCount = 1
            }
        };

        auto [resView, view] = _Device.createImageView(viewInfo);
        if (resView != vk::Result::eSuccess) return false;
        _SwapchainImageViews[i] = view;

        auto [resSem, semaphore] = _Device.createSemaphore({});
        if (resSem != vk::Result::eSuccess) return false;
        _RenderFinishedSemaphores[i] = semaphore;
    }

    return true;
}

void Vulkan::DestroySwapchain(bool keepHandle)
{
    for (auto view : _SwapchainImageViews)      if (view) _Device.destroyImageView(view);
    for (auto sem  : _RenderFinishedSemaphores) if (sem)  _Device.destroySemaphore(sem);
    _SwapchainImageViews.clear();
    _RenderFinishedSemaphores.clear();
    _SwapchainImages.clear();

    // keepHandle: the old swapchain is passed as oldSwapchain to the new one.
    if (not keepHandle and _Swapchain) {
        _Device.destroySwapchainKHR(_Swapchain);
        _Swapchain = VK_NULL_HANDLE;
    }
}

bool Vulkan::RebuildSwapchain()
{
    u32 w = 0, h = 0;
    SDL::GetBinding().PixelSize(w, h);
    if (w == 0 or h == 0) return false;     // minimised; try again later

    (void)_Device.waitIdle();
    DestroySwapchain(true);
    if (not CreateSwapchain()) return false;

    _SwapchainDirty = false;
    Log::Trace("Vulkan", "Swapchain rebuilt: {}x{}", _SwapchainExtent.width, _SwapchainExtent.height);
    return true;
}

bool Vulkan::CreateSyncAndCommandObjects()
{
    for (auto& frame : _Frames)
    {
        vk::CommandPoolCreateInfo poolInfo {
            .flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            .queueFamilyIndex = _QueueFamily
        };

        auto [resPool, pool] = _Device.createCommandPool(poolInfo);
        if (resPool != vk::Result::eSuccess) return false;
        frame.CommandPool = pool;

        vk::CommandBufferAllocateInfo allocInfo {
            .commandPool        = frame.CommandPool,
            .level              = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };

        auto [resCmd, cmdBuffers] = _Device.allocateCommandBuffers(allocInfo);
        if (resCmd != vk::Result::eSuccess or cmdBuffers.empty()) return false;
        frame.CommandBuffer = cmdBuffers[0];

        auto [resSem, sem] = _Device.createSemaphore({});
        if (resSem != vk::Result::eSuccess) return false;
        frame.ImageAvailable = sem;

        vk::FenceCreateInfo fenceInfo {
            .flags = vk::FenceCreateFlagBits::eSignaled
        };

        auto [resFence, fence] = _Device.createFence(fenceInfo);
        if (resFence != vk::Result::eSuccess) return false;
        frame.InFlight = fence;
    }

    vk::CommandPoolCreateInfo immediateInfo {
        .flags            = vk::CommandPoolCreateFlagBits::eTransient,
        .queueFamilyIndex = _QueueFamily
    };
    auto [resPool, pool] = _Device.createCommandPool(immediateInfo);
    if (resPool != vk::Result::eSuccess) return false;
    _ImmediatePool = pool;

    return true;
}

bool Vulkan::CreateDescriptorPool()
{
    vk::DescriptorPoolSize poolSizes[] = {
        { vk::DescriptorType::eSampler,              1000 },
        { vk::DescriptorType::eCombinedImageSampler, 1000 },
        { vk::DescriptorType::eSampledImage,         1000 },
        { vk::DescriptorType::eStorageImage,         1000 },
        { vk::DescriptorType::eUniformTexelBuffer,   1000 },
        { vk::DescriptorType::eStorageTexelBuffer,   1000 },
        { vk::DescriptorType::eUniformBuffer,        1000 },
        { vk::DescriptorType::eStorageBuffer,        1000 },
        { vk::DescriptorType::eUniformBufferDynamic, 1000 },
        { vk::DescriptorType::eStorageBufferDynamic, 1000 },
        { vk::DescriptorType::eInputAttachment,      1000 }
    };

    vk::DescriptorPoolCreateInfo descPoolInfo {
        .flags          = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets        = 1000 * u32(std::size(poolSizes)),
        .poolSizeCount  = u32(std::size(poolSizes)),
        .pPoolSizes     = poolSizes
    };

    auto [resDesc, descPool] = _Device.createDescriptorPool(descPoolInfo);
    if (resDesc != vk::Result::eSuccess) return false;
    _DescriptorPool = descPool;
    return true;
}

// -----------------------------------------------------------------------------
// Frame lifecycle
// -----------------------------------------------------------------------------

bool Vulkan::BeginFrame(FrameContext& outFrame)
{
    if (SDL::GetBinding().IsMinimized()) return false;
    if (_SwapchainDirty and not RebuildSwapchain()) return false;

    FrameSync& frame = _Frames[_CurrentFrame];

    if (_Device.waitForFences(1, &frame.InFlight, vk::True, UINT64_MAX) != vk::Result::eSuccess) return false;

    // GPU is done with this slot: run deferred destruction.
    for (auto& destroy : frame.DeletionQueue) destroy();
    frame.DeletionQueue.clear();

    u32 imageIndex = 0;
    VkResult acquire = vkAcquireNextImageKHR(_Device, _Swapchain, UINT64_MAX, frame.ImageAvailable, VK_NULL_HANDLE, &imageIndex);

    if (acquire == VK_ERROR_OUT_OF_DATE_KHR) { _SwapchainDirty = true; return false; }
    if (acquire == VK_SUBOPTIMAL_KHR)         _SwapchainDirty = true;
    else if (acquire != VK_SUCCESS) {
        Log::Error("Vulkan", "vkAcquireNextImageKHR failed: {}", i32(acquire));
        return false;
    }

    if (_Device.resetFences(1, &frame.InFlight) != vk::Result::eSuccess) return false;

    (void)frame.CommandBuffer.reset();

    vk::CommandBufferBeginInfo beginInfo {
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };
    if (frame.CommandBuffer.begin(beginInfo) != vk::Result::eSuccess) return false;

    outFrame = FrameContext {
        .Cmd            = frame.CommandBuffer,
        .SwapchainImage = _SwapchainImages[imageIndex],
        .SwapchainView  = _SwapchainImageViews[imageIndex],
        .Extent         = _SwapchainExtent,
        .ImageIndex     = imageIndex,
        .FrameIndex     = _CurrentFrame,
    };
    return true;
}

void Vulkan::BeginSwapchainPass(const FrameContext& frame, Vec4 clearColor)
{
    TransitionImage(frame.Cmd, frame.SwapchainImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);

    vk::RenderingAttachmentInfo colorAttachment {
        .imageView   = frame.SwapchainView,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp      = vk::AttachmentLoadOp::eClear,
        .storeOp     = vk::AttachmentStoreOp::eStore,
        .clearValue  = vk::ClearColorValue(std::array<float, 4>{ clearColor.x, clearColor.y, clearColor.z, clearColor.w }),
    };

    vk::RenderingInfo renderingInfo {
        .renderArea           = vk::Rect2D { .offset = { 0, 0 }, .extent = frame.Extent },
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &colorAttachment,
    };

    frame.Cmd.beginRendering(renderingInfo);
}

void Vulkan::EndSwapchainPass(const FrameContext& frame)
{
    frame.Cmd.endRendering();
    TransitionImage(frame.Cmd, frame.SwapchainImage, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);
}

void Vulkan::EndFrame(const FrameContext& ctx)
{
    FrameSync& frame = _Frames[ctx.FrameIndex];

    (void)ctx.Cmd.end();

    vk::SemaphoreSubmitInfo waitInfo {
        .semaphore = frame.ImageAvailable,
        .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    };
    vk::SemaphoreSubmitInfo signalInfo {
        .semaphore = _RenderFinishedSemaphores[ctx.ImageIndex],
        .stageMask = vk::PipelineStageFlagBits2::eAllCommands,
    };
    vk::CommandBufferSubmitInfo cmdInfo {
        .commandBuffer = ctx.Cmd,
    };
    vk::SubmitInfo2 submitInfo {
        .waitSemaphoreInfoCount   = 1,
        .pWaitSemaphoreInfos      = &waitInfo,
        .commandBufferInfoCount   = 1,
        .pCommandBufferInfos      = &cmdInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos    = &signalInfo,
    };

    if (_Queue.submit2(1, &submitInfo, frame.InFlight) != vk::Result::eSuccess) {
        Log::Error("Vulkan", "vkQueueSubmit2 failed");
    }

    VkSemaphore    renderFinished = _RenderFinishedSemaphores[ctx.ImageIndex];
    VkSwapchainKHR swapchain      = _Swapchain;
    VkPresentInfoKHR presentInfo  = {};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &renderFinished;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &swapchain;
    presentInfo.pImageIndices      = &ctx.ImageIndex;

    VkResult present = vkQueuePresentKHR(_Queue, &presentInfo);
    if (present == VK_ERROR_OUT_OF_DATE_KHR or present == VK_SUBOPTIMAL_KHR) _SwapchainDirty = true;
    else if (present != VK_SUCCESS) Log::Error("Vulkan", "vkQueuePresentKHR failed: {}", i32(present));

    _CurrentFrame = (_CurrentFrame + 1) % FramesInFlight;
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

void Vulkan::WaitIdle() const
{
    if (_Device) (void)_Device.waitIdle();
}

void Vulkan::DeferDestroy(Function<void()> destroy)
{
    _Frames[_CurrentFrame].DeletionQueue.push_back(std::move(destroy));
}

void Vulkan::FlushDeferred()
{
    if (not _Device) return;
    (void)_Device.waitIdle();
    for (auto& frame : _Frames) {
        for (auto& destroy : frame.DeletionQueue) destroy();
        frame.DeletionQueue.clear();
    }
}

bool Vulkan::ImmediateSubmit(const Function<void(vk::CommandBuffer)>& record)
{
    vk::CommandBufferAllocateInfo allocInfo {
        .commandPool        = _ImmediatePool,
        .level              = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };
    auto [resCmd, cmds] = _Device.allocateCommandBuffers(allocInfo);
    if (resCmd != vk::Result::eSuccess) return false;
    vk::CommandBuffer cmd = cmds[0];

    vk::CommandBufferBeginInfo beginInfo { .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    (void)cmd.begin(beginInfo);
    record(cmd);
    (void)cmd.end();

    auto [resFence, fence] = _Device.createFence({});
    if (resFence != vk::Result::eSuccess) return false;

    vk::CommandBufferSubmitInfo cmdInfo { .commandBuffer = cmd };
    vk::SubmitInfo2 submitInfo { .commandBufferInfoCount = 1, .pCommandBufferInfos = &cmdInfo };

    bool ok = _Queue.submit2(1, &submitInfo, fence) == vk::Result::eSuccess
          and _Device.waitForFences(1, &fence, vk::True, UINT64_MAX) == vk::Result::eSuccess;

    _Device.destroyFence(fence);
    _Device.freeCommandBuffers(_ImmediatePool, 1, &cmd);
    return ok;
}

Optional<u32> Vulkan::FindMemoryType(u32 typeBits, vk::MemoryPropertyFlags properties) const
{
    auto memProps = _PhysicalDevice.getMemoryProperties();
    for (u32 i = 0; i < memProps.memoryTypeCount; i++)
        if ((typeBits & (1u << i)) and (memProps.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    return std::nullopt;
}

void Vulkan::TransitionImage(vk::CommandBuffer cmd, vk::Image image,
                             vk::ImageLayout from, vk::ImageLayout to,
                             vk::ImageAspectFlags aspect)
{
    // Deliberately coarse (ALL_COMMANDS); fine for a handful of transitions per
    // frame. Render engines that need tighter sync can issue their own barriers.
    vk::ImageMemoryBarrier2 barrier {
        .srcStageMask        = vk::PipelineStageFlagBits2::eAllCommands,
        .srcAccessMask       = vk::AccessFlagBits2::eMemoryWrite,
        .dstStageMask        = vk::PipelineStageFlagBits2::eAllCommands,
        .dstAccessMask       = vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
        .oldLayout           = from,
        .newLayout           = to,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image               = image,
        .subresourceRange    = {
            .aspectMask     = aspect,
            .baseMipLevel   = 0,
            .levelCount     = vk::RemainingMipLevels,
            .baseArrayLayer = 0,
            .layerCount     = vk::RemainingArrayLayers
        }
    };

    vk::DependencyInfo dependency {
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = &barrier
    };

    cmd.pipelineBarrier2(dependency);
}
