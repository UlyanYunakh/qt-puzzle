#include "puzzleapplicationsettings.h"

#include <QSettings>

namespace
{
    constexpr auto backgroundColorKey = "appearance/backgroundColor";
    constexpr auto autoSaveMinutesKey = "save/autoSaveMinutes";
}

PuzzleApplicationSettings::PuzzleApplicationSettings(const QColor& defaultBackgroundColor,
                                                     QObject* parent)
    : QObject(parent)
      , m_defaultBackgroundColor(defaultBackgroundColor)
{
    const QColor savedColor(QSettings().value(
        QString::fromLatin1(backgroundColorKey), defaultBackgroundColor).toString());
    m_backgroundColor = savedColor.isValid() ? savedColor : defaultBackgroundColor;
    setAutoSaveMinutes(QSettings().value(QString::fromLatin1(autoSaveMinutesKey), 5).toInt());
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
}
