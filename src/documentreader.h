#ifndef DOCUMENTREADER_H
#define DOCUMENTREADER_H

#include <string>
#include <vector>
#include <dlfcn.h>
#include <PasspR.h>
#include <RFID.h>
#include <QObject>
#include <QLibrary>
#include <QVariant>
#include <functional>

class DocumentReader
{

private:
    bool passpr40Connected = false;
    QString passpr40LibName = "/usr/lib/regula/sdk/libPasspR40.so";
    QLibrary passrp40Lib;

    bool hasDocument;
    _LibraryVersionFunc LibraryVersion;
    _SetCallbackFuncFunc SetCallbackFunc;
    _InitializeFunc Initialize;
    _FreeFunc Free;
    _ExecuteCommandFunc ExecuteCommand;
    _CheckResultFunc CheckResult;

    _CheckResultFromListFunc CheckResultFromList;
    _ResultTypeAvailableFunc ResultTypeAvailable;

    bool RFIDConnected;
    QString RFIDLibName = "/usr/lib/regula/sdk/libRFID_SDK.so";
    QLibrary RFIDLib;

    bool hasRfid;
    rfid::_RFID_LibraryVersion RFID_LibraryVersion;
    rfid::_RFID_Initialize RFID_Initialize;
    rfid::_RFID_Free RFID_Free;
    rfid::_RFID_SetCallbackFunc RFID_SetCallbackFunc;
    rfid::_RFID_ExecuteCommand RFID_ExecuteCommand;
    rfid::_RFID_CheckResult RFID_CheckResult;
    rfid::_RFID_CheckResultFromList RFID_CheckResultFromList;

    TRegulaDeviceProperties *deviceProps;

    static DocumentReader *reader;

    static void ResultReceivingCallback(TResultContainer *result, uint32_t *PostAction, uint32_t *PostActionParameter);
    static void NotifyCallback(intptr_t code, intptr_t value);
    static NotifyFunc notificationCallback;

    static void RFID_NotifyCallback(int code, void *value);
    static RFID_NotifyFunc rfidNotificationCallback;

public:
    DocumentReader();
    ~DocumentReader();
    bool IsConnected();
    bool IsRFIDConnected();
    bool HasDocument();
    long Connect(const std::string name);
    long ConnectPasspr();
    long ConnectRFID();
    long Disconnect();
    long DisconnectPasspr();
    long DisconnectRfid();
    long Process(intptr_t processingMode);
    long Calibrate();
    long SetAuthenticityChecks(intptr_t authCheckMode);
    long GetReaderResultsCount(eRPRM_ResultType resultType);
    std::string GetTextField(const eVisualFieldType fieldType);
    std::string GetTextField(const std::vector<eVisualFieldType>& fieldType);
    std::string GetRfidKey();
    std::string GetReaderResult(eRPRM_ResultType resultType, long index, long &pageIndex, eRPRM_OutputFormat format = eRPRM_OutputFormat::ofrFormat_XML);
    std::vector<uint8_t> GetReaderResultImage(eRPRM_ResultType resultType, long index, std::string &lightType, long &pageIndex);
    std::vector<uint8_t> GetReaderResultFromList(eRPRM_ResultType resultType, long index, long elementIndex, long &pageIndex, std::string& fieldType);
    std::vector<uint8_t> GetReaderResultFromList(TResultContainer* resultContainer, long index, long& fieldType);
    std::string GetRfidResultXml(eRFID_ResultType resultType);
    std::vector<uint8_t> GetRfidResultFromList(eRFID_ResultType resultType, long elementIndex, std::string& fieldType);
    std::vector<uint8_t> GetRfidResultFromList(TResultContainer* resultContainer, long index, long& fieldType);
    static std::string LightNameFromIndex(eRPRM_Lights light);
    static std::string GraphicNameFromType(eGraphicFieldType type);
    void SetNotificationCallback(NotifyFunc notificationFunction) { notificationCallback = notificationFunction; }

    std::function<void(TResultContainer*)> VdCallback;
    bool enableVd = false;
    bool enableJson = false;

    std::string getFileExtension();
    std::string getDeviceInfo();
};

#endif // DOCUMENTREADER_H
