#pragma once

#include "util/class.h"
#include "waveform/renderers/qopengl/rgbdata.h"
#include "waveform/renderers/qopengl/shaders/rgbshader.h"
#include "waveform/renderers/qopengl/vertexdata.h"
#include "waveform/renderers/qopengl/waveformrenderersignalbase.h"

namespace qopengl {
class WaveformRendererRGB;
}

class qopengl::WaveformRendererRGB final : public qopengl::WaveformRendererSignalBase {
  public:
    explicit WaveformRendererRGB(WaveformWidgetRenderer* waveformWidget);
    ~WaveformRendererRGB() override;

    // override ::WaveformRendererSignalBase
    void onSetup(const QDomNode& node) override;

    void initializeGL() override;
    void renderGL() override;

  private:
    RGBShader m_shader;
    VertexData m_vertices;
    RGBData m_colors;

    DISALLOW_COPY_AND_ASSIGN(WaveformRendererRGB);
};
