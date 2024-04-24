#include "effects/backends/builtin/reverbedfadeeffect.h"

#include "effects/backends/effectmanifest.h"
#include "engine/effects/engineeffectparameter.h"
#include "util/sample.h"

// static
QString ReverbedFadeEffect::getId() {
    return "org.mixxx.effects.reverbedfade";
}

// static
EffectManifestPointer ReverbedFadeEffect::getManifest() {
    EffectManifestPointer pManifest(new EffectManifest());
    pManifest->setAddDryToWet(true);
    pManifest->setEffectRampsFromDry(true);

    pManifest->setId(getId());
    pManifest->setName(QObject::tr("Reverbed Fade"));
    pManifest->setAuthor("vespadj, based on Reverb by The Mixxx Team, CAPS Plugins");
    pManifest->setVersion("1.0");
    pManifest->setDescription(QObject::tr(
            "Reverb and finally fade the sound"));

    EffectManifestParameterPointer decay = pManifest->addParameter();
    decay->setId("decay");
    decay->setName(QObject::tr("Decay"));
    decay->setShortName(QObject::tr("Decay"));
    decay->setDescription(QObject::tr(
            "Lower decay values cause reverberations to fade out more quickly."));
    decay->setValueScaler(EffectManifestParameter::ValueScaler::Linear);
    decay->setUnitsHint(EffectManifestParameter::UnitsHint::Unknown);
    decay->setRange(0, 0.75, 1);

    EffectManifestParameterPointer bandwidth = pManifest->addParameter();
    bandwidth->setId("bandwidth");
    bandwidth->setName(QObject::tr("Bandwidth"));
    bandwidth->setShortName(QObject::tr("BW"));
    bandwidth->setDescription(QObject::tr(
            "Bandwidth of the low pass filter at the input.\n"
            "Higher values result in less attenuation of high frequencies."));
    bandwidth->setValueScaler(EffectManifestParameter::ValueScaler::Linear);
    bandwidth->setUnitsHint(EffectManifestParameter::UnitsHint::Unknown);
    bandwidth->setRange(0, 1, 1);

    EffectManifestParameterPointer damping = pManifest->addParameter();
    damping->setId("damping");
    damping->setName(QObject::tr("Damping"));
    damping->setShortName(QObject::tr("Damping"));
    damping->setDescription(
            QObject::tr("Higher damping values cause high frequencies to decay "
                        "more quickly than low frequencies."));
    damping->setValueScaler(EffectManifestParameter::ValueScaler::Linear);
    damping->setUnitsHint(EffectManifestParameter::UnitsHint::Unknown);
    damping->setRange(0, 0, 1);

    EffectManifestParameterPointer send = pManifest->addParameter();
    send->setId("send_amount");
    send->setName(QObject::tr("Send"));
    send->setShortName(QObject::tr("Send"));
    send->setDescription(QObject::tr(
            "How much of the signal to send in to the effect"));
    send->setValueScaler(EffectManifestParameter::ValueScaler::Linear);
    send->setUnitsHint(EffectManifestParameter::UnitsHint::Unknown);
    // send->setDefaultLinkType(EffectManifestParameter::LinkType::Linked);
    // send->setDefaultLinkInversion(EffectManifestParameter::LinkInversion::NotInverted);
    send->setRange(0, 1, 1);

    EffectManifestParameterPointer main = pManifest->addParameter();
    main->setId("main");
    main->setName(QObject::tr("main"));
    main->setShortName(QObject::tr("main"));
    main->setDescription(QObject::tr(
            "Main knob"));
    main->setValueScaler(EffectManifestParameter::ValueScaler::Linear);
    main->setUnitsHint(EffectManifestParameter::UnitsHint::Unknown);
    main->setDefaultLinkType(EffectManifestParameter::LinkType::Linked);
    main->setDefaultLinkInversion(EffectManifestParameter::LinkInversion::NotInverted);
    main->setRange(0, 0, 1);

    return pManifest;
}

void ReverbedFadeEffect::loadEngineEffectParameters(
        const QMap<QString, EngineEffectParameterPointer>& parameters) {
    m_pDecayParameter = parameters.value("decay");
    m_pBandWidthParameter = parameters.value("bandwidth");
    m_pDampingParameter = parameters.value("damping");
    m_pSendParameter = parameters.value("send_amount");
    m_pMainParameter = parameters.value("main");
}

double ReverbedFadeEffect::f_in(double x) {
    // Calculate input coefficient based on main
    if (x < 0.7) {
        return 1;
    } else if (x < 0.9) {
        return 1 - ((x - 0.7) / 0.2);
    } else {
        return 0;
    }
}

double ReverbedFadeEffect::f_wet(double x) {
    // Calculate wet coefficient based on main
    if (x <= 0) {
        return 0;
    } else if (x < 0.2) {
        return x * 3;
    } else if (x >= 0.2 && x < 0.7) {
        return x * 0.8 + 0.44;
    } else {
        return 1;
    }
}

void ReverbedFadeEffect::processChannel(
        ReverbedFadeGroupState* pState,
        const CSAMPLE* pInput,
        CSAMPLE* pOutput,
        const mixxx::EngineParameters& engineParameters,
        const EffectEnableState enableState,
        const GroupFeatureState& groupFeatures) {
    Q_UNUSED(groupFeatures);

    double main = m_pMainParameter->value();
    CSAMPLE_GAIN in_coeff = static_cast<CSAMPLE_GAIN>(f_in(main));
    double wet_coeff = f_wet(main);

    const auto decay = static_cast<sample_t>(m_pDecayParameter->value());
    const auto bandwidth = static_cast<sample_t>(m_pBandWidthParameter->value());
    const auto damping = static_cast<sample_t>(m_pDampingParameter->value());
    const auto sendCurrent = static_cast<sample_t>(m_pSendParameter->value() * wet_coeff);

    const auto samplesPerBuffer = engineParameters.samplesPerBuffer();

    // SampleUtil::addWithRampingGain(pOutput, pInput, 1.0, in_coeff, samplesPerBuffer); // always mute

    // Apply in_coeff to input signal
    SampleUtil::copyWithGain(pInputgained, pInput, in_coeff, samplesPerBuffer);
    

    // Reinitialize the effect when turning it on to prevent replaying the old buffer
    // from the last time the effect was enabled.
    // Also, update the sample rate if it has changed.
    if (enableState == EffectEnableState::Enabling ||
            pState->sampleRate != engineParameters.sampleRate()) {
        pState->reverb.init(engineParameters.sampleRate());
        pState->sampleRate = engineParameters.sampleRate();
    }

    pState->reverb.processBuffer(pInputgained,
            pOutput,
            engineParameters.samplesPerBuffer(),
            bandwidth,
            decay,
            damping,
            sendCurrent,
            pState->sendPrevious);

    // The ramping of the send parameter handles ramping when enabling, so
    // this effect must handle ramping to dry when disabling itself (instead
    // of being handled by EngineEffect::process).
    if (enableState == EffectEnableState::Disabling) {
        SampleUtil::applyRampingGain(pOutput, 1.0, 0.0, engineParameters.samplesPerBuffer());
        pState->sendPrevious = 0;
    } else {
        pState->sendPrevious = sendCurrent;
    }
}
