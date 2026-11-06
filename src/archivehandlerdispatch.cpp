#include "archivehandlerdispatch.h"

#include <QSet>

#include <uibase/game_features/dataarchives.h>
#include <uibase/game_features/igamefeatures.h>
#include <uibase/imoinfo.h>

namespace
{

QStringList builtInArchiveNameFilters()
{
  return {"*.bsa", "*.ba2"};
}

}  // namespace

bool canUseBuiltInArchiveTools(const MOBase::IOrganizer* organizer)
{
  if (organizer == nullptr || organizer->gameFeatures() == nullptr) {
    return false;
  }

  return organizer->gameFeatures()->gameFeature<MOBase::DataArchives>() != nullptr;
}

std::shared_ptr<const MOBase::GameArchiveHandler>
findGameArchiveHandler(const MOBase::IOrganizer* organizer)
{
  if (organizer == nullptr || organizer->gameFeatures() == nullptr) {
    return {};
  }

  return organizer->gameFeatures()->gameFeature<MOBase::GameArchiveHandler>();
}

// Combine the existing Bethesda archive list with a game handler's supported
// formats. Every delegated candidate must be accepted by supportsArchive().
QFileInfoList findExtractableArchives(
    const QDir& directory, bool includeBuiltInArchives,
    const std::shared_ptr<const MOBase::GameArchiveHandler>& archiveHandler)
{
  QFileInfoList archives;

  if (includeBuiltInArchives) {
    archives = directory.entryInfoList(
        builtInArchiveNameFilters(), QDir::Files | QDir::NoDotAndDotDot,
        QDir::Name | QDir::IgnoreCase);
  }

  if (!archiveHandler) {
    return archives;
  }

  // Built-in and delegated name filters can match the same archive. Normalize
  // case and path spelling to show each file only once.
  QSet<QString> seenPaths;
  for (const auto& archiveInfo : archives) {
    seenPaths.insert(QDir::cleanPath(archiveInfo.absoluteFilePath()).toLower());
  }

  // An empty handler filter means it must inspect every file; the handler's
  // supportsArchive() decision remains authoritative for each candidate.
  QStringList delegatedFilters = archiveHandler->supportedArchiveNameFilters();
  QFileInfoList allFiles =
      delegatedFilters.isEmpty()
          ? directory.entryInfoList(QDir::Files | QDir::NoDotAndDotDot,
                                    QDir::Name | QDir::IgnoreCase)
          : directory.entryInfoList(delegatedFilters, QDir::Files | QDir::NoDotAndDotDot,
                                    QDir::Name | QDir::IgnoreCase);

  for (const auto& fileInfo : allFiles) {
    const QString cleanPath = QDir::cleanPath(fileInfo.absoluteFilePath()).toLower();
    if (seenPaths.contains(cleanPath)) {
      continue;
    }

    if (archiveHandler->supportsArchive(fileInfo.absoluteFilePath())) {
      archives.push_back(fileInfo);
      seenPaths.insert(cleanPath);
    }
  }

  return archives;
}

bool shouldDelegateArchive(
    const QString& archivePath,
    const std::shared_ptr<const MOBase::GameArchiveHandler>& archiveHandler)
{
  return archiveHandler && archiveHandler->supportsArchive(archivePath);
}
