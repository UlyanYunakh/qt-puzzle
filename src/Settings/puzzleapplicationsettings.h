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
    Q_PROPERTY(qreal magnetism READ magnetism WRITE setMagnetism NOTIFY magnetismChanged)
    Q_PROPERTY(qreal minimumMagnetism READ minimumMagnetism CONSTANT)
    Q_PROPERTY(qreal maximumMagnetism READ maximumMagnetism CONSTANT)

public:
    explicit PuzzleApplicationSettings(const QColor& defaultBackgroundColor,
                                       qreal defaultMagnetism,
                                       qreal minimumMagnetism,
                                       qreal maximumMagnetism,
                                       QObject* parent = nullptr);

    [[nodiscard]] QColor backgroundColor() const;
    void setBackgroundColor(const QColor& color);
    [[nodiscard]] int autoSaveMinutes() const;
    void setAutoSaveMinutes(int minutes);
    [[nodiscard]] qreal magnetism() const;
    void setMagnetism(qreal magnetism);
    [[nodiscard]] qreal minimumMagnetism() const;
    [[nodiscard]] qreal maximumMagnetism() const;

    Q_INVOKABLE void resetToDefaults();

signals:
    void backgroundColorChanged();
    void autoSaveMinutesChanged();
    void magnetismChanged();

private:
    QColor m_defaultBackgroundColor;
    QColor m_backgroundColor;
    qreal m_defaultMagnetism = 0.25;
    qreal m_minimumMagnetism = 0.0;
    qreal m_maximumMagnetism = 0.5;
    qreal m_magnetism = 0.25;
    int m_autoSaveMinutes = 5;
};

#endif // PUZZLEAPPLICATIONSETTINGS_H
