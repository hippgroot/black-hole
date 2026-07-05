#include "camera.hpp"

#include <grf/grf.hpp>
#include <grf/imgui.h>

#include <format>

using namespace grf;

struct TracePC {
  u64   camBufAddr;
  uvec2 screenDims;
  u32   bgIndex;
  u32   outputIndex;
  u32   samplerIndex;
  f32   rs;
};

struct DrawPC {
  uvec2 screenDims;
  u32   traceOutputIndex;
  u32   samplerIndex;
};

const u32 g_flightFrames = 2;

int main() {
  auto pendStarBackground = readImage(std::format("{}/stars.hdr", ASSET_DIR));

  GRF grf(Settings{
    .windowTitle  = "Black Hole Simulation",
    .flightFrames = g_flightFrames
  });

  Camera camera;
  f32 schwarzschildRadius = 1.0f;

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

  Shader traceShader = grf.compileShader(ShaderType::Compute, std::format("{}/trace.gsl", SHADER_DIR));
  ComputePipeline tracePipeline = grf.createComputePipeline(traceShader);

  Buffer camBuf = grf.createBuffer(BufferIntent::FrequentUpdate, sizeof(Camera::Matrices));

  Sampler linearSampler = grf.createSampler(SamplerSettings{
    .uMode = SampleMode::Repeat,
    .vMode = SampleMode::ClampToEdge
  });
  Tex2D starBackground;
  Img2D traceOutput;
{
  auto [w, h] = grf.screenDims();
  traceOutput = grf.createImg2D(Format::rgba16_sfloat, w, h);

  std::vector<std::byte> zeros(4 * w * h, std::byte{});
  traceOutput.write(zeros, Layout::ShaderReadOptimal);
}

  Ring<Sync> syncRing = grf.createSyncRing();
  Ring<CommandBuffer> cmdRing = grf.createCmdRing(QueueType::Graphics);

{
  ImageData data = pendStarBackground.get();
  starBackground = grf.createTex2D(data.format, data.width, data.height);
  starBackground.write(data.bytes, Layout::ShaderReadOptimal);
}

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

    ImGui::Begin("Simulation");
      ImGui::SliderFloat("rs", &schwarzschildRadius, 0.0f, 5.0f, "%.2f");
    ImGui::End();

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

    cmd.transition(traceOutput, Layout::ShaderReadOptimal, Layout::General);

    cmd.bindPipeline(tracePipeline);
  {
    auto [w, h] = grf.screenDims();
    cmd.push(TracePC{
      .camBufAddr = camBuf.address(),
      .screenDims = uvec2(w, h),
      .bgIndex = starBackground.heapIndex(),
      .outputIndex = traceOutput.storageHeapIndex(),
      .samplerIndex = linearSampler.heapIndex(),
      .rs = schwarzschildRadius
    });

    auto [x, y, _] = traceShader.threadGroup();
    cmd.beginProfile("trace");
    cmd.dispatch((w + x - 1) / x, (h + y - 1) / y);
    cmd.endProfile();
  }

    cmd.transition(traceOutput, Layout::General, Layout::ShaderReadOptimal);
    cmd.transition(renderTarget, Layout::Undefined, Layout::ColorAttachmentOptimal);

    cmd.beginRendering({ renderTargetAttachment });

    cmd.bindPipeline(drawPipeline);
  {
    auto [w, h] = grf.screenDims();
    cmd.push(DrawPC{
      .screenDims       = uvec2(w, h),
      .traceOutputIndex = traceOutput.sampledHeapIndex(),
      .samplerIndex     = linearSampler.heapIndex()
    });
  }
    cmd.beginProfile("draw");
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

    if (!grf.gui().wantsKeyboard()) {
      vec3 inputDir = vec3(
        input.isPressed(Key::D) - input.isPressed(Key::A),
        input.isPressed(Key::Space) - input.isPressed(Key::LeftShift),
        input.isPressed(Key::S) - input.isPressed(Key::W)
      );
      if (inputDir != vec3(0.0)) {
        vec3 delta = camera.speed() * glm::normalize(inputDir) * dt;
        camera.translate(delta);
      }
    }

    if (!grf.gui().wantsMouse() && input.isPressed(MouseButton::Left)) {
      auto [dx, dy] = input.cursorDelta();
      camera.rotate(-dx * camera.sensitivity(), -dy * camera.sensitivity());
    }

    grf.endFrame();
  }
}