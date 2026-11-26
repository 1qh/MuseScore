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

#include "macosappmenumodelhook.h"

#include <Cocoa/Cocoa.h>
#include <objc/runtime.h>

#include <QTimer>
#include <vector>

static NSNotificationName const MuseMainMenuDidChangeNotification = @"MuseMainMenuDidChangeNotification";

@interface NSApplication (MuseAppMenuTitle)
- (void)muse_appMenuTitle_setMainMenu:(NSMenu*)menu;
@end

@implementation NSApplication (MuseAppMenuTitle)
- (void)muse_appMenuTitle_setMainMenu:(NSMenu*)menu
{
    [self muse_appMenuTitle_setMainMenu:menu];
    [[NSNotificationCenter defaultCenter] postNotificationName:MuseMainMenuDidChangeNotification object:self];
}
@end

using namespace mu::appshell;

struct MacOSAppMenuModelHook::NativeMenuObserver {
    ~NativeMenuObserver()
    {
        for (id observer : observers) {
            [[NSNotificationCenter defaultCenter] removeObserver:observer];
        }
    }

    std::vector<id> observers;
    QTimer timer;
    bool updatePending = false;
};

MacOSAppMenuModelHook::MacOSAppMenuModelHook(const muse::modularity::ContextPtr& iocCtx)
    : muse::Contextable(iocCtx), m_nativeMenuObserver(std::make_unique<NativeMenuObserver>())
{
}

MacOSAppMenuModelHook::~MacOSAppMenuModelHook() = default;

void MacOSAppMenuModelHook::updateApplicationMenuTitle()
{
    NSMenu* mainMenu = [NSApp mainMenu];
    if (!mainMenu || [mainMenu numberOfItems] == 0) {
        return;
    }

    NSString* title = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleDisplayName"];
    if (![title length]) {
        title = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleName"];
    }
    if (![title length]) {
        title = [[NSProcessInfo processInfo] processName];
    }
    if (auto project = globalContext()->currentProject()) {
        QString name = project->isNewlyCreated() || project->path().empty()
                       ? project->displayName() : muse::io::filename(project->path(), false).toQString();
        title = name.toNSString();
    }

    NSMenuItem* appItem = [mainMenu itemAtIndex:0];
    //! NOTE A new main menu or a replaced application item is drawn by AppKit with the app's own name even where the
    //! item's title already equals the target, so a title is set again through an empty one whenever either changes
    static const NSMenu* titledMenu = nil; // compared by identity only, never dereferenced
    static const NSMenuItem* titledItem = nil;
    if (titledMenu != mainMenu || titledItem != appItem) {
        [appItem setTitle:@""];
        titledMenu = mainMenu;
        titledItem = appItem;
    }
    if (![[appItem title] isEqualToString:title]) {
        [appItem setTitle:title];
    }
    NSMenu* appMenu = [appItem submenu];
    if (appMenu && ![[appMenu title] isEqualToString:title]) {
        [appMenu setTitle:title];
    }
}

void MacOSAppMenuModelHook::onCurrentProjectChanged()
{
    m_projectSubscriptions.async_disconnectAll();
    if (auto project = globalContext()->currentProject()) {
        project->pathChanged().onNotify(&m_projectSubscriptions, [this]() {
            updateNativeMenu();
        });
        project->displayNameChanged().onNotify(&m_projectSubscriptions, [this]() {
            updateNativeMenu();
        });
    }
    updateNativeMenu();
}

static void ensureWindowsMenu()
{
    NSMenu* mainMenu = [NSApp mainMenu];
    if (!mainMenu || [mainMenu numberOfItems] == 0
        || ([NSApp windowsMenu] && [mainMenu indexOfItemWithSubmenu:[NSApp windowsMenu]] >= 0)) {
        return;
    }

    NSString* title = NSLocalizedString(@"Window", nil);
    NSMenuItem* windowItem = [[NSMenuItem alloc] initWithTitle:title action:nil keyEquivalent:@""];
    NSMenu* windowMenu = [[NSMenu alloc] initWithTitle:title];
    [windowItem setSubmenu:windowMenu];

    NSInteger helpIndex = [NSApp helpMenu] ? [mainMenu indexOfItemWithSubmenu:[NSApp helpMenu]] : -1;
    if (helpIndex < 0) {
        helpIndex = [mainMenu numberOfItems];
    }
    [mainMenu insertItem:windowItem atIndex:helpIndex];
    [NSApp setWindowsMenu:windowMenu];
}

void MacOSAppMenuModelHook::scheduleNativeMenuUpdate()
{
    if (m_nativeMenuObserver->updatePending) {
        return;
    }
    m_nativeMenuObserver->updatePending = true;
    QTimer::singleShot(0, &m_nativeMenuObserver->timer, [this]() {
        m_nativeMenuObserver->updatePending = false;
        updateNativeMenu();
    });
}

void MacOSAppMenuModelHook::updateNativeMenu()
{
    ensureWindowsMenu();
    updateApplicationMenuTitle();
}

void MacOSAppMenuModelHook::onAppMenuInited()
{
    if (m_inited) {
        return;
    }
    m_inited = true;

    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"NSDisabledDictationMenuItem"];
    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"NSDisabledCharacterPaletteMenuItem"];

    static dispatch_once_t once;
    dispatch_once(&once, ^{
        Method original = class_getInstanceMethod([NSApplication class], @selector(setMainMenu:));
        Method replacement = class_getInstanceMethod([NSApplication class], @selector(muse_appMenuTitle_setMainMenu:));
        method_exchangeImplementations(original, replacement);
    });

    for (NSNotificationName name : { NSMenuDidChangeItemNotification, NSMenuDidAddItemNotification,
                                    NSMenuDidRemoveItemNotification, MuseMainMenuDidChangeNotification }) {
        id observer = [[NSNotificationCenter defaultCenter] addObserverForName:name object:nil
                                                                       queue:[NSOperationQueue mainQueue]
                                                                  usingBlock:^(NSNotification*) {
            scheduleNativeMenuUpdate();
        }];
        m_nativeMenuObserver->observers.push_back(observer);
    }

    //! NOTE AppKit redraws the application menu with the app's own name when the app becomes active, without changing
    //! the menu item's title, so a title that is already equal is set again through an empty one to make AppKit redraw it
    id activeObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSApplicationDidBecomeActiveNotification
                                                                          object:nil
                                                                           queue:[NSOperationQueue mainQueue]
                                                                      usingBlock:^(NSNotification*) {
        NSMenuItem* appItem = [[NSApp mainMenu] numberOfItems] > 0 ? [[NSApp mainMenu] itemAtIndex:0] : nil;
        if (appItem) {
            NSString* title = [[appItem title] copy];
            [appItem setTitle:@""];
            [appItem setTitle:title];
        }
        updateNativeMenu();
    }];
    m_nativeMenuObserver->observers.push_back(activeObserver);


    globalContext()->currentProjectChanged().onNotify(this, [this]() {
        onCurrentProjectChanged();
    });
    onCurrentProjectChanged();

    scheduleNativeMenuUpdate();
}
