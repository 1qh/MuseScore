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

#include <memory>

#include "../../iappmenumodelhook.h"
#include "async/asyncable.h"
#include "context/iglobalcontext.h"
#include "modularity/ioc.h"

namespace mu::appshell {
class MacOSAppMenuModelHook : public IAppMenuModelHook, public muse::Contextable, public muse::async::Asyncable
{
    muse::ContextInject<context::IGlobalContext> globalContext = { this };

public:
    explicit MacOSAppMenuModelHook(const muse::modularity::ContextPtr& iocCtx);
    ~MacOSAppMenuModelHook() override;

    void onAppMenuInited() override;

private:
    void onCurrentProjectChanged();
    void scheduleNativeMenuUpdate();
    void updateNativeMenu();
    void updateApplicationMenuTitle();

    struct NativeMenuObserver;
    muse::async::Asyncable m_projectSubscriptions;
    std::unique_ptr<NativeMenuObserver> m_nativeMenuObserver;
    bool m_inited = false;
};
}
