#include "puzzleconfig.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QFileInfo>
#include <QUrl>

#include <stdexcept>

namespace
{
    std::uint32_t unsignedValue(const QJsonObject& object,
                                const QString& key,
                                std::uint32_t fallback)
    {
        const QJsonValue value = object.value(key);
        if (value.isDouble())
            return static_cast<std::uint32_t>(value.toDouble(fallback));
        if (value.isString())
        {
            bool valid = false;
            const std::uint32_t parsed = value.toString().toUInt(&valid, 0);
            if (valid)
                return parsed;
        }
        return fallback;
    }

    QColor colorValue(const QJsonObject& object, const QString& key, const QColor& fallback)
    {
        const QColor color(object.value(key).toString());
        return color.isValid() ? color : fallback;
    }

    bool soundSourceExists(const QString& source)
    {
        if (source.startsWith(QStringLiteral(":/")))
            return QFile::exists(source);

        const QFileInfo fileInfo(source);
        if (fileInfo.isAbsolute())
            return fileInfo.exists();

        const QUrl url(source);
        if (url.scheme() == QStringLiteral("qrc"))
            return QFile::exists(QStringLiteral(":") + url.path());
        if (url.isLocalFile())
            return QFile::exists(url.toLocalFile());
        return url.scheme().isEmpty() && QFile::exists(source);
    }
}

PuzzleConfig PuzzleConfigLoader::load(const QString& resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error(QStringLiteral("Unable to open puzzle config: %1")
                                 .arg(resourcePath).toStdString());

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        throw std::runtime_error(QStringLiteral("Invalid puzzle config %1: %2")
                                 .arg(resourcePath, parseError.errorString()).toStdString());

    PuzzleConfig config;
    const QJsonObject root = document.object();
    const int schemaVersion = root.value(QStringLiteral("schemaVersion")).toInt(1);
    const QJsonObject puzzle = root.value(QStringLiteral("puzzle")).toObject();
    config.columns = puzzle.value(QStringLiteral("columns")).toInt(config.columns);
    config.rows = puzzle.value(QStringLiteral("rows")).toInt(config.rows);
    config.geometrySeed = unsignedValue(puzzle, QStringLiteral("geometrySeed"),
                                        config.geometrySeed);
    config.imageResource = puzzle.value(QStringLiteral("imageResource"))
                                 .toString(config.imageResource);
    config.shapeResource = puzzle.value(QStringLiteral("shapeResource"))
                                 .toString(config.shapeResource);

    const QJsonObject layout = root.value(QStringLiteral("initialLayout")).toObject();
    config.initialLayout.centerX = layout.value(QStringLiteral("centerX"))
                                         .toDouble(config.initialLayout.centerX);
    config.initialLayout.centerY = layout.value(QStringLiteral("centerY"))
                                         .toDouble(config.initialLayout.centerY);
    config.initialLayout.widthRatio = layout.value(QStringLiteral("widthRatio"))
                                            .toDouble(config.initialLayout.widthRatio);
    config.initialLayout.heightRatio = layout.value(QStringLiteral("heightRatio"))
                                             .toDouble(config.initialLayout.heightRatio);
    config.initialLayout.pieceGap = layout.value(QStringLiteral("pieceGap"))
                                          .toDouble(config.initialLayout.pieceGap);
    config.initialLayout.pieceScale = layout.value(QStringLiteral("pieceScale"))
                                            .toDouble(config.initialLayout.pieceScale);
    config.initialLayout.centerClearWidthRatio = layout.value(
                                                           QStringLiteral("centerClearWidthRatio"))
                                                       .toDouble(config.initialLayout.centerClearWidthRatio);
    config.initialLayout.centerClearHeightRatio = layout.value(
                                                            QStringLiteral("centerClearHeightRatio"))
                                                        .toDouble(config.initialLayout.centerClearHeightRatio);
    config.initialLayout.naturalJitter = layout.value(QStringLiteral("naturalJitter"))
                                               .toDouble(config.initialLayout.naturalJitter);
    config.initialLayout.seed = unsignedValue(layout, QStringLiteral("seed"),
                                              config.initialLayout.seed);

    const QJsonObject interaction = root.value(QStringLiteral("interaction")).toObject();
    config.interaction.minimumZoom = interaction.value(QStringLiteral("minimumZoom"))
                                                .toDouble(config.interaction.minimumZoom);
    config.interaction.maximumZoom = interaction.value(QStringLiteral("maximumZoom"))
                                                .toDouble(config.interaction.maximumZoom);
    config.interaction.zoomedPieceSize = interaction.value(
        QStringLiteral("zoomedPieceSize")).toDouble(config.interaction.zoomedPieceSize);
    config.interaction.adaptiveZoomLimit = interaction.value(
        QStringLiteral("adaptiveZoomLimit")).toDouble(config.interaction.adaptiveZoomLimit);
    config.interaction.magnetRadiusRatio = interaction.value(
        QStringLiteral("magnetRadiusRatio")).toDouble(config.interaction.magnetRadiusRatio);
    config.interaction.connectionRadiusRatio = interaction.value(
                                                              QStringLiteral("connectionRadiusRatio"))
                                                          .toDouble(config.interaction.connectionRadiusRatio);
    config.interaction.maximumMagneticPull = interaction.value(
                                                            QStringLiteral("maximumMagneticPull"))
                                                        .toDouble(config.interaction.maximumMagneticPull);
    config.interaction.zoomStep = interaction.value(QStringLiteral("zoomStep"))
                                             .toDouble(config.interaction.zoomStep);

    const QJsonObject appearance = root.value(QStringLiteral("appearance")).toObject();
    config.appearance.background = colorValue(appearance, QStringLiteral("background"),
                                              config.appearance.background);
    config.appearance.pieceShadow = colorValue(appearance, QStringLiteral("pieceShadow"),
                                               config.appearance.pieceShadow);
    config.appearance.selection = colorValue(appearance, QStringLiteral("selection"),
                                             config.appearance.selection);
    config.appearance.marqueeFill = colorValue(appearance, QStringLiteral("marqueeFill"),
                                               config.appearance.marqueeFill);
    config.appearance.marqueeBorder = colorValue(appearance, QStringLiteral("marqueeBorder"),
                                                 config.appearance.marqueeBorder);
    config.appearance.pieceShadowOffsetX = appearance.value(
                                                         QStringLiteral("pieceShadowOffsetX"))
                                                     .toDouble(config.appearance.pieceShadowOffsetX);
    config.appearance.pieceShadowOffsetY = appearance.value(
                                                         QStringLiteral("pieceShadowOffsetY"))
                                                     .toDouble(config.appearance.pieceShadowOffsetY);
    config.appearance.selectionBorderWidth = appearance.value(
                                                           QStringLiteral("selectionBorderWidth"))
                                                       .toDouble(config.appearance.selectionBorderWidth);
    config.appearance.marqueeBorderWidth = appearance.value(
                                                         QStringLiteral("marqueeBorderWidth"))
                                                     .toDouble(config.appearance.marqueeBorderWidth);

    const QJsonObject window = root.value(QStringLiteral("window")).toObject();
    config.window.title = window.value(QStringLiteral("title")).toString(config.window.title);
    config.window.width = window.value(QStringLiteral("width")).toInt(config.window.width);
    config.window.height = window.value(QStringLiteral("height")).toInt(config.window.height);

    const QJsonObject audio = root.value(QStringLiteral("audio")).toObject();
    config.audio.pieceDropResource = audio.value(QStringLiteral("pieceDropResource"))
                                          .toString(config.audio.pieceDropResource);
    config.audio.piecePlacedResource = audio.value(QStringLiteral("piecePlacedResource"))
                                            .toString(config.audio.piecePlacedResource);
    config.audio.buttonClickResource = audio.value(QStringLiteral("buttonClickResource"))
                                            .toString(config.audio.buttonClickResource);

    for (const QString& soundSource : {
             config.audio.pieceDropResource,
             config.audio.piecePlacedResource,
             config.audio.buttonClickResource
         })
    {
        if (!soundSourceExists(soundSource))
            throw std::runtime_error(QStringLiteral("Configured sound does not exist: %1")
                                     .arg(soundSource).toStdString());
    }

    if (schemaVersion != 1 || config.columns < 1 || config.rows < 1
        || config.imageResource.isEmpty()
        || config.shapeResource.isEmpty() || config.interaction.minimumZoom <= 0.0
        || config.interaction.maximumZoom < config.interaction.minimumZoom
        || config.interaction.zoomedPieceSize <= 0.0
        || config.interaction.adaptiveZoomLimit < config.interaction.maximumZoom
        || config.interaction.magnetRadiusRatio < 0.0
        || config.interaction.connectionRadiusRatio < 0.0
        || config.interaction.connectionRadiusRatio > 1.0
        || config.interaction.maximumMagneticPull < 0.0
        || config.interaction.maximumMagneticPull > 1.0
        || config.interaction.zoomStep <= 1.0
        || config.initialLayout.pieceScale <= 0.0
        || config.initialLayout.pieceScale > 1.0
        || config.initialLayout.centerClearWidthRatio < 0.0
        || config.initialLayout.centerClearWidthRatio >= 1.0
        || config.initialLayout.centerClearHeightRatio < 0.0
        || config.initialLayout.centerClearHeightRatio >= 1.0
        || config.initialLayout.naturalJitter < 0.0
        || config.initialLayout.naturalJitter > 1.0
        || config.appearance.pieceShadow.alpha() != 255
        || config.appearance.pieceShadowOffsetX < 0.0
        || config.appearance.pieceShadowOffsetY < 0.0
        || config.appearance.selectionBorderWidth <= 0.0
        || config.appearance.marqueeBorderWidth <= 0.0
        || config.window.title.isEmpty() || config.window.width < 1 || config.window.height < 1)
        throw std::runtime_error("Puzzle configuration contains invalid values");

    return config;
}
