#include "camera.hpp"

#include <grf/grf.hpp>

#include <format>

using namespace grf;

const     u32 g_flightFrames = 2;

int main() {
  GRF grf(Settings{
    .windowTitle  = "Black Hole Simulation",
    .flightFrames = g_flightFrames
  });

  Camera camera;

  auto resizeCallback = [&](u32 w , u32 h) {
    f32 ar = static_cast<f32>(w) / static_cast<f32>(h);
    camera.setAspect(ar);
  };
  grf.resizeCallback(resizeCallback);
{
  auto [w, h] = grf.screenDims();
  resizeCallback(w, h);
}

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

  Buffer camBuf = grf.createBuffer(BufferIntent::FrequentUpdate, sizeof(Camera::Matrices));

  Ring<Sync> syncRing = grf.createSyncRing();
  Ring<CommandBuffer> cmdRing = grf.createCmdRing(QueueType::Graphics);

  Input& input = grf.input();
  while (grf.running([&](){ return input.isJustPressed(Key::Escape); })) {
    if (camera.needsUpdate()) {
      camera.update();
      camBuf.write(camera.matrices());
    }

    auto [frameIndex, dt] = grf.beginFrame();
    grf.gui().beginFrame();

    grf.profiler().render();
    camera.settingsGUI();

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

    vec3 inputDir = vec3(
      input.isPressed(Key::D) - input.isPressed(Key::A),
      input.isPressed(Key::Space) - input.isPressed(Key::LeftShift),
      input.isPressed(Key::S) - input.isPressed(Key::W)
    );
    if (inputDir != vec3(0.0)) {
      vec3 delta = camera.speed() * glm::normalize(inputDir) * dt;
      camera.translate(delta);
    }

    if (input.isPressed(MouseButton::Left)) {
      auto [dx, dy] = input.cursorDelta();
      camera.rotate(-dx * camera.sensitivity(), -dy * camera.sensitivity());
    }

    grf.endFrame();
  }
}