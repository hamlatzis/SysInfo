#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "sysinfo.h"

#include <qdebug.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    SysInfo sysInfo{};
    QVector<SysInfo::VideoCardInfo> videoCards = sysInfo.getVideoCardInfo();
    SysInfo::OSInfo operatingSystemInfo = sysInfo.getOSInfo();
    QVector<SysInfo::AudioInfo> audioCards = sysInfo.getAudioCardInfo();

    qDebug() << "************* Video Cards *************";
    for ( const auto& value : videoCards )
    {
        qDebug() << "Manufacturer :" << value._manufacturer;
        qDebug() << "Card Name :" << value._cardName;
        qDebug() << "Card Family :" << value._cardFamily;
        qDebug() << "Driver Version :" << value._driverVersion;
        qDebug() << "Driver Date :" << value._driverDate;
        qDebug() << "";
    }
    qDebug() << "************* OS information *************";

    qDebug() << "Kernel Type :" << operatingSystemInfo._kernelType;
    qDebug() << "Kernel Version :" << operatingSystemInfo._kernelVersion;
    qDebug() << "Product Name :" << operatingSystemInfo._prettyProductName;
    qDebug() << "Product Type :" << operatingSystemInfo._productType;
    qDebug() << "Product Version :" << operatingSystemInfo._productVersion;

    qDebug() << "************* Audio information *************";
    for ( const auto& value : audioCards )
    {
        qDebug() << "Device Name :" << value._deviceName;
        qDebug() << "";
    }


    return app.exec();
}
