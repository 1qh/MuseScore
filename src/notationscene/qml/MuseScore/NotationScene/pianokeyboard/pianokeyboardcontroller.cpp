/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2022 MuseScore Limited and others
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
#include "pianokeyboardcontroller.h"

#include <algorithm>

#include "log.h"

#include "notation/imasternotation.h"
#include "notation/inotationparts.h"
#include "notation/inotationsolomutestate.h"

using namespace mu::notation;

PianoKeyboardController::PianoKeyboardController(const muse::modularity::ContextPtr& iocCtx)
    : muse::Contextable(iocCtx)
{
}

void PianoKeyboardController::init()
{
    if (m_isInitialized) {
        return;
    }
    m_isInitialized = true;
    onNotationChanged();

    auto globalContext = context();
    if (!globalContext) {
        return;
    }
    globalContext->currentNotationChanged().onNotify(this, [this]() {
        onNotationChanged();
    });

    globalContext->currentMasterNotationChanged().onNotify(this, [this]() { onMasterNotationChanged(); });
    auto playbackState = globalContext->playbackState();
    if (!playbackState) {
        return;
    }
    playbackState->playbackStatusChanged().onReceive(this, [this](const muse::audio::PlaybackStatus& status) {
        onPlaybackStatusChanged(status);
    });
    playbackState->playbackPositionChanged().onReceive(this, [this](const muse::audio::secs_t& position) {
        onPlaybackPositionChanged(position);
    });
    auto controller = playbackController();
    if (controller) {
        controller->playbackInitedChanged().onReceive(this, [this](const bool&) { refreshPlaybackKeys(); });
        controller->trackAdded().onReceive(this, [this](const muse::audio::TrackId&) { refreshPlaybackKeys(); });
        controller->trackRemoved().onReceive(this, [this](const muse::audio::TrackId&) { refreshPlaybackKeys(); });
    }
    auto configuration = notationConfiguration();
    if (configuration) {
        configuration->isPlayChordSymbolsChanged().onNotify(this, [this]() { refreshPlaybackKeys(); });
    }
    onPlaybackStatusChanged(playbackState->playbackStatus());
}

void PianoKeyboardController::setPlaybackTrackingEnabled(bool enabled)
{
    if (m_isPlaybackTrackingEnabled == enabled) {
        return;
    }
    m_isPlaybackTrackingEnabled = enabled;
    if (!enabled) {
        replacePlaybackKeys({});
        m_lastPlaybackPosition.reset();
        return;
    }
    if (!m_isInitialized || m_playbackStatus != muse::audio::PlaybackStatus::Running || m_isWaitingForCountIn) {
        return;
    }
    auto globalContext = context();
    auto playbackState = globalContext ? globalContext->playbackState() : nullptr;
    if (playbackState) {
        updatePlaybackKeys(playbackState->playbackPosition(), true);
    }
}

size_t PianoKeyboardController::keyPlaybackLayer(piano_key_t key) const
{
    if (key >= m_playbackKeys.size()) {
        return 0;
    }
    if (m_pressedKey == key) {
        return livePlaybackLayer();
    }
    //! NOTE The owner's own notes keep their colour whether or not the score plays
    for (const auto& channel : m_liveKeys) {
        if (channel[key]) {
            return livePlaybackLayer();
        }
    }
    if (m_playbackKeys[key] || m_playbackStatus == muse::audio::PlaybackStatus::Running) {
        return m_playbackKeys[key];
    }
    //! NOTE Outside playback a selected note lights in its own staff's colour, never in the theme's grey
    const auto selected = m_selectionLayers.find(key);
    return selected == m_selectionLayers.cend() ? 0 : selected->second;
}

KeyState PianoKeyboardController::keyState(piano_key_t key) const
{
    if (m_playbackStatus == muse::audio::PlaybackStatus::Running) {
        return KeyState::None;
    }
    if (m_keys.find(key) != m_keys.cend()) {
        return KeyState::Selected;
    }
    if (m_otherNotesInChord.find(key) != m_otherNotesInChord.cend()) {
        return KeyState::OtherInSelectedChord;
    }
    return KeyState::None;
}

muse::async::Notification PianoKeyboardController::keyStatesChanged() const
{
    return m_keyStatesChanged;
}

bool PianoKeyboardController::isFromMidi() const
{
    return m_isFromMidi;
}

std::optional<piano_key_t> PianoKeyboardController::pressedKey() const
{
    return m_pressedKey;
}

void PianoKeyboardController::setPressedKey(std::optional<piano_key_t> key)
{
    if (m_pressedKey == key) {
        return;
    }

    if (m_pressedKey.has_value()) {
        sendNoteOff(m_pressedKey.value());
    }

    if (key.has_value()) {
        sendNoteOn(key.value());
    }

    m_pressedKey = key;
    m_keyStatesChanged.notify();
}

void PianoKeyboardController::setHoveredKey(std::optional<piano_key_t> key)
{
    if (m_hoveredKey == key) {
        return;
    }

    auto notation = currentNotation();
    if (notation && notation->interaction()->noteInput()->isNoteInputMode()) {
        if (key.has_value()) {
            notation->interaction()->showShadowNoteForMidiPitch(key.value());
        } else {
            notation->interaction()->hideShadowNote();
        }
    }

    m_hoveredKey = key;
}

void PianoKeyboardController::onNotationChanged()
{
    if (m_notation) {
        m_notation->midiInput()->noteEventsReceived().disconnect(this);
        m_notation->midiInput()->notesReceived().disconnect(this);
        m_notation->interaction()->selectionChanged().disconnect(this);
        m_notation->soloMuteState()->trackSoloMuteStateChanged().disconnect(this);
        if (auto parts = m_notation->parts()) {
            parts->partsChanged().disconnect(this);
            parts->scoreOrderChanged().disconnect(this);
        }
    }
    m_liveKeys = {};
    m_notation = currentNotation();
    if (m_notation) {
        m_notation->soloMuteState()->trackSoloMuteStateChanged().onReceive(this, [this](const engraving::InstrumentTrackId&, const INotationSoloMuteState::SoloMuteState&) {
            refreshPlaybackKeys();
        });
    }
    if (auto notation = currentNotation()) {
        if (auto parts = notation->parts()) {
            parts->partsChanged().onNotify(this, [this]() { refreshPlaybackKeys(); });
            parts->scoreOrderChanged().onNotify(this, [this]() { refreshPlaybackKeys(); });
        }
        notation->interaction()->selectionChanged().onNotify(this, [this]() {
            auto notation = currentNotation();
            if (!notation) {
                return;
            }

            auto selection = notation->interaction()->selection();
            if (selection->isNone()) {
                updateNotesKeys({}, false);
                refreshPlaybackKeys();
                return;
            }

            std::vector<const Note*> notes;
            for (const mu::engraving::Note* note : selection->notes()) {
                notes.push_back(note);
            }

            updateNotesKeys(notes, false);
            refreshPlaybackKeys();
        }, Asyncable::Mode::SetReplace /* FIXME */);

        notation->midiInput()->noteEventsReceived().onReceive(this, [this](const muse::midi::Event& event) {
            const auto pitch = event.note();
            const auto channel = event.channel();
            if (channel >= m_liveKeys.size() || pitch >= m_liveKeys[channel].size()) {
                return;
            }
            const bool pressed = event.opcode() == muse::midi::Event::Opcode::NoteOn && event.velocity7() != 0;
            if (m_liveKeys[channel][pitch] == pressed) {
                return;
            }
            m_liveKeys[channel][pitch] = pressed;
            m_keyStatesChanged.notify();
        });

        notation->midiInput()->notesReceived().onReceive(this, [this](const std::vector<const Note*>& notes) {
            updateNotesKeys(notes, true);
        }, Asyncable::Mode::SetReplace /* FIXME */);
    }
    refreshPlaybackKeys();
}

void PianoKeyboardController::updateNotesKeys(const std::vector<const Note*>& receivedNotes, bool isFromMidi)
{
    std::unordered_set<piano_key_t> newKeys;
    std::unordered_set<piano_key_t> newOtherNotesInChord;
    std::unordered_map<piano_key_t, size_t> newSelectionLayers;

    const bool useWrittenPitch = notationConfiguration()->midiUseWrittenPitch().val;

    for (const mu::engraving::Note* note : receivedNotes) {
        // the owner's own notes are layer 1; a note of the score takes its staff's layer, as playback does
        const mu::engraving::staff_idx_t staff = note->staffIdx();
        const size_t layer = isFromMidi ? livePlaybackLayer() : staffPlaybackLayer(staff == muse::nidx ? 0 : staff);
        newKeys.insert(static_cast<piano_key_t>(useWrittenPitch ? note->epitch() : note->ppitch()));
        newSelectionLayers.emplace(static_cast<piano_key_t>(useWrittenPitch ? note->epitch() : note->ppitch()), layer);
        for (const mu::engraving::Note* otherNote : note->chord()->notes()) {
            const auto other = static_cast<piano_key_t>(useWrittenPitch ? otherNote->epitch() : otherNote->ppitch());
            newOtherNotesInChord.insert(other);
            newSelectionLayers.emplace(other, layer);
        }
    }

    if (newKeys == m_keys && newOtherNotesInChord == m_otherNotesInChord && isFromMidi == m_isFromMidi) {
        return;
    }
    m_keys = std::move(newKeys);
    m_otherNotesInChord = std::move(newOtherNotesInChord);
    m_selectionLayers = std::move(newSelectionLayers);
    m_isFromMidi = isFromMidi;
    m_keyStatesChanged.notify();
}

void PianoKeyboardController::onMasterNotationChanged()
{
    m_lastPlaybackPosition.reset();
    m_playbackTrackLayers.reset();
    refreshPlaybackKeys();
}

void PianoKeyboardController::onPlaybackStatusChanged(muse::audio::PlaybackStatus status)
{
    if (m_playbackStatus == status) return;
    m_playbackStatus = status;
    m_lastPlaybackPosition.reset();
    m_playbackTrackLayers.reset();
    const bool liveKeysChanged = std::any_of(m_liveKeys.cbegin(), m_liveKeys.cend(), [](const auto& channel) {
        return std::any_of(channel.cbegin(), channel.cend(), [](bool pressed) { return pressed; });
    });
    const bool selectionChanged = !m_keys.empty() || !m_otherNotesInChord.empty() || liveKeysChanged;
    if (status != muse::audio::PlaybackStatus::Running) {
        m_isWaitingForCountIn = false;
        m_countInStartPosition.reset();
        if ((replacePlaybackKeys({}) || selectionChanged) && m_isPlaybackTrackingEnabled) m_keyStatesChanged.notify();
        return;
    }
    auto configuration = notationConfiguration();
    m_isWaitingForCountIn = configuration && configuration->isCountInEnabled();
    auto globalContext = context();
    auto playbackState = globalContext ? globalContext->playbackState() : nullptr;
    if (m_isWaitingForCountIn && playbackState) m_countInStartPosition = playbackState->playbackPosition();
    if ((replacePlaybackKeys({}) || selectionChanged) && m_isPlaybackTrackingEnabled) m_keyStatesChanged.notify();
    if (!m_isWaitingForCountIn && m_isPlaybackTrackingEnabled && playbackState) updatePlaybackKeys(playbackState->playbackPosition(), true);
}

void PianoKeyboardController::onPlaybackPositionChanged(muse::audio::secs_t position)
{
    if (m_playbackStatus != muse::audio::PlaybackStatus::Running) return;
    if (m_isWaitingForCountIn) {
        if (m_countInStartPosition == position) return;
        m_isWaitingForCountIn = false;
        m_countInStartPosition.reset();
        m_lastPlaybackPosition.reset();
    }
    updatePlaybackKeys(position);
}

void PianoKeyboardController::refreshPlaybackKeys()
{
    m_playbackTrackLayers.reset();
    if (!m_isPlaybackTrackingEnabled || m_playbackStatus != muse::audio::PlaybackStatus::Running || m_isWaitingForCountIn) return;
    auto globalContext = context();
    auto playbackState = globalContext ? globalContext->playbackState() : nullptr;
    if (playbackState) updatePlaybackKeys(playbackState->playbackPosition(), true);
}

void PianoKeyboardController::updatePlaybackKeys(muse::audio::secs_t position, bool force)
{
    if (!m_isPlaybackTrackingEnabled || m_playbackStatus != muse::audio::PlaybackStatus::Running || m_isWaitingForCountIn
        || (!force && m_lastPlaybackPosition == position)) return;
    m_lastPlaybackPosition = position;
    auto globalContext = context();
    auto masterNotation = globalContext ? globalContext->currentMasterNotation() : nullptr;
    auto notationPlayback = masterNotation ? masterNotation->playback() : nullptr;
    auto controller = playbackController();
    if (notationPlayback && controller) {
        if (!m_playbackTrackLayers) {
            m_playbackTrackLayers = notationPlayback->playbackTrackLayers(controller->audibleInstrumentTrackIds());
        }
        updatePlaybackLayers(notationPlayback, *m_playbackTrackLayers, position);
    } else if (replacePlaybackKeys({})) {
        m_keyStatesChanged.notify();
    }
}

void PianoKeyboardController::setExportPlaybackPosition(const INotationPlaybackPtr& playback,
                                                       const engraving::InstrumentTrackIdSet& tracks,
                                                       std::optional<muse::audio::secs_t> position)
{
    m_playbackStatus = muse::audio::PlaybackStatus::Running;
    m_pressedKey.reset();
    m_liveKeys = {};
    m_keys.clear();
    m_otherNotesInChord.clear();
    m_selectionLayers.clear();
    if (position) {
        updatePlaybackLayers(playback, playback->playbackTrackLayers(tracks), *position);
    } else if (replacePlaybackKeys({})) {
        m_keyStatesChanged.notify();
    }
}

void PianoKeyboardController::updatePlaybackLayers(const INotationPlaybackPtr& notationPlayback,
                                                  const std::vector<engraving::InstrumentTrackIdSet>& layers,
                                                  muse::audio::secs_t position)
{
    PianoKeyLayers playbackKeys {};
    for (size_t layer = 0; layer < layers.size(); ++layer) {
        const auto& tracks = layers[layer];
        if (tracks.empty()) {
            continue;
        }
        const auto pitches = notationPlayback->activePlaybackPitches(position, tracks);
        const size_t playbackLayer = layer + 1 == layers.size()
                                     ? chordPlaybackLayer(layer) : staffPlaybackLayer(layer);
        applyPianoKeyLayer(playbackKeys, pitches, playbackLayer);
    }
    if (replacePlaybackKeys(std::move(playbackKeys))) m_keyStatesChanged.notify();
}

bool PianoKeyboardController::replacePlaybackKeys(PianoKeyLayers keys)
{
    if (m_playbackKeys == keys) return false;
    m_playbackKeys = std::move(keys);
    return true;
}

void PianoKeyboardController::sendNoteOn(piano_key_t key)
{
    auto notation = currentNotation();
    if (!notation) {
        return;
    }

    muse::midi::Event ev;
    ev.setMessageType(muse::midi::Event::MessageType::ChannelVoice10);
    ev.setOpcode(muse::midi::Event::Opcode::NoteOn);
    ev.setNote(key);
    ev.setVelocity7(80);

    notation->midiInput()->onMidiEventReceived(ev);
}

void PianoKeyboardController::sendNoteOff(piano_key_t key)
{
    auto notation = currentNotation();
    if (!notation) {
        return;
    }

    muse::midi::Event ev;
    ev.setMessageType(muse::midi::Event::MessageType::ChannelVoice10);
    ev.setOpcode(muse::midi::Event::Opcode::NoteOff);
    ev.setNote(key);

    notation->midiInput()->onMidiEventReceived(ev);
}

INotationPtr PianoKeyboardController::currentNotation() const
{
    auto globalContext = context();
    return globalContext ? globalContext->currentNotation() : nullptr;
}
