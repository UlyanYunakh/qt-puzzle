#ifndef PUZZLESHAPELOADER_H
#define PUZZLESHAPELOADER_H

#include "puzzleshapeset.h"

class QString;

class PuzzleShapeLoader
{
public:
    [[nodiscard]] static PuzzleShapeSet load(const QString& resourcePath);
};

#endif // PUZZLESHAPELOADER_H
