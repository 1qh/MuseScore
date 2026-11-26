/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "playbackconfiguration.h"

#include <QJsonDocument>

#include "settings.h"
#include "project/audioinputparamsjson.h"
#include "types/string.h"

#include "playbacktypes.h"

#include "log.h"

using namespace mu;
using namespace mu::playback;
using namespace muse;
using namespace muse::audio;

static const std::string moduleName("playback");

static const Settings::Key PLAYBACK_CURSOR_TYPE_KEY(moduleName, "application/playback/cursorType");
static const Settings::Key PLAY_NOTES_WHEN_EDITING(moduleName, "score/note/playOnClick");
static const Settings::Key PLAY_NOTES_ON_MIDI_INPUT(moduleName, "score/note/playOnMidiInput");
static const Settings::Key PLAY_CHORD_WHEN_EDITING(moduleName, "score/chord/playOnAddNote");
static const Settings::Key PLAY_HARMONY_WHEN_EDITING(moduleName, "score/harmony/play/onedit");

static const Settings::Key SOUND_PRESETS_MULTI_SELECTION_KEY(moduleName, "application/playback/soundPresetsMultiSelectionEnabled");

static const Settings::Key MIXER_LABELS_SECTION_VISIBLE_KEY(moduleName, "playback/mixer/labelsSectionVisible");
static const Settings::Key MIXER_SOUND_SECTION_VISIBLE_KEY(moduleName, "playback/mixer/soundSectionVisible");
static const Settings::Key MIXER_FADER_SECTION_VISIBLE_KEY(moduleName, "playback/mixer/faderSectionVisible");
static const Settings::Key MIXER_MUTE_AND_SOLO_SECTION_VISIBLE_KEY(moduleName, "playback/mixer/muteAndSoloSectionVisible");
static const Settings::Key MIXER_TITLE_SECTION_VISIBLE_KEY(moduleName, "playback/mixer/titleSectionVisible");

static const Settings::Key MIXER_RESET_SOUND_FLAGS_WHEN_CHANGE_SOUND_WARNING(moduleName,
                                                                             "playback/mixer/needToShowAboutResetSoundFlagsWhwnChangeSoundWarning");
static const Settings::Key MIXER_RESET_SOUND_FLAGS_WHEN_CHANGE_PLAYBACK_PROFILE_WARNING(moduleName,
                                                                                        "playback/mixer/needToShowAboutResetSoundFlagsWhwnChangePlaybackProfileWarning");

static const Settings::Key MUTE_HIDDEN_INSTRUMENTS(moduleName, "playback/mixer/muteHiddenInstruments");

static const Settings::Key DEFAULT_SOUND_PROFILE_FOR_NEW_PROJECTS(moduleName, "playback/profiles/defaultProfileName");
static const Settings::Key REMEMBERED_TRACK_INPUTS_KEY(moduleName, "playback/mixer/rememberedTrackInputs");
static const SoundProfileName BASIC_PROFILE_NAME(u"MuseScore Basic");
static const SoundProfileName MUSESOUNDS_PROFILE_NAME(u"MuseSounds");
static const SoundProfileName COMPAT_MUSESOUNDS_PROFILE_NAME(u"Muse Sounds");

static Settings::Key mixerSectionVisibleKey(MixerSectionType sectionType)
{
    switch (sectionType) {
    case MixerSectionType::Labels: return MIXER_LABELS_SECTION_VISIBLE_KEY;
    case MixerSectionType::Sound: return MIXER_SOUND_SECTION_VISIBLE_KEY;
    case MixerSectionType::Fader: return MIXER_FADER_SECTION_VISIBLE_KEY;
    case MixerSectionType::MuteAndSolo: return MIXER_MUTE_AND_SOLO_SECTION_VISIBLE_KEY;
    case MixerSectionType::Title: return MIXER_TITLE_SECTION_VISIBLE_KEY;
    case MixerSectionType::Unknown: break;
    }

    return Settings::Key();
}

void PlaybackConfiguration::init()
{
    settings()->setDefaultValue(REMEMBERED_TRACK_INPUTS_KEY, Val("{}"));
    settings()->setDefaultValue(PLAY_NOTES_WHEN_EDITING, Val(true));
    settings()->valueChanged(PLAY_NOTES_WHEN_EDITING).onReceive(this, [this](const Val&) {
        m_playNotesWhenEditingChanged.notify();
    });
    settings()->setDefaultValue(PLAY_CHORD_WHEN_EDITING, Val(true));
    settings()->valueChanged(PLAY_CHORD_WHEN_EDITING).onReceive(this, [this](const Val& val) {
        m_playChordWhenEditingChanged.send(val.toBool());
    });
    settings()->setDefaultValue(PLAY_HARMONY_WHEN_EDITING, Val(true));
    settings()->valueChanged(PLAY_HARMONY_WHEN_EDITING).onReceive(this, [this](const Val& val) {
        m_playHarmonyWhenEditingChanged.send(val.toBool());
    });
    settings()->setDefaultValue(PLAY_NOTES_ON_MIDI_INPUT, Val(true));
    settings()->valueChanged(PLAY_NOTES_ON_MIDI_INPUT).onReceive(this, [this](const Val& val) {
        m_playNotesOnMidiInputChanged.send(val.toBool());
    });
    settings()->setDefaultValue(PLAYBACK_CURSOR_TYPE_KEY, Val(PlaybackCursorType::STEPPED));
    settings()->setDefaultValue(SOUND_PRESETS_MULTI_SELECTION_KEY, Val(false));
    settings()->setDefaultValue(MIXER_RESET_SOUND_FLAGS_WHEN_CHANGE_SOUND_WARNING, Val(true));
    settings()->setDefaultValue(MIXER_RESET_SOUND_FLAGS_WHEN_CHANGE_PLAYBACK_PROFILE_WARNING, Val(true));

    for (MixerSectionType sectionType : allMixerSectionTypes()) {
        settings()->setDefaultValue(mixerSectionVisibleKey(sectionType), Val(true));
        settings()->valueChanged(mixerSectionVisibleKey(sectionType)).onReceive(this, [this, sectionType](const Val& val) {
            m_isMixerSectionVisibleChanged.send(sectionType, val.toBool());
        });
    }

    settings()->setDefaultValue(MUTE_HIDDEN_INSTRUMENTS, Val(true));
    settings()->valueChanged(MUTE_HIDDEN_INSTRUMENTS).onReceive(nullptr, [this](const Val& mute) {
        m_muteHiddenInstrumentsChanged.send(mute.toBool());
    });

    settings()->setDefaultValue(DEFAULT_SOUND_PROFILE_FOR_NEW_PROJECTS, Val(fallbackSoundProfileStr().toStdString()));
}

bool PlaybackConfiguration::playNotesWhenEditing() const
{
    return settings()->value(PLAY_NOTES_WHEN_EDITING).toBool();
}

void PlaybackConfiguration::setPlayNotesWhenEditing(bool value)
{
    settings()->setSharedValue(PLAY_NOTES_WHEN_EDITING, Val(value));
}

muse::async::Notification PlaybackConfiguration::playNotesWhenEditingChanged() const
{
    return m_playNotesWhenEditingChanged;
}

bool PlaybackConfiguration::playChordWhenEditing() const
{
    return settings()->value(PLAY_CHORD_WHEN_EDITING).toBool();
}

void PlaybackConfiguration::setPlayChordWhenEditing(bool value)
{
    settings()->setSharedValue(PLAY_CHORD_WHEN_EDITING, Val(value));
}

async::Channel<bool> PlaybackConfiguration::playChordWhenEditingChanged() const
{
    return m_playChordWhenEditingChanged;
}

bool PlaybackConfiguration::playHarmonyWhenEditing() const
{
    return settings()->value(PLAY_HARMONY_WHEN_EDITING).toBool();
}

void PlaybackConfiguration::setPlayHarmonyWhenEditing(bool value)
{
    settings()->setSharedValue(PLAY_HARMONY_WHEN_EDITING, Val(value));
}

async::Channel<bool> PlaybackConfiguration::playHarmonyWhenEditingChanged() const
{
    return m_playHarmonyWhenEditingChanged;
}

bool PlaybackConfiguration::playNotesOnMidiInput() const
{
    return settings()->value(PLAY_NOTES_ON_MIDI_INPUT).toBool();
}

void PlaybackConfiguration::setPlayNotesOnMidiInput(bool value)
{
    settings()->setSharedValue(PLAY_NOTES_ON_MIDI_INPUT, Val(value));
}

muse::async::Channel<bool> PlaybackConfiguration::playNotesOnMidiInputChanged() const
{
    return m_playNotesOnMidiInputChanged;
}

PlaybackCursorType PlaybackConfiguration::cursorType() const
{
    return settings()->value(PLAYBACK_CURSOR_TYPE_KEY).toEnum<PlaybackCursorType>();
}

bool PlaybackConfiguration::isMixerSectionVisible(MixerSectionType sectionType) const
{
    return settings()->value(mixerSectionVisibleKey(sectionType)).toBool();
}

void PlaybackConfiguration::setMixerSectionVisible(MixerSectionType sectionType, bool visible)
{
    settings()->setSharedValue(mixerSectionVisibleKey(sectionType), Val(visible));
}

muse::async::Channel<MixerSectionType, bool> PlaybackConfiguration::isMixerSectionVisibleChanged() const
{
    return m_isMixerSectionVisibleChanged;
}

gain_t PlaybackConfiguration::defaultAuxSendValue(aux_channel_idx_t index, AudioSourceType sourceType,
                                                  const muse::String& instrumentSoundId) const
{
    TRACEFUNC;

    constexpr gain_t DEFAULT_VALUE = 0.30f;

    if (sourceType == AudioSourceType::MuseSampler) {
        if (index == REVERB_CHANNEL_IDX) {
            float lvl = musesamplerInfo()->defaultReverbLevel(instrumentSoundId);
            return muse::RealIsNull(lvl) ? DEFAULT_VALUE : lvl;
        }
    } else if (sourceType == AudioSourceType::Vsti) {
        return 0.f;
    }

    return DEFAULT_VALUE;
}

bool PlaybackConfiguration::muteHiddenInstruments() const
{
    return settings()->value(MUTE_HIDDEN_INSTRUMENTS).toBool();
}

void PlaybackConfiguration::setMuteHiddenInstruments(bool mute)
{
    settings()->setSharedValue(MUTE_HIDDEN_INSTRUMENTS, Val(mute));
}

muse::async::Channel<bool> PlaybackConfiguration::muteHiddenInstrumentsChanged() const
{
    return m_muteHiddenInstrumentsChanged;
}

const SoundProfileName& PlaybackConfiguration::basicSoundProfileName() const
{
    return BASIC_PROFILE_NAME;
}

const SoundProfileName& PlaybackConfiguration::museSoundsProfileName() const
{
    return MUSESOUNDS_PROFILE_NAME;
}

const SoundProfileName& PlaybackConfiguration::compatMuseSoundsProfileName() const
{
    return COMPAT_MUSESOUNDS_PROFILE_NAME;
}

SoundProfileName PlaybackConfiguration::defaultProfileForNewProjects() const
{
    return muse::String::fromStdString(settings()->value(DEFAULT_SOUND_PROFILE_FOR_NEW_PROJECTS).toString());
}

void PlaybackConfiguration::setDefaultProfileForNewProjects(const SoundProfileName& name)
{
    settings()->setSharedValue(DEFAULT_SOUND_PROFILE_FOR_NEW_PROJECTS, Val(name.toStdString()));
}

bool PlaybackConfiguration::soundPresetsMultiSelectionEnabled() const
{
    return settings()->value(SOUND_PRESETS_MULTI_SELECTION_KEY).toBool();
}

void PlaybackConfiguration::setSoundPresetsMultiSelectionEnabled(bool enabled)
{
    settings()->setSharedValue(SOUND_PRESETS_MULTI_SELECTION_KEY, Val(enabled));
}

bool PlaybackConfiguration::needToShowResetSoundFlagsWhenChangeSoundWarning() const
{
    return settings()->value(MIXER_RESET_SOUND_FLAGS_WHEN_CHANGE_SOUND_WARNING).toBool();
}

void PlaybackConfiguration::setNeedToShowResetSoundFlagsWhenChangeSoundWarning(bool show)
{
    settings()->setSharedValue(MIXER_RESET_SOUND_FLAGS_WHEN_CHANGE_SOUND_WARNING, Val(show));
}

bool PlaybackConfiguration::needToShowResetSoundFlagsWhenChangePlaybackProfileWarning() const
{
    return settings()->value(MIXER_RESET_SOUND_FLAGS_WHEN_CHANGE_PLAYBACK_PROFILE_WARNING).toBool();
}

void PlaybackConfiguration::setNeedToShowResetSoundFlagsWhenChangePlaybackProfileWarning(bool show)
{
    settings()->setSharedValue(MIXER_RESET_SOUND_FLAGS_WHEN_CHANGE_PLAYBACK_PROFILE_WARNING, Val(show));
}

bool PlaybackConfiguration::shouldMeasureInputLag() const
{
    return audioConfiguration()->shouldMeasureInputLag();
}

const SoundProfileName& PlaybackConfiguration::fallbackSoundProfileStr() const
{
    if (musesamplerInfo() && musesamplerInfo()->isLoaded()) {
        return MUSESOUNDS_PROFILE_NAME;
    }

    return BASIC_PROFILE_NAME;
}

AudioInputParams PlaybackConfiguration::rememberedTrackInputParams(const std::string& slot) const
{
    const auto slotInputs = QJsonDocument::fromJson(
        QByteArray::fromStdString(settings()->value(REMEMBERED_TRACK_INPUTS_KEY).toString())).object();
    AudioInputParams params = project::audioSettingsJson::inputParamsFromJson(slotInputs.value(QString::fromStdString(slot)).toObject());
    return params.isValid() ? params : AudioInputParams {};
}

void PlaybackConfiguration::setRememberedTrackInputParams(const std::string& slot, const AudioInputParams& params)
{
    if (slot.empty() || !params.isValid()) {
        return;
    }
    auto slotInputs = QJsonDocument::fromJson(
        QByteArray::fromStdString(settings()->value(REMEMBERED_TRACK_INPUTS_KEY).toString())).object();
    slotInputs.insert(QString::fromStdString(slot), project::audioSettingsJson::inputParamsToJson(params));
    settings()->setSharedValue(REMEMBERED_TRACK_INPUTS_KEY, Val(QJsonDocument(slotInputs).toJson(QJsonDocument::Compact).toStdString()));
}
