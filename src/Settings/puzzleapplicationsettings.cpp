#include "puzzleapplicationsettings.h"

#include <QSettings>

#include <algorithm>

namespace
{
    constexpr auto backgroundColorKey = "appearance/backgroundColor";
    constexpr auto autoSaveMinutesKey = "save/autoSaveMinutes";
    constexpr auto magnetismKey = "interaction/magnetism";
}

PuzzleApplicationSettings::PuzzleApplicationSettings(const QColor& defaultBackgroundColor,
                                                     qreal defaultMagnetism,
                                                     qreal minimumMagnetism,
                                                     qreal maximumMagnetism,
                                                     QObject* parent)
    : QObject(parent)
      , m_defaultBackgroundColor(defaultBackgroundColor)
      , m_defaultMagnetism(defaultMagnetism)
      , m_minimumMagnetism(minimumMagnetism)
      , m_maximumMagnetism(maximumMagnetism)
{
    const QColor savedColor(QSettings().value(
        QString::fromLatin1(backgroundColorKey), defaultBackgroundColor).toString());
    m_backgroundColor = savedColor.isValid() ? savedColor : defaultBackgroundColor;
    setAutoSaveMinutes(QSettings().value(QString::fromLatin1(autoSaveMinutesKey), 5).toInt());
    m_defaultMagnetism = std::clamp(m_defaultMagnetism,
                                    m_minimumMagnetism, m_maximumMagnetism);
    m_magnetism = std::clamp(
        QSettings().value(QString::fromLatin1(magnetismKey), m_defaultMagnetism).toDouble(),
        m_minimumMagnetism, m_maximumMagnetism);
}

int PuzzleApplicationSettings::autoSaveMinutes() const
{
    return m_autoSaveMinutes;
}

void PuzzleApplicationSettings::setAutoSaveMinutes(int minutes)
{
    if (minutes != 1 && minutes != 5 && minutes != 10)
        minutes = 5;
    if (m_autoSaveMinutes == minutes)
        return;
    m_autoSaveMinutes = minutes;
    QSettings().setValue(QString::fromLatin1(autoSaveMinutesKey), minutes);
    emit autoSaveMinutesChanged();
}

qreal PuzzleApplicationSettings::magnetism() const
{
    return m_magnetism;
}

void PuzzleApplicationSettings::setMagnetism(qreal magnetism)
{
    magnetism = std::clamp(magnetism, m_minimumMagnetism, m_maximumMagnetism);
    if (qFuzzyCompare(m_magnetism, magnetism))
        return;
    m_magnetism = magnetism;
    QSettings().setValue(QString::fromLatin1(magnetismKey), magnetism);
    emit magnetismChanged();
}

qreal PuzzleApplicationSettings::minimumMagnetism() const
{
    return m_minimumMagnetism;
}

qreal PuzzleApplicationSettings::maximumMagnetism() const
{
    return m_maximumMagnetism;
}

QColor PuzzleApplicationSettings::backgroundColor() const
{
    return m_backgroundColor;
}

void PuzzleApplicationSettings::setBackgroundColor(const QColor& color)
{
    if (!color.isValid() || color == m_backgroundColor)
        return;

    m_backgroundColor = color;
    QSettings().setValue(QString::fromLatin1(backgroundColorKey), color.name(QColor::HexArgb));
    emit backgroundColorChanged();
}

void PuzzleApplicationSettings::resetToDefaults()
{
    setBackgroundColor(m_defaultBackgroundColor);
    setAutoSaveMinutes(5);
    setMagnetism(m_defaultMagnetism);
}
