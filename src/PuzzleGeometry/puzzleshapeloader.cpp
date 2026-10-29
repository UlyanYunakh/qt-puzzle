#include "puzzleshapeloader.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLoggingCategory>

#include <stdexcept>

namespace
{
    QPointF pointFromArray(const QJsonArray& values, int offset)
    {
        return {values.at(offset).toDouble(), values.at(offset + 1).toDouble()};
    }
}

PuzzleShapeSet PuzzleShapeLoader::load(const QString& resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error(QStringLiteral("Unable to open puzzle shape set: %1")
                                     .arg(resourcePath).toStdString());

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        throw std::runtime_error(QStringLiteral("Invalid puzzle shape set %1: %2")
                                     .arg(resourcePath, parseError.errorString()).toStdString());

    const QJsonObject root = document.object();
    PuzzleShapeSet shapeSet;
    shapeSet.schemaVersion = root.value(QStringLiteral("schemaVersion")).toInt(1);
    shapeSet.id = root.value(QStringLiteral("id")).toString();
    shapeSet.displayName = root.value(QStringLiteral("displayName")).toString();
    shapeSet.tabDepth = root.value(QStringLiteral("tabDepth")).toDouble(0.2);
    shapeSet.samplesPerBezier = root.value(QStringLiteral("samplesPerBezier")).toInt(5);

    const QJsonArray profiles = root.value(QStringLiteral("profiles")).toArray();
    for (const QJsonValue& profileValue : profiles)
    {
        const QJsonObject profileObject = profileValue.toObject();
        PuzzleEdgeProfile profile;
        profile.id = profileObject.value(QStringLiteral("id")).toString();
        profile.weight = profileObject.value(QStringLiteral("weight")).toInt(1);

        const QJsonArray beziers = profileObject.value(QStringLiteral("beziers")).toArray();
        for (const QJsonValue& bezierValue : beziers)
        {
            const QJsonArray values = bezierValue.toArray();
            if (values.size() != 6)
                throw std::runtime_error("A puzzle edge Bezier must contain six numbers");
            profile.beziers.push_back({pointFromArray(values, 0),
                                        pointFromArray(values, 2),
                                        pointFromArray(values, 4)});
        }

        if (profile.id.isEmpty() || profile.weight <= 0 || profile.beziers.isEmpty())
            throw std::runtime_error("A puzzle edge profile is incomplete");
        shapeSet.profiles.push_back(std::move(profile));
    }

    if (shapeSet.schemaVersion != 1 || shapeSet.id.isEmpty() || shapeSet.profiles.isEmpty()
        || shapeSet.tabDepth <= 0.0 || shapeSet.samplesPerBezier < 1)
    {
        throw std::runtime_error("The puzzle shape set is incomplete or unsupported");
    }

    return shapeSet;
}
