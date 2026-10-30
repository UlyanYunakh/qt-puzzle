#ifndef PUZZLESAVEGAMESYSTEM_H
#define PUZZLESAVEGAMESYSTEM_H

#include <QObject>
#include <QUrl>
#include <QVariantMap>

class PuzzleSaveGameSystem final : public QObject
{
    Q_OBJECT

public:
    explicit PuzzleSaveGameSystem(QObject* parent = nullptr);

    Q_INVOKABLE QVariantMap slotInfo(int slot) const;
    Q_INVOKABLE QVariantMap loadState(int slot) const;
    Q_INVOKABLE QString previewFilePath(int slot) const;
    Q_INVOKABLE bool save(int slot, const QVariantMap& canvasState,
                          const QUrl& originalImage, int elapsedSeconds);

signals:
    void slotsChanged();

private:
    [[nodiscard]] bool validSlot(int slot) const;
    [[nodiscard]] QString saveDirectory() const;
    [[nodiscard]] QString saveFilePath(int slot) const;
};

#endif // PUZZLESAVEGAMESYSTEM_H
