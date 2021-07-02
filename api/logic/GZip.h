#pragma once
#include <QByteArray>

#include "multiservermc_logic_export.h"

class MULTISERVERMC_LOGIC_EXPORT GZip
{
public:
    static bool unzip(const QByteArray &compressedBytes, QByteArray &uncompressedBytes);
    static bool zip(const QByteArray &uncompressedBytes, QByteArray &compressedBytes);
};

