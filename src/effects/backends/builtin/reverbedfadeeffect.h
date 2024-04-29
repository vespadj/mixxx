// Ported from CAPS Reverb.
// This effect is GPL code.

#pragma once

#include <Reverb.h>

#include <QMap>

#include "effects/backends/effectprocessor.h"
#include "util/class.h"
#include "util/types.h"

class ReverbedFadeGroupState : public EffectState {
  public:
    ReverbedFadeGroupState(const mixxx::EngineParameters& engineParameters)
            : EffectState(engineParameters),
              sampleRate(engineParameters.sampleRate()),
              sendPrevious(0) {
    }
    ~ReverbedFadeGroupState() override = default;

    void engineParametersChanged(const mixxx::EngineParameters& engineParameters) {
        sampleRate = engineParameters.sampleRate();
        sendPrevious = 0;
    }

    float sampleRate;
    float sendPrevious;
    MixxxPlateX2 reverb;
};

class ReverbedFadeEffect : public EffectProcessorImpl<ReverbedFadeGroupState> {
  public:
    ReverbedFadeEffect() = default;
    ~ReverbedFadeEffect() override = default;

    static QString getId();
    static EffectManifestPointer getManifest();

    void loadEngineEffectParameters(
            const QMap<QString, EngineEffectParameterPointer>& parameters) override;

    void processChannel(
            ReverbedFadeGroupState* pState,
            const CSAMPLE* pInput,
            CSAMPLE* pOutput,
            const mixxx::EngineParameters& engineParameters,
            const EffectEnableState enableState,
            const GroupFeatureState& groupFeatures) override;

    double f_in(double main);
    double f_wet(double main);

    CSAMPLE inputgained;
    CSAMPLE* pInputgained;

  private:
    QString debugString() const {
        return getId();
    }

    EngineEffectParameterPointer m_pDecayParameter;
    EngineEffectParameterPointer m_pBandWidthParameter;
    EngineEffectParameterPointer m_pDampingParameter;
    EngineEffectParameterPointer m_pSendParameter;
    EngineEffectParameterPointer m_pMainParameter;

    DISALLOW_COPY_AND_ASSIGN(ReverbedFadeEffect);
};
