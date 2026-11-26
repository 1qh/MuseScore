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

#include "mixerchannelitem.h"

#include "translation.h"

using namespace mu::playback;
using namespace muse;
using namespace muse::audio;

static const std::string VSTI_EDITOR_ACTION("action://vst/instrument_editor");

static const std::string TRACK_ID_KEY("trackId");
static const std::string RESOURCE_ID_KEY("resourceId");

MixerChannelItem::MixerChannelItem(QObject* parent, Type type, bool outputOnly, audio::TrackId trackId)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this)),
    m_type(type),
    m_trackId(trackId),
    m_outputOnly(outputOnly)
{
    if (!m_outputOnly) {
        m_inputResourceItem = buildInputResourceItem();
    }

    m_panel = new ui::NavigationPanel(this);
    m_panel->setDirection(ui::NavigationPanel::Vertical);
    m_panel->setName("MixerChannelPanel " + QString::number(m_trackId));
    m_panel->accessible()->setName(muse::qtrc("playback", "Mixer channel panel %1").arg(m_trackId));
    m_panel->componentComplete();
}

MixerChannelItem::Type MixerChannelItem::type() const
{
    return m_type;
}

TrackId MixerChannelItem::trackId() const
{
    return m_trackId;
}

const mu::engraving::InstrumentTrackId& MixerChannelItem::instrumentTrackId() const
{
    return m_instrumentTrackId;
}

void MixerChannelItem::setInstrumentTrackId(const mu::engraving::InstrumentTrackId& instrumentTrackId)
{
    m_instrumentTrackId = instrumentTrackId;
}

QString MixerChannelItem::title() const
{
    return m_title;
}

float MixerChannelItem::volumeLevel() const
{
    return m_outParams.volume;
}

bool MixerChannelItem::solo() const
{
    return m_outParams.solo;
}

bool MixerChannelItem::muted() const
{
    return m_outParams.muted;
}

bool MixerChannelItem::forceMute() const
{
    return m_outParams.forceMute;
}

muse::ui::NavigationPanel* MixerChannelItem::panel() const
{
    return m_panel;
}

void MixerChannelItem::setPanelOrder(int panelOrder)
{
    m_panel->setOrder(panelOrder);
}

void MixerChannelItem::setPanelSection(muse::ui::INavigationSection* section)
{
    m_panel->setSection(section);
}

void MixerChannelItem::loadInputParams(const AudioInputParams& newParams)
{
    if (m_outputOnly) {
        return;
    }

    if (m_inputParams == newParams) {
        return;
    }

    m_inputParams = newParams;
    m_inputResourceItem->setParams(newParams);
}

void MixerChannelItem::loadOutputParams(const AudioOutputParams& newParams)
{
    if (!muse::RealIsEqual(m_outParams.volume, newParams.volume)) {
        m_outParams.volume = newParams.volume;
        emit volumeLevelChanged(newParams.volume);
    }

    if (m_outParams.solo != newParams.solo) {
        m_outParams.solo = newParams.solo;
        emit soloChanged();
    }

    if (m_outParams.muted != newParams.muted) {
        m_outParams.muted = newParams.muted;
        emit mutedChanged();
    }

    if (m_outParams.forceMute != newParams.forceMute) {
        m_outParams.forceMute = newParams.forceMute;
        emit forceMuteChanged();
    }

    m_outParams = newParams;
}

void MixerChannelItem::loadSoloMuteState(const notation::INotationSoloMuteState::SoloMuteState& newState)
{
    if (m_outParams.muted != newState.mute) {
        m_outParams.muted = newState.mute;
        emit mutedChanged();
    }

    if (m_outParams.solo != newState.solo) {
        m_outParams.solo = newState.solo;
        emit soloChanged();
    }
}

void MixerChannelItem::setTitle(QString title)
{
    if (m_title == title) {
        return;
    }

    m_title = title;
    emit titleChanged(m_title);
}

void MixerChannelItem::setVolumeLevel(float volumeLevel)
{
    if (qFuzzyCompare(m_outParams.volume, volumeLevel)) {
        return;
    }

    m_outParams.volume = volumeLevel;
    emit volumeLevelChanged(m_outParams.volume);
    emit outputParamsChanged(m_outParams);
}

void MixerChannelItem::setSolo(bool solo)
{
    if (m_outParams.solo == solo) {
        return;
    }

    m_outParams.solo = solo;

    notation::INotationSoloMuteState::SoloMuteState soloMuteState;
    soloMuteState.mute = m_outParams.muted;
    soloMuteState.solo = m_outParams.solo;

    emit soloMuteStateChanged(soloMuteState);
    emit soloChanged();

    if (solo && m_outParams.muted) {
        setMuted(false);
    }
}

void MixerChannelItem::setMuted(bool mute)
{
    if (m_outParams.muted == mute) {
        return;
    }

    m_outParams.muted = mute;

    notation::INotationSoloMuteState::SoloMuteState soloMuteState;
    soloMuteState.mute = m_outParams.muted;
    soloMuteState.solo = m_outParams.solo;

    emit soloMuteStateChanged(soloMuteState);
    emit mutedChanged();

    if (mute && m_outParams.solo) {
        setSolo(false);
    }
}

mu::notation::INotationPlaybackPtr MixerChannelItem::notationPlayback() const
{
    project::INotationProjectPtr project = context()->currentProject();
    return project ? project->masterNotation()->playback() : nullptr;
}

InputResourceItem* MixerChannelItem::buildInputResourceItem()
{
    InputResourceItem* newItem = new InputResourceItem(this);

    connect(newItem, &InputResourceItem::inputParamsChangeRequested, this, [this, newItem](const AudioResourceMeta& newMeta) {
        if (askAboutChangingSound()) {
            newItem->setParamsRecourceMeta(newMeta);
        }
    });

    connect(newItem, &InputResourceItem::inputParamsChanged, this, [this, newItem]() {
        bool audioSourceChanged = m_inputParams.type() != newItem->params().type();

        m_inputParams = newItem->params();
        emit inputParamsChanged(m_inputParams);

        if (!audioSourceChanged) {
            return;
        }

        bool auxParamsChanged = false;
        for (aux_channel_idx_t idx = 0; idx < static_cast<size_t>(m_outParams.auxSends.size()); ++idx) {
            const muse::String& soundId = m_inputParams.resourceMeta.attributeVal(PLAYBACK_SETUP_DATA_ATTRIBUTE);
            gain_t newAudioSignalAmount = configuration()->defaultAuxSendValue(idx, m_inputParams.type(), soundId);

            if (!muse::RealIsEqual(m_outParams.auxSends.at(idx).signalAmount, newAudioSignalAmount)) {
                m_outParams.auxSends.at(idx).signalAmount = newAudioSignalAmount;
                auxParamsChanged = true;
            }
        }

        if (auxParamsChanged) {
            emit outputParamsChanged(m_outParams);
        }
    });

    connect(newItem, &InputResourceItem::isBlankChanged, this, &MixerChannelItem::inputResourceItemChanged);

    connect(newItem, &InputResourceItem::nativeEditorViewLaunchRequested, this, [this, newItem]() {
        if (newItem->params().type() != AudioSourceType::Vsti) {
            return;
        }

        actions::ActionQuery aq(VSTI_EDITOR_ACTION);
        aq.addParam(TRACK_ID_KEY, Val(m_trackId));
        aq.addParam(RESOURCE_ID_KEY, Val(newItem->params().resourceMeta.id));

        openEditor(newItem, aq);
    });

    connect(newItem, &InputResourceItem::nativeEditorViewCloseRequested, this, [this, newItem]() {
        closeEditor(newItem);
    });

    return newItem;
}

void MixerChannelItem::openEditor(AbstractAudioResourceItem* item, const actions::ActionQuery& action)
{
    if (item->editorAction() != action) {
        if (item->editorAction().isValid()) {
            // make and send close
            actions::ActionQuery closeAction = item->editorAction();
            closeAction.addParam("operation", Val("close"));
            closeAction.addParam("sync", Val(true));
            dispatcher()->dispatch(closeAction);
        }
        // set new action
        item->setEditorAction(action);
    }

    dispatcher()->dispatch(action);
}

void MixerChannelItem::closeEditor(AbstractAudioResourceItem* item)
{
    // make and send close
    actions::ActionQuery closeAction = item->editorAction();
    closeAction.addParam("operation", Val("close"));
    closeAction.addParam("sync", Val(true));
    dispatcher()->dispatch(closeAction);

    item->setEditorAction(UriQuery());
}

bool MixerChannelItem::askAboutChangingSound()
{
    if (!configuration()->needToShowResetSoundFlagsWhenChangeSoundWarning()) {
        return true;
    }

    if (!notationPlayback()->hasSoundFlags({ m_instrumentTrackId })) {
        return true;
    }

    int changeBtn = int(IInteractive::Button::Apply);
    IInteractive::Options options = IInteractive::Option::WithIcon | IInteractive::Option::WithDontShowAgainCheckBox;
    IInteractive::ButtonDatas buttons = {
        interactive()->buttonData(IInteractive::Button::Cancel),
        IInteractive::ButtonData(changeBtn, muse::trc("playback", "Change sound"), true /*accent*/)
    };

    IInteractive::Result result = interactive()->warningSync(muse::trc("playback", "Are you sure you want to change this sound?"),
                                                             muse::trc("playback",
                                                                       "Sound flags on this instrument may be reset, but staff text will remain. This action can’t be undone."),
                                                             buttons, changeBtn, options);

    if (result.button() == changeBtn) {
        if (!result.showAgain()) {
            configuration()->setNeedToShowResetSoundFlagsWhenChangeSoundWarning(false);
        }

        return true;
    } else {
        return false;
    }
}

bool MixerChannelItem::outputOnly() const
{
    return m_outputOnly;
}

const AudioInputParams& MixerChannelItem::inputParams() const
{
    return m_inputParams;
}

const AudioOutputParams& MixerChannelItem::outputParams() const
{
    return m_outParams;
}

InputResourceItem* MixerChannelItem::inputResourceItem() const
{
    return m_inputResourceItem;
}

