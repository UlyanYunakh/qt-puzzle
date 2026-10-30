#ifndef PUZZLESOUNDSYSTEM_H
#define PUZZLESOUNDSYSTEM_H

#include <QObject>

class QSoundEffect;
struct PuzzleAudioConfig;

class PuzzleSoundSystem final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal gameplayEffectsVolume READ gameplayEffectsVolume
        WRITE setGameplayEffectsVolume NOTIFY gameplayEffectsVolumeChanged)
    Q_PROPERTY(qreal systemSoundsVolume READ systemSoundsVolume
        WRITE setSystemSoundsVolume NOTIFY systemSoundsVolumeChanged)

public:
    explicit PuzzleSoundSystem(const PuzzleAudioConfig& config, QObject* parent = nullptr);

    [[nodiscard]] qreal gameplayEffectsVolume() const;
    [[nodiscard]] qreal systemSoundsVolume() const;

    void setGameplayEffectsVolume(qreal volume);
    void setSystemSoundsVolume(qreal volume);

    /// Plays the sound appropriate for a completed piece drag.
    Q_INVOKABLE void playPieceRelease(bool connectedToCorrectNeighbour);

    /// Plays the shared feedback sound used by all application buttons.
    Q_INVOKABLE void playButtonClick();

    /// Restores both sound categories to their application defaults.
    Q_INVOKABLE void resetToDefaults();

signals:
    void gameplayEffectsVolumeChanged();
    void systemSoundsVolumeChanged();

private:
    static qreal boundedVolume(qreal volume);
    static void restart(QSoundEffect& effect);

    QSoundEffect* m_pieceDropEffect = nullptr;
    QSoundEffect* m_piecePlacedEffect = nullptr;
    QSoundEffect* m_buttonClickEffect = nullptr;
};

#endif // PUZZLESOUNDSYSTEM_H
