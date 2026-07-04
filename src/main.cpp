#include <grf/grf.hpp>

#include <format>
#include <grf/types.hpp>

using namespace grf;

const u32 g_flightFrames = 2;

int main() {
  GRF grf(Settings{
    .windowTitle  = "Black Hole Simulation",
    .flightFrames = g_flightFrames
  });

  BlendState alphaBlend{
    .enable = true,
    .srcColorFactor = BlendFactor::SrcAlpha,
    .dstAlphaFactor = BlendFactor::OneMinusSrcAlpha,
    .alphaOp        = BlendOp::Add
  };

  Shader vertShader = grf.compileShader(ShaderType::Vertex, std::format("{}/vert.gsl", SHADER_DIR));
  Shader fragShader = grf.compileShader(ShaderType::Fragment, std::format("{}/frag.gsl", SHADER_DIR));
  GraphicsPipeline drawPipeline = grf.createGraphicsPipeline(vertShader, fragShader, GraphicsPipelineSettings{
    .colorFormats = { Format::bgra8_srgb },
    .cullMode     = CullMode::None,
    .blends       = { alphaBlend }
  });

  Ring<Sync> syncRing = grf.createSyncRing();
  Ring<CommandBuffer> cmdRing = grf.createCmdRing(QueueType::Graphics);

  Input& input = grf.input();
  while (grf.running([&](){ return input.isJustPressed(Key::Escape); })) {
    auto [frameIndex, dt] = grf.beginFrame();
    grf.gui().beginFrame();
    grf.profiler().render();

    auto& sync = syncRing[frameIndex];
    auto& cmd = cmdRing[frameIndex];

    grf.wait(sync);

    auto renderTarget = grf.nextSwapchainImage();
    ColorAttachment renderTargetAttachment{
      .img      = renderTarget,
      .loadOp   = LoadOp::Clear,
      .storeOp  = StoreOp::Store
    };

    cmd.begin();
    cmd.transition(renderTarget, Layout::Undefined, Layout::ColorAttachmentOptimal);
    cmd.beginRendering({ renderTargetAttachment });

    cmd.beginProfile("draw");
    cmd.bindPipeline(drawPipeline);
  {
    auto [w, h] = grf.screenDims();
    cmd.push(uvec2(w, h));
  }
    cmd.draw(6);
    cmd.endProfile();

    cmd.beginProfile("gui");
    grf.gui().render(cmd);
    cmd.endProfile();

    cmd.endRendering();
    cmd.transition(renderTarget, Layout::ColorAttachmentOptimal, Layout::PresentSrc);
    cmd.end();

    sync = grf.submit(cmd, { renderTarget.sync() });
    grf.present(renderTarget, { sync });
  }
}