#pragma once

#include <qstring.h>
#include <qvector.h>

class SysInfo
{
public:
    struct VideoCardInfo
    {
        QString _manufacturer;
        QString _cardName;
        QString _cardFamily;
        QString _driverVersion;
        QString _driverDate;
    };

    struct OSInfo
    {
        QString	_kernelType;
        QString	_kernelVersion;
        QString	_prettyProductName;
        QString	_productType;
        QString	_productVersion;
        QString _manufacturer;
        QString _spMajor;
        QString _spMinor;
    };

    struct AudioInfo
    {
        QString _deviceName;
    };

public:
    explicit SysInfo();

    QVector<VideoCardInfo> getVideoCardInfo();
    OSInfo getOSInfo();
    QVector<AudioInfo> getAudioCardInfo();
};
