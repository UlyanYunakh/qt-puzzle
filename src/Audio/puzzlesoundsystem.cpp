#include "puzzlesoundsystem.h"

#include "Config/puzzleconfig.h"

#include <QSettings>
#include <QSoundEffect>
#include <QUrl>
#include <QFileInfo>

#include <algorithm>
#include <cmath>

namespace
{
    constexpr auto gameplayEffectsVolumeKey = "audio/gameplayEffectsVolume";
    constexpr auto systemSoundsVolumeKey = "audio/systemSoundsVolume";
    constexpr qreal defaultVolume = 0.75;

    QUrl soundUrl(const QString& source)
    {
        if (source.startsWith(QStringLiteral(":/")))
            return QUrl(QStringLiteral("qrc") + source);

        const QUrl url(source);
        return url.isRelative()
                   ? QUrl::fromLocalFile(QFileInfo(source).absoluteFilePath())
                   : url;
    }
}

PuzzleSoundSystem::PuzzleSoundSystem(const PuzzleAudioConfig& config, QObject* parent)
    : QObject(parent)
      , m_pieceDropEffect(new QSoundEffect(this))
      , m_piecePlacedEffect(new QSoundEffect(this))
      , m_buttonClickEffect(new QSoundEffect(this))
{
    m_pieceDropEffect->setSource(soundUrl(config.pieceDropResource));
    m_piecePlacedEffect->setSource(soundUrl(config.piecePlacedResource));
    m_buttonClickEffect->setSource(soundUrl(config.buttonClickResource));

    const QSettings settings;
    const qreal gameplayVolume = boundedVolume(
        settings.value(QString::fromLatin1(gameplayEffectsVolumeKey),
                       settings.value(QStringLiteral("audio/pieceDropVolume"), defaultVolume))
                .toReal());
    m_pieceDropEffect->setVolume(gameplayVolume);
    m_piecePlacedEffect->setVolume(gameplayVolume);
    m_buttonClickEffect->setVolume(boundedVolume(
        settings.value(QString::fromLatin1(systemSoundsVolumeKey),
                       settings.value(QStringLiteral("audio/buttonClickVolume"), defaultVolume))
                .toReal()));
}

qreal PuzzleSoundSystem::gameplayEffectsVolume() const
{
    return m_pieceDropEffect->volume();
}

qreal PuzzleSoundSystem::systemSoundsVolume() const
{
    return m_buttonClickEffect->volume();
}

void PuzzleSoundSystem::setGameplayEffectsVolume(qreal volume)
{
    volume = boundedVolume(volume);
    if (qFuzzyCompare(1.0 + gameplayEffectsVolume(), 1.0 + volume))
        return;

    m_pieceDropEffect->setVolume(volume);
    m_piecePlacedEffect->setVolume(volume);
    QSettings().setValue(QString::fromLatin1(gameplayEffectsVolumeKey), volume);
    emit gameplayEffectsVolumeChanged();
}

void PuzzleSoundSystem::setSystemSoundsVolume(qreal volume)
{
    volume = boundedVolume(volume);
    if (qFuzzyCompare(1.0 + systemSoundsVolume(), 1.0 + volume))
        return;

    m_buttonClickEffect->setVolume(volume);
    QSettings().setValue(QString::fromLatin1(systemSoundsVolumeKey), volume);
    emit systemSoundsVolumeChanged();
}

void PuzzleSoundSystem::playPieceRelease(bool connectedToCorrectNeighbour)
{
    restart(connectedToCorrectNeighbour ? *m_piecePlacedEffect : *m_pieceDropEffect);
}

void PuzzleSoundSystem::playButtonClick()
{
    restart(*m_buttonClickEffect);
}

void PuzzleSoundSystem::resetToDefaults()
{
    setGameplayEffectsVolume(defaultVolume);
    setSystemSoundsVolume(defaultVolume);
}

qreal PuzzleSoundSystem::boundedVolume(qreal volume)
{
    return std::clamp(volume, qreal(0.0), qreal(1.0));
}

void PuzzleSoundSystem::restart(QSoundEffect& effect)
{
    effect.stop();
    effect.play();
}
