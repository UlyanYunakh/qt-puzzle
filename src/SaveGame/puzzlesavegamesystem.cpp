#include "puzzlesavegamesystem.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>

PuzzleSaveGameSystem::PuzzleSaveGameSystem(QObject* parent)
    : QObject(parent)
{
    QDir().mkpath(saveDirectory());
}

QVariantMap PuzzleSaveGameSystem::slotInfo(int slot) const
{
    QVariantMap result{{QStringLiteral("slot"), slot},
                       {QStringLiteral("exists"), false}};
    if (!validSlot(slot))
        return result;

    QFile file(saveFilePath(slot));
    if (!file.open(QIODevice::ReadOnly))
        return result;
    const QVariantMap save = QJsonDocument::fromJson(file.readAll()).toVariant().toMap();
    result[QStringLiteral("exists")] = true;
    result[QStringLiteral("originalImage")] = save.value(QStringLiteral("originalImage"));
    result[QStringLiteral("elapsedSeconds")] = save.value(QStringLiteral("elapsedSeconds"));
    result[QStringLiteral("savedAt")] = save.value(QStringLiteral("savedAt"));
    const QString preview = previewFilePath(slot);
    result[QStringLiteral("statePreview")] = QFile::exists(preview)
        ? QUrl::fromLocalFile(preview).toString() + QStringLiteral("?v=")
              + QString::number(QFileInfo(preview).lastModified().toMSecsSinceEpoch())
        : QString();
    return result;
}

QVariantMap PuzzleSaveGameSystem::loadState(int slot) const
{
    if (!validSlot(slot))
        return {};
    QFile file(saveFilePath(slot));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll()).toVariant().toMap();
}

QString PuzzleSaveGameSystem::previewFilePath(int slot) const
{
    return QDir(saveDirectory()).filePath(QStringLiteral("slot-%1.png").arg(slot));
}

bool PuzzleSaveGameSystem::save(int slot, const QVariantMap& canvasState,
                                const QUrl& originalImage, int elapsedSeconds)
{
    if (!validSlot(slot) || canvasState.isEmpty())
        return false;
    const QVariantMap document{
        {QStringLiteral("version"), 1},
        {QStringLiteral("savedAt"), QDateTime::currentDateTime().toString(Qt::ISODate)},
        {QStringLiteral("elapsedSeconds"), elapsedSeconds},
        {QStringLiteral("originalImage"), originalImage.toString()},
        {QStringLiteral("canvasState"), canvasState}
    };
    QSaveFile file(saveFilePath(slot));
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument::fromVariant(document).toJson(QJsonDocument::Compact));
    if (!file.commit())
        return false;
    emit slotsChanged();
    return true;
}

bool PuzzleSaveGameSystem::validSlot(int slot) const
{
    return slot >= 1 && slot <= 10;
}

QString PuzzleSaveGameSystem::saveDirectory() const
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
        .filePath(QStringLiteral("saves"));
}

QString PuzzleSaveGameSystem::saveFilePath(int slot) const
{
    return QDir(saveDirectory()).filePath(QStringLiteral("slot-%1.json").arg(slot));
}
