#ifndef PUZZLECONFIG_H
#define PUZZLECONFIG_H

#include <QColor>
#include <QString>

#include <cstdint>

struct PuzzleInitialLayoutConfig
{
    double centerX = 0.5;
    double centerY = 0.5;
    double widthRatio = 0.90;
    double heightRatio = 0.86;
    double pieceGap = 12.0;
    double pieceScale = 0.72;
    double centerClearWidthRatio = 0.46;
    double centerClearHeightRatio = 0.48;
    double naturalJitter = 0.75;
    std::uint32_t seed = 0x50555A5Au;
};

struct PuzzleInteractionConfig
{
    double minimumZoom = 0.15;
    double maximumZoom = 6.0;
    double zoomedPieceSize = 180.0;
    double adaptiveZoomLimit = 80.0;
    double minimumMagnetRadiusRatio = 0.0;
    double magnetRadiusRatio = 0.25;
    double maximumMagnetRadiusRatio = 0.5;
    double connectionRadiusRatio = 0.28;
    double maximumMagneticPull = 0.65;
    double zoomStep = 1.15;
};

struct PuzzleAppearanceConfig
{
    QColor background = QColor(QStringLiteral("#20242b"));
    QColor pieceShadow = QColor(QStringLiteral("#101318"));
    QColor selection = QColor(255, 193, 7, 255);
    QColor marqueeFill = QColor(70, 150, 255, 120);
    QColor marqueeBorder = QColor(100, 180, 255, 255);
    double pieceShadowOffsetX = 3.0;
    double pieceShadowOffsetY = 5.0;
    double selectionBorderWidth = 3.0;
    double marqueeBorderWidth = 1.5;
};

struct PuzzleWindowConfig
{
    QString title = QStringLiteral("Puzzle Canvas");
    int width = 1280;
    int height = 800;
};

struct PuzzleAudioConfig
{
    QString pieceDropResource = QStringLiteral(":/sfx/piece-drop.wav");
    QString piecePlacedResource = QStringLiteral(":/sfx/piece-placed.wav");
    QString buttonClickResource = QStringLiteral(":/sfx/button-click-v2.wav");
};

struct PuzzleConfig
{
    int columns = 12;
    int rows = 8;
    std::uint32_t geometrySeed = 0x50555A5Au;
    QString imageResource = QStringLiteral(":/assets/test-puzzle.jpg");
    QString shapeResource = QStringLiteral(":/puzzle-shapes/classic.json");
    PuzzleInitialLayoutConfig initialLayout;
    PuzzleInteractionConfig interaction;
    PuzzleAppearanceConfig appearance;
    PuzzleWindowConfig window;
    PuzzleAudioConfig audio;
};

class PuzzleConfigLoader
{
public:
    [[nodiscard]] static PuzzleConfig load(const QString& resourcePath);
};

#endif // PUZZLECONFIG_H
