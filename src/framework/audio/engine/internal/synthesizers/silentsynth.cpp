#include "silentsynth.h"

using namespace muse;
using namespace muse::audio;
using namespace muse::audio::synth;

SilentSynth::SilentSynth(const modularity::ContextPtr& iocCtx)
    : AbstractSynthesizer({}, iocCtx)
{
}

SilentSynth::~SilentSynth()
{
    m_playbackData.mainStream.disconnect(this);
}

std::string SilentSynth::name() const
{
    return "Silent";
}

AudioSourceType SilentSynth::type() const
{
    return AudioSourceType::Undefined;
}

bool SilentSynth::isValid() const
{
    return true;
}

void SilentSynth::setupSound(const mpe::PlaybackSetupData&)
{
}

void SilentSynth::setupEvents(const mpe::PlaybackData& playbackData)
{
    m_playbackData.mainStream.disconnect(this);
    m_playbackData = playbackData;
    m_playbackData.mainStream.onReceive(this, [this](const mpe::PlaybackEventsMap& events,
                                                  const mpe::DynamicLevelLayers& dynamics) {
        m_playbackData.originEvents = events;
        m_playbackData.dynamics = dynamics;
    });
}

const mpe::PlaybackData& SilentSynth::playbackData() const
{
    return m_playbackData;
}

void SilentSynth::flushSound()
{
}

bool SilentSynth::isActive() const
{
    return m_isActive;
}

void SilentSynth::setIsActive(bool active)
{
    m_isActive = active;
}

msecs_t SilentSynth::playbackPosition() const
{
    return m_playbackPosition;
}

void SilentSynth::setPlaybackPosition(msecs_t position)
{
    m_playbackPosition = position;
}

void SilentSynth::setOutputSpec(const OutputSpec& spec)
{
    unsigned int previousCount = audioChannelsCount();
    m_outputSpec = spec;
    if (previousCount != audioChannelsCount()) {
        m_audioChannelsCountChanged.send(audioChannelsCount());
    }
}

unsigned int SilentSynth::audioChannelsCount() const
{
    return m_outputSpec.audioChannelCount;
}

async::Channel<unsigned int> SilentSynth::audioChannelsCountChanged() const
{
    return m_audioChannelsCountChanged;
}

samples_t SilentSynth::process(float*, samples_t samplesPerChannel)
{
    if (m_isActive && m_outputSpec.sampleRate > 0) {
        m_playbackPosition += samplesToMsecs(samplesPerChannel, m_outputSpec.sampleRate);
    }
    return 0;
}
