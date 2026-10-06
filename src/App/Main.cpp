#include <__Vendor/SDL3/SDL.h>
#include <__Vendor/SDL3/SDL_vulkan.h>
#include <__Vendor/Vulkan/vulkan/vulkan.h>
#include <__Vendor/ImGui/imgui.h>
#include <__Vendor/ImGui/Backends/imgui_impl_sdl3.h>
#include <__Vendor/ImGui/Backends/imgui_impl_vulkan.h>
#include <stdio.h>
#include <stdlib.h>

// Macro to catch silent Vulkan failures instantly
#define CHECK_VK(res, msg) if ((res) != VK_SUCCESS) { fprintf(stderr, "[Vulkan Error] %s (Code: %d)\n", msg, res); return false; }

// Active Vulkan Core Primitives 
VkInstance       g_Instance       = VK_NULL_HANDLE;
VkPhysicalDevice g_PhysicalDevice = VK_NULL_HANDLE;
VkDevice         g_Device         = VK_NULL_HANDLE;
VkQueue          g_Queue          = VK_NULL_HANDLE;
VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;
uint32_t         g_QueueFamily    = 0; 

// Presentation Layer Primitives (Swapchain & Synchronization)
VkSurfaceKHR    g_Surface                 = VK_NULL_HANDLE;
VkSwapchainKHR  g_Swapchain               = VK_NULL_HANDLE;
uint32_t        g_ImageCount              = 0;
VkImage*        g_SwapchainImages         = nullptr;
VkImageView*    g_SwapchainImageViews     = nullptr;
VkCommandPool   g_CommandPool             = VK_NULL_HANDLE;
VkCommandBuffer g_CommandBuffer           = VK_NULL_HANDLE;
VkSemaphore     g_ImageAvailableSemaphore = VK_NULL_HANDLE;
VkSemaphore     g_RenderFinishedSemaphore = VK_NULL_HANDLE;
VkFence         g_InFlightFence           = VK_NULL_HANDLE;

bool SetupVulkan(SDL_Window* window) {
    // 1. Extensions Setup
    uint32_t extensions_count = 0;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensions_count);
    if (!extensions) {
        fprintf(stderr, "[Vulkan Error] Failed to get SDL3 instance extensions: %s\n", SDL_GetError());
        return false;
    }

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Halftone";
    app_info.apiVersion = VK_API_VERSION_1_3; 

    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &app_info;
    instance_create_info.enabledExtensionCount = extensions_count;
    instance_create_info.ppEnabledExtensionNames = extensions;
    
    VkResult res = vkCreateInstance(&instance_create_info, nullptr, &g_Instance);
    CHECK_VK(res, "vkCreateInstance failed");

    // 2. Hardware Selection
    uint32_t gpu_count = 0;
    vkEnumeratePhysicalDevices(g_Instance, &gpu_count, nullptr);
    if (gpu_count == 0) {
        fprintf(stderr, "[Vulkan Error] No Vulkan-supported GPUs found.\n");
        return false;
    }
    VkPhysicalDevice* gpus = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * gpu_count);
    vkEnumeratePhysicalDevices(g_Instance, &gpu_count, gpus);
    g_PhysicalDevice = gpus[0]; 
    free(gpus);

    // 3. Queue Family Search
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &queue_family_count, nullptr);
    if (queue_family_count == 0) {
        fprintf(stderr, "[Vulkan Error] Failed to query hardware queue families.\n");
        return false;
    }
    VkQueueFamilyProperties* queue_families = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &queue_family_count, queue_families);
    
    int graphics_family = -1;
    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_family = i;
            break;
        }
    }
    free(queue_families);
    if (graphics_family == -1) {
        fprintf(stderr, "[Vulkan Error] Hardware does not support a graphics queue.\n");
        return false;
    }
    g_QueueFamily = (uint32_t)graphics_family;

    // 4. Native OS Surface Hook (Verified ordering match for SDL3)
    if (!SDL_Vulkan_CreateSurface(window, g_Instance, nullptr, &g_Surface)) {
        fprintf(stderr, "[Vulkan Error] SDL_Vulkan_CreateSurface failed: %s\n", SDL_GetError());
        return false;
    }

    // Verify presentation capabilities on our selected graphics queue
    VkBool32 present_support = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily, g_Surface, &present_support);
    if (!present_support) {
        fprintf(stderr, "[Vulkan Error] Selected graphics queue family does not support presentation.\n");
        return false;
    }

    // 5. Logical Device Creation (Enabling Swapchain Extension & Vulkan 1.3 Dynamic Rendering)
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = g_QueueFamily;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queue_priority;

    VkPhysicalDeviceVulkan13Features features13 = {};
    features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features13.dynamicRendering = VK_TRUE; 

    const char* device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = &features13;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = 1;
    device_info.ppEnabledExtensionNames = device_extensions;

    res = vkCreateDevice(g_PhysicalDevice, &device_info, nullptr, &g_Device);
    CHECK_VK(res, "vkCreateDevice failed");
    
    vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);

    // 6. Swapchain Construction (1280x720 Target)
    VkSwapchainCreateInfoKHR swapchain_info = {};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = g_Surface;
    swapchain_info.minImageCount = 2;
    swapchain_info.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    swapchain_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain_info.imageExtent = { 1280, 720 };
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_info.clipped = VK_TRUE;
    
    res = vkCreateSwapchainKHR(g_Device, &swapchain_info, nullptr, &g_Swapchain);
    CHECK_VK(res, "vkCreateSwapchainKHR failed");

    // Retrieve Images and Generate Views
    res = vkGetSwapchainImagesKHR(g_Device, g_Swapchain, &g_ImageCount, nullptr);
    CHECK_VK(res, "vkGetSwapchainImagesKHR (count query) failed");
    
    g_SwapchainImages = (VkImage*)malloc(sizeof(VkImage) * g_ImageCount);
    res = vkGetSwapchainImagesKHR(g_Device, g_Swapchain, &g_ImageCount, g_SwapchainImages);
    CHECK_VK(res, "vkGetSwapchainImagesKHR (retrieval) failed");
    
    g_SwapchainImageViews = (VkImageView*)malloc(sizeof(VkImageView) * g_ImageCount);
    for (uint32_t i = 0; i < g_ImageCount; i++) {
        VkImageViewCreateInfo view_info = {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = g_SwapchainImages[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = VK_FORMAT_B8G8R8A8_UNORM;
        view_info.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.layerCount = 1;
        res = vkCreateImageView(g_Device, &view_info, nullptr, &g_SwapchainImageViews[i]);
        CHECK_VK(res, "vkCreateImageView factory allocation failed");
    }

    // 7. Allocation Pools & Engine Synchronizers
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = g_QueueFamily;
    res = vkCreateCommandPool(g_Device, &pool_info, nullptr, &g_CommandPool);
    CHECK_VK(res, "vkCreateCommandPool failed");

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = g_CommandPool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;
    res = vkAllocateCommandBuffers(g_Device, &alloc_info, &g_CommandBuffer);
    CHECK_VK(res, "vkAllocateCommandBuffers failed");

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    res = vkCreateSemaphore(g_Device, &semaphore_info, nullptr, &g_ImageAvailableSemaphore);
    CHECK_VK(res, "vkCreateSemaphore (ImageAvailable) failed");
    res = vkCreateSemaphore(g_Device, &semaphore_info, nullptr, &g_RenderFinishedSemaphore);
    CHECK_VK(res, "vkCreateSemaphore (RenderFinished) failed");

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    res = vkCreateFence(g_Device, &fence_info, nullptr, &g_InFlightFence);
    CHECK_VK(res, "vkCreateFence failed");

    // 8. Robust Reference Standard Descriptor Pool Allocation
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo desc_pool_info = {};
    desc_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    desc_pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    desc_pool_info.maxSets = 1000 * 11;
    desc_pool_info.poolSizeCount = 11;
    desc_pool_info.pPoolSizes = pool_sizes;
    res = vkCreateDescriptorPool(g_Device, &desc_pool_info, nullptr, &g_DescriptorPool);
    CHECK_VK(res, "Robust vkCreateDescriptorPool allocation failed");

    return true;
}

void CleanupVulkan() {
    if (g_Device) vkDeviceWaitIdle(g_Device);
    if (g_ImageAvailableSemaphore) vkDestroySemaphore(g_Device, g_ImageAvailableSemaphore, nullptr);
    if (g_RenderFinishedSemaphore) vkDestroySemaphore(g_Device, g_RenderFinishedSemaphore, nullptr);
    if (g_InFlightFence)           vkDestroyFence(g_Device, g_InFlightFence, nullptr);
    if (g_CommandPool)             vkDestroyCommandPool(g_Device, g_CommandPool, nullptr);
    
    if (g_SwapchainImageViews) {
        for (uint32_t i = 0; i < g_ImageCount; i++) {
            if (g_SwapchainImageViews[i]) vkDestroyImageView(g_Device, g_SwapchainImageViews[i], nullptr);
        }
        free(g_SwapchainImageViews);
    }
    if (g_SwapchainImages) free(g_SwapchainImages);

    if (g_Swapchain)      vkDestroySwapchainKHR(g_Device, g_Swapchain, nullptr);
    if (g_Surface)        vkDestroySurfaceKHR(g_Instance, g_Surface, nullptr);
    if (g_DescriptorPool) vkDestroyDescriptorPool(g_Device, g_DescriptorPool, nullptr);
    if (g_Device)         vkDestroyDevice(g_Device, nullptr);
    if (g_Instance)       vkDestroyInstance(g_Instance, nullptr);
}

int main(int argc, char* argv[]) {
    printf("[Checkpoint 1] Entered main()\n"); fflush(stdout);
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) return -1;

    SDL_Window* window = SDL_CreateWindow("Halftone", 1280, 720, SDL_WINDOW_VULKAN);
    if (!window) return -1;

    printf("[Checkpoint 4] Entering SetupVulkan()\n"); fflush(stdout);
    if (!SetupVulkan(window)) {
        fprintf(stderr, "[Fatal Error] Vulkan pipeline broke during setup loop.\n");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    printf("[Checkpoint 5] SetupVulkan completed safely\n"); fflush(stdout);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    printf("[Checkpoint 5a] ImGui Base Context Allocated\n"); fflush(stdout);

    ImGui_ImplSDL3_InitForVulkan(window);
    printf("[Checkpoint 5b] ImGui SDL3 Platform Hooked\n"); fflush(stdout);
    
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion = VK_API_VERSION_1_3;
    init_info.Instance = g_Instance;
    init_info.PhysicalDevice = g_PhysicalDevice;
    init_info.Device = g_Device;
    init_info.QueueFamily = g_QueueFamily; 
    init_info.Queue = g_Queue;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.MinImageCount = 2;
    init_info.ImageCount = g_ImageCount;
    init_info.UseDynamicRendering = true; 

    VkFormat color_format = VK_FORMAT_B8G8R8A8_UNORM; 
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &color_format;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    
    ImGui_ImplVulkan_Init(&init_info);
    printf("[Checkpoint 5c] ImGui Vulkan Renderer Backend Ready\n"); fflush(stdout);

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) running = false;
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Engine Controller");
        ImGui::Text("Hello, World! Presentation pipeline fully active.");
        ImGui::End();

        ImGui::Render();

        vkWaitForFences(g_Device, 1, &g_InFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(g_Device, 1, &g_InFlightFence);

        uint32_t image_index;
        vkAcquireNextImageKHR(g_Device, g_Swapchain, UINT64_MAX, g_ImageAvailableSemaphore, VK_NULL_HANDLE, &image_index);

        vkResetCommandBuffer(g_CommandBuffer, 0);
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(g_CommandBuffer, &begin_info);

        VkImageMemoryBarrier layout_barrier = {};
        layout_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        layout_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        layout_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        layout_barrier.srcAccessMask = 0;
        layout_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        layout_barrier.image = g_SwapchainImages[image_index];
        layout_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        layout_barrier.subresourceRange.levelCount = 1;
        layout_barrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(g_CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &layout_barrier);

        VkClearValue clear_color = {{{0.08f, 0.08f, 0.1f, 1.0f}}}; 
        VkRenderingAttachmentInfo color_attachment = {};
        color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        color_attachment.imageView = g_SwapchainImageViews[image_index];
        color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.clearValue = clear_color;

        VkRenderingInfo rendering_info = {};
        rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        rendering_info.renderArea = {{0, 0}, {1280, 720}};
        rendering_info.layerCount = 1;
        rendering_info.colorAttachmentCount = 1;
        rendering_info.pColorAttachments = &color_attachment;

        vkCmdBeginRendering(g_CommandBuffer, &rendering_info);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), g_CommandBuffer);
        vkCmdEndRendering(g_CommandBuffer);

        layout_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        layout_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        layout_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        layout_barrier.dstAccessMask = 0;
        vkCmdPipelineBarrier(g_CommandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &layout_barrier);

        vkEndCommandBuffer(g_CommandBuffer);

        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &g_ImageAvailableSemaphore;
        submit_info.pWaitDstStageMask = wait_stages;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &g_CommandBuffer;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &g_RenderFinishedSemaphore;
        vkQueueSubmit(g_Queue, 1, &submit_info, g_InFlightFence);

        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &g_RenderFinishedSemaphore;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &g_Swapchain;
        present_info.pImageIndices = &image_index;
        vkQueuePresentKHR(g_Queue, &present_info);
    }

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    CleanupVulkan();
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
