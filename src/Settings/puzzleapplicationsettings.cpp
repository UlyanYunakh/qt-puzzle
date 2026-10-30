#include "puzzleapplicationsettings.h"

#include <QSettings>

namespace
{
    constexpr auto backgroundColorKey = "appearance/backgroundColor";
}

PuzzleApplicationSettings::PuzzleApplicationSettings(const QColor& defaultBackgroundColor,
                                                     QObject* parent)
    : QObject(parent)
      , m_defaultBackgroundColor(defaultBackgroundColor)
{
    const QColor savedColor(QSettings().value(
        QString::fromLatin1(backgroundColorKey), defaultBackgroundColor).toString());
    m_backgroundColor = savedColor.isValid() ? savedColor : defaultBackgroundColor;
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
}
