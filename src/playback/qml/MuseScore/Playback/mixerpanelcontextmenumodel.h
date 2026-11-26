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

#pragma once

#include "actions/actionable.h"
#include "uicomponents/qml/Muse/UiComponents/abstractmenumodel.h"

#include "actions/iactionsdispatcher.h"
#include "playback/iplaybackconfiguration.h"

#include "playback/playbacktypes.h"

namespace mu::playback {
class MixerPanelContextMenuModel : public muse::uicomponents::AbstractMenuModel, public muse::actions::Actionable
{
    Q_OBJECT

    Q_PROPERTY(bool labelsSectionVisible READ labelsSectionVisible NOTIFY labelsSectionVisibleChanged)
    Q_PROPERTY(bool soundSectionVisible READ soundSectionVisible NOTIFY soundSectionVisibleChanged)
    Q_PROPERTY(bool faderSectionVisible READ faderSectionVisible NOTIFY faderSectionVisibleChanged)
    Q_PROPERTY(bool muteAndSoloSectionVisible READ muteAndSoloSectionVisible NOTIFY muteAndSoloSectionVisibleChanged)
    Q_PROPERTY(bool titleSectionVisible READ titleSectionVisible NOTIFY titleSectionVisibleChanged)

    QML_ELEMENT

    muse::GlobalInject<playback::IPlaybackConfiguration> configuration;
    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher = { this };

public:
    explicit MixerPanelContextMenuModel(QObject* parent = nullptr);

    bool labelsSectionVisible() const;
    bool soundSectionVisible() const;
    bool faderSectionVisible() const;
    bool muteAndSoloSectionVisible() const;
    bool titleSectionVisible() const;

    Q_INVOKABLE void load() override;

signals:
    void labelsSectionVisibleChanged();
    void soundSectionVisibleChanged();
    void faderSectionVisibleChanged();
    void muteAndSoloSectionVisibleChanged();
    void titleSectionVisibleChanged();

private:
    bool isSectionVisible(MixerSectionType sectionType) const;

    muse::uicomponents::MenuItem* buildSectionVisibleItem(MixerSectionType sectionType);

    void toggleMixerSection(const muse::actions::ActionData& args);

    void emitMixerSectionVisibilityChanged(MixerSectionType sectionType);

    void setViewMenuItemChecked(const QString& itemId, bool checked);
};
}
