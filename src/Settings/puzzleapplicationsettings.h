#ifndef PUZZLEAPPLICATIONSETTINGS_H
#define PUZZLEAPPLICATIONSETTINGS_H

#include <QColor>
#include <QObject>

class PuzzleApplicationSettings final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor
        NOTIFY backgroundColorChanged)

public:
    explicit PuzzleApplicationSettings(const QColor& defaultBackgroundColor,
                                       QObject* parent = nullptr);

    [[nodiscard]] QColor backgroundColor() const;
    void setBackgroundColor(const QColor& color);

    Q_INVOKABLE void resetToDefaults();

signals:
    void backgroundColorChanged();

private:
    QColor m_defaultBackgroundColor;
    QColor m_backgroundColor;
};

#endif // PUZZLEAPPLICATIONSETTINGS_H
