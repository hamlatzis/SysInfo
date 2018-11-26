#include "sysinfo.h"

#include <qdebug.h>

#define USE_WINDOWS_SPECIFIC 1

#if USE_WINDOWS_SPECIFIC
#include <comutil.h>
#include <Wbemcli.h>
#include <Wbemidl.h>

#include <initguid.h>  // Put this in to get rid of linker errors.

#include <Mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <PropIdl.h>

#else
#include <QScreen>
#include <QtOpenGL>
#include <QSysInfo>
#include <QAudioDeviceInfo>
#include <qlist.h>
#endif

#if USE_WINDOWS_SPECIFIC
class DllHelper
{
public:
    DllHelper() :   _coCreateInstance(nullptr),
                    _sysAllocString(nullptr),
                    _coSetProxyBlanket(nullptr),
                    _sysFreeString(nullptr),
                    _propVariantClear(nullptr)
    {
        _ole32DLL = LoadLibrary(L"ole32.dll");
        _oleAut32DLL = LoadLibrary(L"oleaut32.dll");

        if ( _ole32DLL && _oleAut32DLL )
        {
            _coCreateInstance = reinterpret_cast <functionCoCreateInstance>(GetProcAddress(_ole32DLL, "CoCreateInstance"));
            _sysAllocString = reinterpret_cast <functionSysAllocString>(GetProcAddress(_oleAut32DLL, "SysAllocString"));
            _coSetProxyBlanket = reinterpret_cast <functionCoSetProxyBlanket>(GetProcAddress(_ole32DLL, "CoSetProxyBlanket"));
            _sysFreeString = reinterpret_cast <functionSysFreeString>(GetProcAddress(_oleAut32DLL, "SysFreeString"));
            _propVariantClear = reinterpret_cast <functionPropVariantClear>(GetProcAddress(_ole32DLL, "PropVariantClear"));
        }

        _initialised = (_coCreateInstance && _sysAllocString && _coSetProxyBlanket && _sysFreeString && _propVariantClear);
    }

    ~DllHelper()
    {
        FreeLibrary(_oleAut32DLL);
        FreeLibrary(_ole32DLL);
    }

    bool isInitialised() const { return _initialised; }

public:
    typedef HRESULT(WINAPI *functionCoCreateInstance)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID*);
    typedef BSTR(WINAPI *functionSysAllocString)(const OLECHAR*);
    typedef HRESULT(WINAPI *functionCoSetProxyBlanket)(IUnknown*, DWORD, DWORD, OLECHAR*, DWORD, DWORD, RPC_AUTH_IDENTITY_HANDLE, DWORD);
    typedef void(WINAPI *functionSysFreeString)(BSTR);
    typedef HRESULT(WINAPI *functionPropVariantClear)(PROPVARIANT*);

    functionCoCreateInstance _coCreateInstance;
    functionSysAllocString _sysAllocString;
    functionCoSetProxyBlanket _coSetProxyBlanket;
    functionSysFreeString _sysFreeString;
    functionPropVariantClear _propVariantClear;

private:
    HMODULE _ole32DLL;
    HMODULE _oleAut32DLL;

    bool _initialised;
};
#endif

SysInfo::SysInfo()
{
}

QVector<SysInfo::VideoCardInfo> SysInfo::getVideoCardInfo()
{
    QVector<SysInfo::VideoCardInfo> videoCards;

#if USE_WINDOWS_SPECIFIC

    DllHelper dllHelper;

    if ( dllHelper.isInitialised() )
    {
        HRESULT hr;

        IWbemLocator *pIWbemLocator = nullptr;
        hr = dllHelper._coCreateInstance(__uuidof(WbemLocator), nullptr,
            CLSCTX_INPROC_SERVER, __uuidof(IWbemLocator),
            reinterpret_cast<LPVOID *>(&pIWbemLocator));

        auto bstrServer = dllHelper._sysAllocString(L"\\\\.\\root\\cimv2");
        IWbemServices *pIWbemServices;

        hr = pIWbemLocator->ConnectServer(bstrServer, nullptr, nullptr, 0L, 0L, nullptr,
            nullptr, &pIWbemServices);

        hr = dllHelper._coSetProxyBlanket(pIWbemServices, RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
            RPC_C_IMP_LEVEL_IMPERSONATE, nullptr,
            EOAC_DEFAULT);

        auto bstrWQL  = dllHelper._sysAllocString(L"WQL");
        auto bstrPath = dllHelper._sysAllocString(L"Select * from Win32_VideoController");
        IEnumWbemClassObject* pEnum;
        hr = pIWbemServices->ExecQuery(bstrWQL, bstrPath, WBEM_FLAG_FORWARD_ONLY, nullptr, &pEnum);

        IWbemClassObject* pObj = nullptr;
        ULONG uReturned;
        VARIANT var;

        while ( SUCCEEDED(pEnum->Next(WBEM_INFINITE, 1, &pObj, &uReturned)) && uReturned )
        {
            if ( uReturned )
            {
                VideoCardInfo videoCardInfo;

                // driver name
                auto driverName = dllHelper._sysAllocString(L"Name");
                hr = pObj->Get(driverName, 0, &var, nullptr, nullptr);
                if(SUCCEEDED(hr))
                    videoCardInfo._cardName = QString::fromWCharArray(var.bstrVal);
                dllHelper._sysFreeString(driverName);

                // driver version
                auto driverVersion = dllHelper._sysAllocString(L"DriverVersion");
                hr = pObj->Get(driverVersion, 0, &var, nullptr, nullptr);
                if(SUCCEEDED(hr))
                    videoCardInfo._driverVersion = QString::fromWCharArray(var.bstrVal);
                dllHelper._sysFreeString(driverVersion);

                // driver date
                auto driverDate = dllHelper._sysAllocString(L"DriverDate");
                hr = pObj->Get(driverDate, 0, &var, nullptr, nullptr);
                if(SUCCEEDED(hr))
                    videoCardInfo._driverDate = QString::fromWCharArray(var.bstrVal);
                dllHelper._sysFreeString(driverDate);

                // driver family
                auto videoProcessor = dllHelper._sysAllocString(L"VideoProcessor");
                hr = pObj->Get(videoProcessor, 0, &var, nullptr, nullptr);
                if(SUCCEEDED(hr))
                    videoCardInfo._cardFamily = QString::fromWCharArray(var.bstrVal);
                dllHelper._sysFreeString(videoProcessor);

                videoCards.append(videoCardInfo);
            }
        }

        pEnum->Release();
        dllHelper._sysFreeString(bstrPath);
        dllHelper._sysFreeString(bstrWQL);
        pIWbemServices->Release();
        dllHelper._sysFreeString(bstrServer);
    }

#else   // WMI
    for (auto screen : QGuiApplication::screens())
    {
        QOffscreenSurface surf;
        surf.create();

        QOpenGLContext ctx;
        ctx.setScreen(screen);
        ctx.create();
        ctx.makeCurrent(&surf);

        VideoCardInfo videoCardInfo;

        videoCardInfo._manufacturer = QString( reinterpret_cast<const char*>(ctx.functions()->glGetString(GL_VENDOR)) );
        videoCardInfo._cardName = QString( reinterpret_cast<const char*>(ctx.functions()->glGetString(GL_RENDERER)) );
        videoCardInfo._driverVersion = QString( reinterpret_cast<const char*>(ctx.functions()->glGetString(GL_VERSION)) );

        videoCards.append(videoCardInfo);
    }
#endif

    return videoCards;
}

SysInfo::OSInfo SysInfo::getOSInfo()
{
    OSInfo operatingSystemInfo;

#if USE_WINDOWS_SPECIFIC

    DllHelper dllHelper;

    if ( dllHelper.isInitialised() )
    {
        HRESULT hr;

        IWbemLocator *pIWbemLocator = nullptr;
        hr = dllHelper._coCreateInstance(__uuidof(WbemLocator), nullptr,
            CLSCTX_INPROC_SERVER, __uuidof(IWbemLocator),
            reinterpret_cast<LPVOID *>(&pIWbemLocator));

        auto bstrServer = dllHelper._sysAllocString(L"\\\\.\\root\\cimv2");
        IWbemServices *pIWbemServices;

        hr = pIWbemLocator->ConnectServer(bstrServer, nullptr, nullptr, 0L, 0L, nullptr,
            nullptr, &pIWbemServices);

        hr = dllHelper._coSetProxyBlanket(pIWbemServices, RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
            RPC_C_IMP_LEVEL_IMPERSONATE, nullptr,
            EOAC_DEFAULT);

        auto bstrWQL  = dllHelper._sysAllocString(L"WQL");
        auto bstrPath = dllHelper._sysAllocString(L"Select * from Win32_OperatingSystem ");
        IEnumWbemClassObject* pEnum;
        hr = pIWbemServices->ExecQuery(bstrWQL, bstrPath, WBEM_FLAG_FORWARD_ONLY, nullptr, &pEnum);

        IWbemClassObject* pObj = nullptr;
        ULONG uReturned;
        VARIANT var;

        hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &uReturned);

        if ( uReturned )
        {
            auto caption = dllHelper._sysAllocString(L"Caption");
            hr = pObj->Get(caption, 0, &var, nullptr, nullptr);
            if(SUCCEEDED(hr))
                operatingSystemInfo._prettyProductName = QString::fromWCharArray(var.bstrVal);
            dllHelper._sysFreeString(caption);

            auto manufacturer = dllHelper._sysAllocString(L"Manufacturer");
            hr = pObj->Get(manufacturer, 0, &var, nullptr, nullptr);
            if(SUCCEEDED(hr))
                operatingSystemInfo._manufacturer = QString::fromWCharArray(var.bstrVal);
            dllHelper._sysFreeString(manufacturer);

            auto servicePackMajorVersion = dllHelper._sysAllocString(L"ServicePackMajorVersion");
            hr = pObj->Get(servicePackMajorVersion, 0, &var, nullptr, nullptr);
            if(SUCCEEDED(hr))
                operatingSystemInfo._spMajor = QString::fromWCharArray(var.bstrVal);
            dllHelper._sysFreeString(servicePackMajorVersion);

            auto servicePackMinorVersion = dllHelper._sysAllocString(L"ServicePackMinorVersion");
            hr = pObj->Get(servicePackMinorVersion, 0, &var, nullptr, nullptr);
            if(SUCCEEDED(hr))
                operatingSystemInfo._spMinor = QString::fromWCharArray(var.bstrVal);
            dllHelper._sysFreeString(servicePackMinorVersion);

            auto version = dllHelper._sysAllocString(L"Version");
            hr = pObj->Get(version, 0, &var, nullptr, nullptr);
            if(SUCCEEDED(hr))
                operatingSystemInfo._kernelVersion = QString::fromWCharArray(var.bstrVal);
            dllHelper._sysFreeString(version);
        }

        pEnum->Release();
        dllHelper._sysFreeString(bstrPath);
        dllHelper._sysFreeString(bstrWQL);
        pIWbemServices->Release();
        dllHelper._sysFreeString(bstrServer);
    }

#else   // #if USE_WINDOWS_SPECIFIC

    operatingSystemInfo._kernelType = QSysInfo ::kernelType();
    operatingSystemInfo._kernelVersion = QSysInfo::kernelVersion();
    operatingSystemInfo._prettyProductName = QSysInfo::prettyProductName();
    operatingSystemInfo._productType = QSysInfo::productType();
    operatingSystemInfo._productVersion = QSysInfo::productVersion();

#endif

    return operatingSystemInfo;
}

QVector<SysInfo::AudioInfo> SysInfo::getAudioCardInfo()
{
    QVector<AudioInfo> audioCardInfo;

#if USE_WINDOWS_SPECIFIC

    DllHelper dllHelper;

    if ( dllHelper.isInitialised() )
    {
        HRESULT hr;
        IMMDeviceEnumerator *pEnumerator = nullptr;
        IMMDeviceCollection *iDevCollection = nullptr;

        hr = dllHelper._coCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<LPVOID *>(&pEnumerator));

        hr = pEnumerator->EnumAudioEndpoints(eAll, DEVICE_STATEMASK_ALL, &iDevCollection);

        IMMDevice *iMMDevice = nullptr;
        UINT devNum = 0;

        while ( SUCCEEDED( iDevCollection->Item(devNum++, &iMMDevice) ) )
        {
            IPropertyStore *iPropStore = nullptr;

            // Get/display the endpoint's friendly-name property
            if ( SUCCEEDED(iMMDevice->OpenPropertyStore(STGM_READWRITE, &iPropStore) ) )
            {
                PROPVARIANT varName;
                PropVariantInit(&varName);

                if ( SUCCEEDED(iPropStore->GetValue(PKEY_Device_FriendlyName, &varName) ) )
                {
                    AudioInfo audioInfo;
                    audioInfo._deviceName = QString::fromWCharArray(varName.pwszVal);

                    const auto items = audioCardInfo.size();
                    auto found = false;
                    auto item = 0;
                    while ( !found && (item < items) )
                    {
                        found = (audioInfo._deviceName == audioCardInfo.at(item)._deviceName);
                        ++item;
                    }

                    if ( !found )
                        audioCardInfo.append(audioInfo);

                   dllHelper._propVariantClear(&varName);
                }

                iPropStore->Release();
            }
        }

        iDevCollection->Release();
    }

#else

    const auto audioInputDevices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for ( const auto& value : audioInputDevices )
    {
        AudioInfo audioInfo;
        audioInfo._deviceName = value.deviceName();

        audioCardInfo.push_back(audioInfo);
    }

    const auto audioOutputDevices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    for ( const auto& value : audioOutputDevices )
    {
        AudioInfo audioInfo;
        audioInfo._deviceName = value.deviceName();

        audioCardInfo.push_back(audioInfo);
    }

#endif

    return audioCardInfo;
}
