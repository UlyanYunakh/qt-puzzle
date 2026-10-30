#ifndef PUZZLEAPPLICATIONSETTINGS_H
#define PUZZLEAPPLICATIONSETTINGS_H

#include <QColor>
#include <QObject>

class PuzzleApplicationSettings final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor
        NOTIFY backgroundColorChanged)
    Q_PROPERTY(int autoSaveMinutes READ autoSaveMinutes WRITE setAutoSaveMinutes
        NOTIFY autoSaveMinutesChanged)

public:
    explicit PuzzleApplicationSettings(const QColor& defaultBackgroundColor,
                                       QObject* parent = nullptr);

    [[nodiscard]] QColor backgroundColor() const;
    void setBackgroundColor(const QColor& color);
    [[nodiscard]] int autoSaveMinutes() const;
    void setAutoSaveMinutes(int minutes);

    Q_INVOKABLE void resetToDefaults();

signals:
    void backgroundColorChanged();
    void autoSaveMinutesChanged();

private:
    QColor m_defaultBackgroundColor;
    QColor m_backgroundColor;
    int m_autoSaveMinutes = 5;
};

#endif // PUZZLEAPPLICATIONSETTINGS_H
