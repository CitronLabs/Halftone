module;
#include <__Vendor/Vulkan/vulkan/vulkan.hpp>
#include <__Vendor/ImGui/imgui.h>
#include <algorithm>
#include <array>

module com.citronlabs.Halftone.Chroma;

import com.citronlabs.Halftone.Core;
import com.citronlabs.Halftone.Bindings;
import com.citronlabs.Halftone.Graphics;
import com.citronlabs.Halftone.Scene;

using namespace Halftone::Chroma;
using namespace Halftone::Core;
using namespace Halftone::Graphics;

namespace
{
    vk::ClearColorValue ToClear(Vec4 c) { return vk::ClearColorValue(std::array<float, 4>{ c.x, c.y, c.z, c.w }); }
}

bool Engine::Initialize(const RenderTarget& /*target*/)
{
    // TODO: create pipelines / descriptor layouts for each pass here.
    // Device: Halftone::Bindings::Vulkan::GetBinding().Device()
    Log::Info("Chroma", "Engine initialized");
    return true;
}

void Engine::Shutdown()
{
    // TODO: destroy pass resources.
}

void Engine::OnTargetResized(const RenderTarget& /*target*/)
{
    // TODO: recreate size-dependent intermediates (G-buffer, line-art edges).
}

void Engine::Render(const RenderContext& ctx)
{
    const auto extent = ctx.Target.Extent();

    Vec4 background = _Settings.SceneBackground;
    switch (ctx.View.Kind) {
        case ViewKind::Scene:       background = _Settings.SceneBackground;  break;
        case ViewKind::CanvasDepth: background = _Settings.SceneBackground;  break;
        case ViewKind::CanvasFlat:  background = _Settings.DeskColor;        break;
        case ViewKind::ModelFocus:  background = _Settings.StudioBackground; break;
    }

    vk::RenderingAttachmentInfo color {
        .imageView   = ctx.Target.ColorView(),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp      = vk::AttachmentLoadOp::eClear,
        .storeOp     = vk::AttachmentStoreOp::eStore,
        .clearValue  = ToClear(background),
    };

    vk::RenderingAttachmentInfo depth {
        .imageView   = ctx.Target.DepthView(),
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp      = vk::AttachmentLoadOp::eClear,
        .storeOp     = vk::AttachmentStoreOp::eDontCare,
        .clearValue  = vk::ClearDepthStencilValue { .depth = 1.0f, .stencil = 0 },
    };

    vk::RenderingInfo rendering {
        .renderArea           = vk::Rect2D { .offset = { 0, 0 }, .extent = extent },
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &color,
        .pDepthAttachment     = ctx.Target.HasDepth() ? &depth : nullptr,
    };

    ctx.Cmd.beginRendering(rendering);

    switch (ctx.View.Kind)
    {
        case ViewKind::CanvasFlat:
        {
            // The page itself. Until the layer compositor exists this is a
            // paper-coloured rectangle at the correct place and zoom.
            auto* canvas = ctx.Scene.FindAs<Halftone::Scene::Canvas>(ctx.View.Focus);
            if (not canvas) break;

            Vec4 r = CanvasPageRect(ctx.View.Camera, canvas->Document.Size, { f32(extent.width), f32(extent.height) });
            i32 x0 = std::clamp(i32(r.x), 0, i32(extent.width));
            i32 y0 = std::clamp(i32(r.y), 0, i32(extent.height));
            i32 x1 = std::clamp(i32(r.x + r.z), 0, i32(extent.width));
            i32 y1 = std::clamp(i32(r.y + r.w), 0, i32(extent.height));
            if (x1 <= x0 or y1 <= y0) break;

            vk::ClearAttachment page { .aspectMask = vk::ImageAspectFlagBits::eColor, .colorAttachment = 0,
                                       .clearValue = ToClear(_Settings.PaperColor) };
            vk::ClearRect rect { .rect = { .offset = { x0, y0 }, .extent = { u32(x1 - x0), u32(y1 - y0) } },
                                 .baseArrayLayer = 0, .layerCount = 1 };
            ctx.Cmd.clearAttachments(1, &page, 1, &rect);

            // TODO(compositor): draw each visible layer's current drawing
            // (Layer::DrawingAt(ctx.TimelineFrame)) with its BlendMode/Opacity.
            break;
        }
        case ViewKind::Scene:
        case ViewKind::CanvasDepth:
        case ViewKind::ModelFocus:
            // TODO: Environment -> Cards -> Models -> Lighting -> LineArt -> Composite
            // (iterate ctx.Scene.Nodes(), view/projection from ctx.View.Camera).
            break;
    }

    ctx.Cmd.endRendering();
}

void Engine::DrawSettings()
{
    ImGui::ColorEdit3("Scene Background", &_Settings.SceneBackground.x);
    ImGui::ColorEdit3("Paper", &_Settings.PaperColor.x);
    ImGui::Checkbox("Line Art", &_Settings.LineArt);
    ImGui::BeginDisabled(not _Settings.LineArt);
    ImGui::SliderFloat("Line Width", &_Settings.LineWidth, 0.5f, 6.0f, "%.1f px");
    ImGui::EndDisabled();
    ImGui::Checkbox("Halftone Shading", &_Settings.HalftoneShading);
    ImGui::BeginDisabled(not _Settings.HalftoneShading);
    ImGui::SliderFloat("Dot Pitch", &_Settings.HalftoneScale, 2.0f, 24.0f, "%.0f px");
    ImGui::EndDisabled();
}
