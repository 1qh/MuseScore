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
#ifndef MU_PROJECT_PROJECTCONFIGURATION_H
#define MU_PROJECT_PROJECTCONFIGURATION_H

#include "types/val.h"

#include "modularity/ioc.h"
#include "global/iglobalconfiguration.h"
#include "io/ifilesystem.h"
#include "accessibility/iaccessibilityconfiguration.h"
#include "notation/inotationconfiguration.h"
#include "languages/ilanguagesservice.h"

#include "../iprojectconfiguration.h"

namespace mu::project {
class ProjectConfiguration : public IProjectConfiguration, public muse::Contextable
{
    muse::GlobalInject<muse::IGlobalConfiguration> globalConfiguration;
    muse::GlobalInject<notation::INotationConfiguration> notationConfiguration;
    muse::GlobalInject<muse::io::IFileSystem> fileSystem;
    muse::GlobalInject<muse::accessibility::IAccessibilityConfiguration> accessibilityConfiguration;
    muse::GlobalInject<muse::languages::ILanguagesService> languagesService;

public:
    ProjectConfiguration(const muse::modularity::ContextPtr& iocCtx);

    void init();

    muse::io::path_t recentFilesJsonPath() const override;
    muse::ByteArray compatRecentFilesData() const override;

    muse::io::path_t myFirstProjectPath() const override;

    muse::io::paths_t availableTemplateDirs() const override;
    muse::io::path_t templateCategoriesJsonPath(const muse::io::path_t& templatesDir) const override;

    muse::io::path_t userTemplatesPath() const override;
    void setUserTemplatesPath(const muse::io::path_t& path) override;
    muse::async::Channel<muse::io::path_t> userTemplatesPathChanged() const override;

    muse::io::path_t lastOpenedProjectsPath() const override;
    void setLastOpenedProjectsPath(const muse::io::path_t& path) override;

    muse::io::path_t lastSavedProjectsPath() const override;
    void setLastSavedProjectsPath(const muse::io::path_t& path) override;

    muse::io::path_t userProjectsPath() const override;
    void setUserProjectsPath(const muse::io::path_t& path) override;
    muse::async::Channel<muse::io::path_t> userProjectsPathChanged() const override;
    muse::io::path_t defaultUserProjectsPath() const override;
    muse::io::path_t defaultOpenProjectsPath() const override;





    muse::io::path_t defaultSavingFilePath(INotationProjectPtr project, const std::string& filenameAddition = "",
                                           const std::string& suffix = "") const override;




    int homeScoresPageTabIndex() const override;
    void setHomeScoresPageTabIndex(int index) override;

    HomeScoresPageViewType homeScoresPageViewType() const override;
    void setHomeScoresPageViewType(HomeScoresPageViewType type) override;

    QColor templatePreviewBackgroundColor() const override;
    muse::async::Notification templatePreviewBackgroundChanged() const override;

    PreferredScoreCreationMode preferredScoreCreationMode() const override;
    void setPreferredScoreCreationMode(PreferredScoreCreationMode mode) override;

    MigrationOptions migrationOptions(MigrationType type) const override;
    void setMigrationOptions(MigrationType type, const MigrationOptions& opt, bool persistent = true) override;

    bool isAutoSaveEnabled() const override;
    void setAutoSaveEnabled(bool enabled) override;
    muse::async::Channel<bool> autoSaveEnabledChanged() const override;

    int autoSaveIntervalMinutes() const override;
    void setAutoSaveInterval(int minutes) override;
    muse::async::Channel<int> autoSaveIntervalChanged() const override;




    muse::io::path_t newProjectTemporaryPath() const override;

    bool isAccessibleEnabled() const override;

    bool shouldDestinationFolderBeOpenedOnExport() const override;
    void setShouldDestinationFolderBeOpenedOnExport(bool shouldDestinationFolderBeOpenedOnExport) override;






    muse::io::path_t projectBackupPath(const muse::io::path_t& projectPath) const override;


    bool disableVersionChecking() const override;
    void setDisableVersionChecking(bool disable) override;

    bool createBackupBeforeSaving() const override;
    void setCreateBackupBeforeSaving(bool create) override;



private:
    muse::io::path_t appTemplatesPath() const;

    std::string uniqueFileNameAddition(const muse::io::path_t& filename, const muse::io::path_t& folderPath,
                                       const std::string& suffix = std::string()) const;

    muse::async::Channel<muse::io::path_t> m_userTemplatesPathChanged;
    muse::async::Channel<muse::io::path_t> m_userScoresPathChanged;

    muse::async::Channel<bool> m_autoSaveEnabledChanged;
    muse::async::Channel<int> m_autoSaveIntervalChanged;



    mutable std::map<MigrationType, MigrationOptions> m_migrationOptions;
};
}

#endif // MU_PROJECT_PROJECTCONFIGURATION_H
