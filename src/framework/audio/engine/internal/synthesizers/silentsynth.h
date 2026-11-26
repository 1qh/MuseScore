#pragma once

#include "abstractsynthesizer.h"

namespace muse::audio::synth {
class SilentSynth : public AbstractSynthesizer
{
public:
    explicit SilentSynth(const modularity::ContextPtr& iocCtx);
    ~SilentSynth() override;

    std::string name() const override;
    AudioSourceType type() const override;
    bool isValid() const override;

    const mpe::PlaybackData& playbackData() const override;
    void flushSound() override;

    bool isActive() const override;
    void setIsActive(bool active) override;
    msecs_t playbackPosition() const override;
    void setPlaybackPosition(msecs_t position) override;

    void setOutputSpec(const OutputSpec& spec) override;
    unsigned int audioChannelsCount() const override;
    async::Channel<unsigned int> audioChannelsCountChanged() const override;
    samples_t process(float* buffer, samples_t samplesPerChannel) override;

protected:
    void setupSound(const mpe::PlaybackSetupData& setupData) override;
    void setupEvents(const mpe::PlaybackData& playbackData) override;

private:
    mpe::PlaybackData m_playbackData;
    OutputSpec m_outputSpec;
    async::Channel<unsigned int> m_audioChannelsCountChanged;
    msecs_t m_playbackPosition = 0;
    bool m_isActive = false;
};
}
