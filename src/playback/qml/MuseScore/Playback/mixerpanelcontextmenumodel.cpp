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

#include "mixerpanelcontextmenumodel.h"

#include "types/translatablestring.h"

using namespace mu;
using namespace mu::playback;
using namespace muse;
using namespace muse::ui;
using namespace muse::uicomponents;
using namespace muse::actions;

static const ActionCode TOGGLE_MIXER_SECTION_ACTION("toggle-mixer-section");

static const QString VIEW_MENU_ID("view-menu");

static TranslatableString mixerSectionTitle(MixerSectionType type)
{
    switch (type) {
    case MixerSectionType::Labels: return TranslatableString("playback", "Labels");
    case MixerSectionType::Sound: return TranslatableString("playback", "Sound");
    case MixerSectionType::Fader: return TranslatableString("playback", "Fader");
    case MixerSectionType::MuteAndSolo: return TranslatableString("playback", "Mute and solo");
    case MixerSectionType::Title: return TranslatableString("playback", "Name");
    case MixerSectionType::Unknown: break;
    }

    return {};
}

MixerPanelContextMenuModel::MixerPanelContextMenuModel(QObject* parent)
    : AbstractMenuModel(parent)
{
}

bool MixerPanelContextMenuModel::labelsSectionVisible() const
{
    return isSectionVisible(MixerSectionType::Labels);
}

bool MixerPanelContextMenuModel::soundSectionVisible() const
{
    return isSectionVisible(MixerSectionType::Sound);
}

bool MixerPanelContextMenuModel::faderSectionVisible() const
{
    return isSectionVisible(MixerSectionType::Fader);
}

bool MixerPanelContextMenuModel::muteAndSoloSectionVisible() const
{
    return isSectionVisible(MixerSectionType::MuteAndSolo);
}

bool MixerPanelContextMenuModel::titleSectionVisible() const
{
    return isSectionVisible(MixerSectionType::Title);
}

void MixerPanelContextMenuModel::load()
{
    AbstractMenuModel::load();

    dispatcher()->reg(this, TOGGLE_MIXER_SECTION_ACTION, this, &MixerPanelContextMenuModel::toggleMixerSection);

    configuration()->isMixerSectionVisibleChanged().onReceive(this, [this](MixerSectionType sectionType, bool newVisibilityValue) {
        setViewMenuItemChecked(QString::number(static_cast<int>(sectionType)), newVisibilityValue);

        emitMixerSectionVisibilityChanged(sectionType);
    });

    MenuItemList viewMenuItems {
        buildSectionVisibleItem(MixerSectionType::Labels),
        buildSectionVisibleItem(MixerSectionType::Sound),
    };

    viewMenuItems.push_back(buildSectionVisibleItem(MixerSectionType::Fader));
    viewMenuItems.push_back(buildSectionVisibleItem(MixerSectionType::MuteAndSolo));
    viewMenuItems.push_back(buildSectionVisibleItem(MixerSectionType::Title));

    MenuItemList items {
        makeMenuItem("playback-setup"),
        makeMenu(TranslatableString("playback", "View"), viewMenuItems, VIEW_MENU_ID)
    };

    setItems(items);
}

bool MixerPanelContextMenuModel::isSectionVisible(MixerSectionType sectionType) const
{
    return configuration()->isMixerSectionVisible(sectionType);
}

MenuItem* MixerPanelContextMenuModel::buildSectionVisibleItem(MixerSectionType sectionType)
{
    int sectionTypeInt = static_cast<int>(sectionType);

    MenuItem* item = new MenuItem(this);
    item->setId(QString::number(sectionTypeInt));
    item->setArgs(ActionData::make_arg1<int>(sectionTypeInt));

    UiAction action;
    action.title = mixerSectionTitle(sectionType);
    action.code = TOGGLE_MIXER_SECTION_ACTION;
    action.checkable = Checkable::Yes;
    item->setAction(action);

    UiActionState state;
    state.enabled = true;
    state.checked = isSectionVisible(sectionType);
    item->setState(state);

    return item;
}

void MixerPanelContextMenuModel::toggleMixerSection(const ActionData& args)
{
    if (args.empty()) {
        return;
    }

    int sectionTypeInt = args.arg<int>(0);
    MixerSectionType sectionType = static_cast<MixerSectionType>(sectionTypeInt);

    bool newVisibilityValue = !isSectionVisible(sectionType);
    configuration()->setMixerSectionVisible(sectionType, newVisibilityValue);
}

void MixerPanelContextMenuModel::setViewMenuItemChecked(const QString& itemId, bool checked)
{
    MenuItem& viewMenu = findMenu(VIEW_MENU_ID);

    for (MenuItem* item : viewMenu.subitems()) {
        if (item->id() == itemId) {
            UiActionState state = item->state();
            state.checked = checked;
            item->setState(state);
            return;
        }
    }
}

void MixerPanelContextMenuModel::emitMixerSectionVisibilityChanged(MixerSectionType sectionType)
{
    switch (sectionType) {
    case MixerSectionType::Labels:
        emit labelsSectionVisibleChanged();
        break;
    case MixerSectionType::Sound:
        emit soundSectionVisibleChanged();
        break;
    case MixerSectionType::Fader:
        emit faderSectionVisibleChanged();
        break;
    case MixerSectionType::MuteAndSolo:
        emit muteAndSoloSectionVisibleChanged();
        break;
    case MixerSectionType::Title:
        emit titleSectionVisibleChanged();
        break;
    case MixerSectionType::Unknown:
        break;
    }
}
