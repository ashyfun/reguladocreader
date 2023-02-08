#include "documentreader.h"
#include <iostream>
#include <string>
#include <thread>
#include <QtXml/QtXml>
#include <QDebug>
#include <QThread>
#include <QMetaEnum>

using namespace rfid;

DocumentReader *DocumentReader::reader = nullptr;
NotifyFunc DocumentReader::notificationCallback = nullptr;
RFID_NotifyFunc DocumentReader::rfidNotificationCallback = nullptr;


DocumentReader::DocumentReader() :
    passpr40Connected(false),
    hasDocument(false),
    RFIDConnected(false)
{
    Disconnect();
    reader = this;
}

DocumentReader::~DocumentReader()
{
    Disconnect();
}

bool DocumentReader::IsConnected()
{
    return passpr40Connected;
}

bool DocumentReader::IsRFIDConnected()
{
    return RFIDConnected;
}

bool DocumentReader::HasDocument()
{
    return hasDocument;
}

void DocumentReader::ResultReceivingCallback(TResultContainer *result, uint32_t *PostAction, uint32_t *PostActionParameter)
{
    Q_UNUSED( PostAction )
    Q_UNUSED( PostActionParameter )

    if(!reader)
        return;

    reader->VdCallback(result);
}

void DocumentReader::NotifyCallback(intptr_t code, intptr_t value)
{
    qDebug() << "Notification" << code << ":" << value;
    switch (code)
    {
        case RPRM_Notification_DocumentReady:
            {
                if(reader)
                {
                    reader->hasDocument = value;
                }
            }
            break;
        case RPRM_Notification_Calibration:
            {
                if(reader)
                {
                    reader->hasDocument = reader->hasDocument;
                }
            }
            break;
        case RPRM_Notification_CalibrationProgress:
            {
                if(reader)
                {
                    reader->hasDocument = reader->hasDocument;
                }
            }
            break;
        case RPRM_Notification_CalibrationStep:
            {
                if(reader)
                {
                    reader->hasDocument = reader->hasDocument;
                }
            }
            break;
        default:
            break;
    }
    if(notificationCallback)
    {
        notificationCallback(code, value);
    }
}

void DocumentReader::RFID_NotifyCallback(int code, void *value)
{
    qDebug() << "RFID notification: " << Qt::hex << code << Qt::dec << ":" << value;
    switch(code)
    {
        case RFID_Notification_DocumentReady:
            qDebug() << "RFID_Notification_DocumentReady:" << (int)(intptr_t)value;
        break;
    }
    if(rfidNotificationCallback)
    {
        rfidNotificationCallback(code, value);
    }
}

long DocumentReader::Connect(const std::string name)
{
    Q_UNUSED( name )
    long result = RPRM_Error_Failed;
    result = ConnectPasspr();

    if(passpr40Connected && !RFIDConnected)
    {
        result = ConnectRFID();
    }
    return result;
}

long DocumentReader::ConnectPasspr()
{
    if (passpr40Connected)
        return RPRM_Error_AlreadyDone;

    long result = RPRM_Error_Failed;
    hasDocument = false;
    qDebug() << "Connecting PasspR40...";
    if (!passrp40Lib.isLoaded())
    {
        passrp40Lib.setFileName(passpr40LibName);
        passrp40Lib.setLoadHints(QLibrary::ResolveAllSymbolsHint);
        qDebug() << "Open PasspR40 lib...";
        if (!passrp40Lib.load())
        {
            qDebug() << "Open PasspR40 lib - FAILED by reason:" << passrp40Lib.errorString();
            return result;
        }
        qDebug() << "Open PasspR40 lib - DONE";
    }
    qDebug() << "Resolve PasspR40 lib symbols...";
    LibraryVersion      = reinterpret_cast<_LibraryVersionFunc>(passrp40Lib.resolve("_LibraryVersion"));
    SetCallbackFunc     = reinterpret_cast<_SetCallbackFuncFunc>(passrp40Lib.resolve("_SetCallbackFunc"));
    Initialize          = reinterpret_cast<_InitializeFunc>(passrp40Lib.resolve("_Initialize"));
    Free                = reinterpret_cast<_FreeFunc>(passrp40Lib.resolve("_Free"));
    ExecuteCommand      = reinterpret_cast<_ExecuteCommandFunc>(passrp40Lib.resolve("_ExecuteCommand"));
    CheckResult         = reinterpret_cast<_CheckResultFunc>(passrp40Lib.resolve("_CheckResult"));
    CheckResultFromList = reinterpret_cast<_CheckResultFromListFunc>(passrp40Lib.resolve("_CheckResultFromList"));
    ResultTypeAvailable = reinterpret_cast<_ResultTypeAvailableFunc>(passrp40Lib.resolve("_ResultTypeAvailable"));
    qDebug() << "Resolve PasspR40 lib symbols - DONE";

    uint32_t libVersion = LibraryVersion();
    qDebug() << "Library version:" << QString("%1.%2").arg(HIWORD(libVersion)).arg(LOWORD(libVersion));
    SetCallbackFunc(
        reinterpret_cast<ResultReceivingFunc>(&ResultReceivingCallback),
        reinterpret_cast<NotifyFunc>(&NotifyCallback)
    );
    qDebug() << "Start initialize...";
    Initialize(nullptr, nullptr);
    intptr_t doLog = 1;
    result = ExecuteCommand(RPRM_Command_Options_BuildExtLog, reinterpret_cast<void*>(doLog), nullptr);

    if(enableVd)
    {
        auto param = reinterpret_cast<void*>(static_cast<intptr_t>(true));
        ExecuteCommand(RPRM_Command_Device_UseVideoDetection, param, nullptr);
    }

    qDebug() << "Build log result:" << Qt::hex << result << Qt::dec;
    long devCount = 0;
    result = ExecuteCommand(RPRM_Command_Device_Count, nullptr, &devCount);
    qDebug () << "Devices count result:" << Qt::hex << result << Qt::dec << Qt::endl << "Found devices:" << devCount;
    if(devCount > 0)
    {
        qDebug() << "Device connecting start...";
        result = ExecuteCommand(RPRM_Command_Device_Connect, reinterpret_cast<void*>(-1), nullptr);
        qDebug() << "Device connecting result:" << Qt::hex << result << Qt::dec;

        if(result == RPRM_Error_NoError)
        {
            passpr40Connected = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    }
    else
    {
        result = RPRM_Error_NoDocumentReadersFound;
    }

    auto param = reinterpret_cast<void*>(static_cast<intptr_t>(true));
    ExecuteCommand(RPRM_Command_Options_Set_MultiPageProcessingMode, param, nullptr);

    if(enableVd)
    {
        ExecuteCommand(RPRM_Command_Options_Set_QuickMrzProcessing, param, nullptr);
    }

    ExecuteCommand(RPRM_Command_Options_GraphicFormat_Select, reinterpret_cast<void*>(1), nullptr);
    ExecuteCommand(RPRM_Command_Options_GraphicFormat_SetCompressionRatio, reinterpret_cast<void*>(10), nullptr);

    qDebug() << "Initialize - DONE";
    qDebug() << "Connecting PasspR40 - DONE";
    return result;
}

long DocumentReader::ConnectRFID()
{
    if (RFIDConnected)
        return RPRM_Error_AlreadyDone;

    long res = RPRM_Error_Failed;
    hasRfid = false;
    qDebug() << "Connecting RFID...";
    if (!RFIDLib.isLoaded())
    {
        RFIDLib.setFileName(RFIDLibName);
        RFIDLib.setLoadHints(QLibrary::ResolveAllSymbolsHint);
        qDebug() << "Open RFID lib...";
        if (!RFIDLib.load())
        {
            qDebug() << "Open RFID lib - FAILED by reason:" << RFIDLib.errorString();
            return res;
        }
        qDebug() << "Open RFID lib - DONE";
    }

    qDebug() << "Resolve RFID lib symbols...";
    RFID_LibraryVersion      = reinterpret_cast<_RFID_LibraryVersion>(RFIDLib.resolve("_RFID_LibraryVersion"));
    RFID_Initialize          = reinterpret_cast<_RFID_Initialize>(RFIDLib.resolve("_RFID_Initialize"));
    RFID_Free                = reinterpret_cast<_RFID_Free>(RFIDLib.resolve("_RFID_Free"));
    RFID_SetCallbackFunc     = reinterpret_cast<_RFID_SetCallbackFunc>(RFIDLib.resolve("_RFID_SetCallbackFunc"));
    RFID_ExecuteCommand      = reinterpret_cast<_RFID_ExecuteCommand>(RFIDLib.resolve("_RFID_ExecuteCommand"));
    RFID_CheckResult         = reinterpret_cast<_RFID_CheckResult>(RFIDLib.resolve("_RFID_CheckResult"));
    RFID_CheckResultFromList = reinterpret_cast<_RFID_CheckResultFromList>(RFIDLib.resolve("_RFID_CheckResultFromList"));
    qDebug() << "Resolve RFID lib symbols - DONE";

    qDebug() << "Start initialize...";
    intptr_t doLog = 0;
    res = RFID_ExecuteCommand(RFID_Command_BuildLog, reinterpret_cast<void*>(doLog), nullptr);
    qDebug() << "Build log result:" << Qt::hex << res << Qt::dec;
    uint32_t libVersion = RFID_LibraryVersion();
    qDebug() << "Library version:" << QString("%1.%2").arg(HIWORD(libVersion)).arg(LOWORD(libVersion));
    res = RFID_Initialize(0);
    qDebug() << "RFID initialize result:" << Qt::hex << res << Qt::dec;
    RFID_SetCallbackFunc((RFID_NotifyFunc)&RFID_NotifyCallback);
    long devCount = 0;
    res = RFID_ExecuteCommand(RFID_Command_Get_DeviceCount, nullptr, &devCount);
    qDebug () << "Devices count result:" << Qt::hex << res << Qt::dec << Qt::endl << "Found devices:" << devCount;
    if(devCount > 0)
    {
        int regulaReaderIndex = 0;
        for(int devIter = 0; devIter < devCount; ++devIter)
        {
            char* devDesc = nullptr;
            res = RFID_ExecuteCommand(RFID_Command_Get_DeviceDescription, reinterpret_cast<void*>(devIter), &devDesc);
            if(res == RFID_Error_NoError && devDesc)
            {
                qDebug() << devIter << ":" << devDesc;
                std::string devDescString(devDesc);
                if ((devDescString.find("Regula") != std::string::npos) &&
                        (devDescString.find("RFID") != std::string::npos))
                {
                    regulaReaderIndex = devIter;
                    break;
                }
            }
        }
        qDebug() << "RFID device " << regulaReaderIndex << " connecting start...";
        res = RFID_ExecuteCommand(RFID_Command_Set_CurrentDevice, reinterpret_cast<void*>(regulaReaderIndex), nullptr);
        qDebug() << "RFID device connecting result:" << Qt::hex << res << Qt::dec;
        if(res == RFID_Error_NoError)
        {
            RFIDConnected = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    }

    qDebug() << "Initialize - DONE";
    qDebug() << "Connecting RFID - DONE";
    return res;
}

long DocumentReader::Disconnect()
{
    qDebug() << "Disconnecting... ";
    long result = 0;
    result = DisconnectRfid();
    qDebug() << "RFID disconnecting result:" << Qt::hex << result << Qt::dec;
    result = DisconnectPasspr();
    qDebug() << "PasspR40 disconnecting result:" << Qt::hex << result << Qt::dec;
    return result;
}

long DocumentReader::DisconnectPasspr()
{
    if(passpr40Connected)
    {
        passpr40Connected = false;
        if(Free)
            Free();
    }
    LibraryVersion = nullptr;
    SetCallbackFunc = nullptr;
    Initialize = nullptr;
    Free = nullptr;
    ExecuteCommand = nullptr;
    CheckResult = nullptr;
    CheckResultFromList = nullptr;
    ResultTypeAvailable = nullptr;
    if(passrp40Lib.isLoaded())
    {
        qDebug() << "Close PasspR40 lib...";
        if (!passrp40Lib.unload())
        {
            qDebug() << "Close PasspR40 lib - FAILED by reason:" << passrp40Lib.errorString();
            return RPRM_Error_Failed;
        }
        qDebug() << "Close PasspR40 lib - DONE";
    }
    return RPRM_Error_NoError;
}

long DocumentReader::DisconnectRfid()
{
    if(RFIDConnected)
    {
        RFIDConnected = false;
        if(RFID_Free)
            RFID_Free();
    }
    RFID_LibraryVersion = nullptr;
    RFID_Initialize = nullptr;
    RFID_Free = nullptr;
    RFID_SetCallbackFunc = nullptr;
    RFID_ExecuteCommand = nullptr;
    RFID_CheckResult = nullptr;
    RFID_CheckResultFromList = nullptr;
    if(RFIDLib.isLoaded())
    {
        qDebug() << "Close RFID lib...";
        if (!RFIDLib.unload())
        {
            qDebug() << "Close RFID lib - FAILED by reason:" << RFIDLib.errorString();
            return RPRM_Error_Failed;
        }
        qDebug() << "Close RFID lib - DONE";
    }
    return RPRM_Error_NoError;
}


long DocumentReader::Process(intptr_t processingMode)
{
     long result = RPRM_Error_NoError;
     try
     {
         if(RFIDConnected && RFID_ExecuteCommand)
         {
             RFID_ExecuteCommand(RFID_Command_Session_Close, nullptr, nullptr);
             RFID_ExecuteCommand(RFID_Command_ClearResults, nullptr, nullptr);
         }

         result = ExecuteCommand(RPRM_Command_Process, (void*)processingMode, nullptr);
         if(result == RPRM_Error_NoError)
         {
             result = ExecuteCommand(RPRM_Command_OCRLexicalAnalyze, nullptr, nullptr);
             if((result == RPRM_Error_NoError) && RFIDConnected)
             {
                // create scenario XML
                // with MRZ/CAN
                std::string rfidScenario = R"({"RFIDTEST_OPTIONS":{"AuthProcType":2,"AuxVerification_CommunityID":false,"AuxVerification_DateOfBirth":false,"BaseSMProcedure":1,"OnlineTA":false,"OnlineTAToSignDataType":0,"PACE_StaticBinding":false,"PKD_DSCert_Priority":false,"PKD_EAC":"","PKD_PA":"","PKD_UseExternalCSCA":false,"PassiveAuth":true,"Perform_RestrictedIdentification":false,"ProfilerType":1,"ReadingBuffer":0,"SkipAA":false,"StrictProcessing":false,"TerminalType":1,"TrustedPKD":false,"UniversalAccessRights":false,"Use_SFI":false,"Write_eID":false,"SignManagementAction":0,"eSignPIN_Default":"","eSignPIN_NewValue":"","Authorized_ST_Signature":false,"Authorized_ST_QSignature":false,"Authorized_Write_DG17":false,"Authorized_Write_DG18":false,"Authorized_Write_DG19":false,"Authorized_Write_DG20":false,"Authorized_Write_DG21":false,"Authorized_Verify_Age":false,"Authorized_Verify_CommunityID":false,"Authorized_PrivilegedTerminal":false,"Authorized_CAN_Allowed":false,"Authorized_PIN_Managment":false,"Authorized_Install_Cert":false,"Authorized_Install_QCert":false,"Read_ePassport":true,"ePassport":{"DG1":true,"DG2":true,"DG3":true,"DG4":true,"DG5":true,"DG6":true,"DG7":true,"DG8":true,"DG9":true,"DG10":true,"DG11":true,"DG12":true,"DG13":true,"DG14":true,"DG15":true,"DG16":true},"Read_eID":false,"Read_eDL":false,"PACEPasswordType":1,"MRZ":")";
                rfidScenario += GetRfidKey();
                rfidScenario += R"("}})";
                char* scenarioResult = nullptr;
                int res = RFID_ExecuteCommand((int)RFID_Command_Scenario_Process, (void*)rfidScenario.c_str(), (void*)&scenarioResult);
                if(res == RFID_Error_NoError)
                {
                    result = RPRM_Error_NoError;
                }
             }
         }
     }
     catch(...)
     {

     }
     return result;
}

long DocumentReader::Calibrate()
{
    long result = 0;
    result = ExecuteCommand(RPRM_Command_Device_Calibration, nullptr, nullptr);
    return result;
}

long DocumentReader::SetAuthenticityChecks(intptr_t authCheckMode)
{
    long result = 0;
    result = ExecuteCommand(RPRM_Command_Options_Set_AuthenticityCheckMode, (void*)authCheckMode, nullptr);
    return result;
}


long DocumentReader::GetReaderResultsCount(eRPRM_ResultType resultType)
{
    long result = 0;
    if(passpr40Connected && ResultTypeAvailable)
    {
        result = ResultTypeAvailable(resultType);
    }
    return result;
}

std::string DocumentReader::GetTextField(const eVisualFieldType fieldType)
{
    std::vector<eVisualFieldType> typesVector {fieldType};
    return GetTextField(typesVector);
}

std::string DocumentReader::GetTextField(const std::vector<eVisualFieldType>& fieldType)
{
    std::string result;
    if(passpr40Connected && CheckResult)
    {
        HANDLE hResult = CheckResult(RPRM_ResultType_OCRLexicalAnalyze, 0, 0, 0);
        if(reinterpret_cast<intptr_t>(hResult) > 0)
        {
            auto lexResult = static_cast<TListVerifiedFields*>((static_cast<TResultContainer*>(hResult))->buffer);
            if(lexResult && lexResult->Count && lexResult->pFieldMaps)
            {
                for(uint32_t i = 0; i < lexResult->Count && result.empty(); ++i)
                {
                    eVisualFieldType tmpType = static_cast<eVisualFieldType>(lexResult->pFieldMaps[i].FieldType);
                    if(std::find(fieldType.begin(), fieldType.end(), tmpType) != fieldType.end())
                    {
                        if(lexResult->pFieldMaps[i].Field_RFID)
                            result = lexResult->pFieldMaps[i].Field_RFID;
                        else if(lexResult->pFieldMaps[i].Field_MRZ)
                            result = lexResult->pFieldMaps[i].Field_MRZ;
                        else if(lexResult->pFieldMaps[i].Field_Barcode)
                            result = lexResult->pFieldMaps[i].Field_Barcode;
                        else if(lexResult->pFieldMaps[i].Field_Visual)
                            result = lexResult->pFieldMaps[i].Field_Visual;
                    }
                }
            }
        }
        else
        {
            hResult = CheckResult(RPRM_ResultType_MRZ_OCR_Extended, 0, 0, 0);
            if(reinterpret_cast<intptr_t>(hResult) > 0)
            {
                auto mrzResult = static_cast<TDocVisualExtendedInfo*>((static_cast<TResultContainer*>(hResult))->buffer);
                if(mrzResult && mrzResult->nFields && mrzResult->pArrayFields)
                {
                    for(uint32_t i = 0; i < mrzResult->nFields && result.empty(); ++i)
                    {
                        eVisualFieldType tmpType = static_cast<eVisualFieldType>(mrzResult->pArrayFields[i].FieldType);
                        if(std::find(fieldType.begin(), fieldType.end(), tmpType) != fieldType.end())
                        {
                            if(mrzResult->pArrayFields[i].Buf_Text)
                            {
                                result = mrzResult->pArrayFields[i].Buf_Text;
                            }
                        }
                    }
                }
            }
        }
    }
    return result;
}

std::string DocumentReader::GetRfidKey()
{
    std::string result = GetTextField(ft_MRZ_Strings_ICAO_RFID);
    if(result.empty())
    {
        std::vector<eVisualFieldType> fieldType { ft_MRZ_Strings, ft_Card_Access_Number };
        result = GetTextField(ft_MRZ_Strings);
    }
    size_t index = 0;
    while (true)
    {
         index = result.find("^", index);
         if (index == std::string::npos)
             break;
         result.replace(index, 1, "");
    }
    return result;
}

std::string DocumentReader::getFileExtension() {
    return enableJson ? ".json" : ".xml";
}

std::string DocumentReader::GetReaderResult(
    eRPRM_ResultType resultType,
    long index,
    long &pageIndex,
    eRPRM_OutputFormat format
)
{
    if (enableJson && resultType != eRPRM_ResultType::RPRM_ResultType_Graphics) {
        format = eRPRM_OutputFormat::ofrFormat_JSON;
    }

    Q_UNUSED( pageIndex )
    std::string result;
    if(passpr40Connected)
    {
        HANDLE resultContainerHandle = CheckResult(resultType, index, format, 0);
        if((intptr_t)resultContainerHandle >= 0)
        {
            TResultContainer *resContainer = (TResultContainer*)resultContainerHandle;
            if(resContainer->result_type == resultType)
            {
                result = (char*)resContainer->XML_buffer;
            }
        }
    }
    return result;
}

std::vector<uint8_t> DocumentReader::GetReaderResultImage(eRPRM_ResultType resultType, long index, std::string &lightType, long &pageIndex)
{
    std::vector<uint8_t> result;
    if(passpr40Connected)
    {
        HANDLE resultContainerHandle = CheckResult(resultType, index, ofrFormat_FileBuffer, 0);
        if((intptr_t)resultContainerHandle >= 0)
        {
            TResultContainer *resContainer = (TResultContainer*)resultContainerHandle;
            if(resContainer->result_type == resultType)
            {
                pageIndex = resContainer->page_idx;
                lightType = LightNameFromIndex((eRPRM_Lights)resContainer->light);
                result = std::vector<uint8_t>((uint8_t*)resContainer->buffer, (uint8_t*)((uintptr_t)resContainer->buffer + resContainer->buf_length));
            }
        }
    }
    return result;
}

std::vector<uint8_t> DocumentReader::GetReaderResultFromList(eRPRM_ResultType resultType, long index, long elementIndex, long &pageIndex, std::string& fieldType)
{
    std::vector<uint8_t> result;
    if(passpr40Connected)
    {
        HANDLE resultContainerHandle = CheckResult(resultType, index, 0, 0);
        if((intptr_t)resultContainerHandle >= 0)
        {
            TResultContainer *resContainer = (TResultContainer*)resultContainerHandle;
            if(resContainer->result_type == resultType)
            {
                pageIndex = resContainer->page_idx;
                long fieldId = 0;
                result = GetReaderResultFromList(resContainer, elementIndex, fieldId);
                if((resultType == RPRM_ResultType_Graphics) ||
                        (resultType == RPRM_ResultType_BarCodes_ImageData))
                {
                    fieldType = GraphicNameFromType(static_cast<eGraphicFieldType>(fieldId));
                }
            }
        }
    }
    return result;
}

std::vector<uint8_t> DocumentReader::GetReaderResultFromList(TResultContainer* resultContainer, long index, long& fieldType)
{
    std::vector<uint8_t> result;
    if(passpr40Connected && resultContainer)
    {
        resultContainer->list_idx = index;
        TResultContainer resContainer{};
        long res = CheckResultFromList((HANDLE)resultContainer, ofrFormat_FileBuffer, (void*)&resContainer);
        if(res && resContainer.buf_length && resContainer.buffer)
        {
            fieldType = res;
            result = std::vector<uint8_t>((uint8_t*)resContainer.buffer, (uint8_t*)((uintptr_t)resContainer.buffer + resContainer.buf_length));
        }
    }
    return result;
}

std::string DocumentReader::GetRfidResultXml(eRFID_ResultType resultType)
{
    std::string result;
    if(RFIDConnected && RFID_CheckResult)
    {
        HANDLE resultContainerHandle = RFID_CheckResult(resultType, ofXML, 0);
        if((intptr_t)resultContainerHandle >= 0)
        {
            auto resContainer = (TResultContainer*)resultContainerHandle;
            if((resContainer->result_type == resultType) &&
                    resContainer->XML_buffer && resContainer->XML_length)
            {
                result = reinterpret_cast<char*>(resContainer->XML_buffer);
            }
        }
    }
    return result;
}

std::vector<uint8_t> DocumentReader::GetRfidResultFromList(eRFID_ResultType resultType, long elementIndex, std::string& fieldType)
{
    std::vector<uint8_t> result;
    if(RFIDConnected && RFID_CheckResult)
    {
        HANDLE resultContainerHandle = RFID_CheckResult(resultType, 0, 0);
        if((intptr_t)resultContainerHandle >= 0)
        {
            auto resContainer = (TResultContainer*)resultContainerHandle;
            if(resContainer->result_type == resultType)
            {
                long fieldId = 0;
                result = GetRfidResultFromList(resContainer, elementIndex, fieldId);
                if(resultType == RFID_ResultType_RFID_ImageData)
                {
                    fieldType = GraphicNameFromType(static_cast<eGraphicFieldType>(fieldId));
                }
            }
        }
    }
    return result;
}

std::vector<uint8_t> DocumentReader::GetRfidResultFromList(TResultContainer* resultContainer, long index, long& fieldType)
{
    std::vector<uint8_t> result;
    if(RFIDConnected && resultContainer && RFID_CheckResultFromList)
    {
        resultContainer->list_idx = index;
        std::string imgNameString("img.bmp"); //TODO: make jpg after R25775
        TResultContainer resContainer{};
        resContainer.XML_buffer = (BYTE*)imgNameString.data();
        resContainer.XML_length = imgNameString.size();
        long res = RFID_CheckResultFromList((HANDLE)resultContainer, ofrFormat_FileBuffer, (void*)&resContainer);
        if(res && resContainer.buf_length && resContainer.buffer)
        {
            fieldType = res;
            result = std::vector<uint8_t>((uint8_t*)resContainer.buffer, (uint8_t*)((uintptr_t)resContainer.buffer + resContainer.buf_length));
        }
    }
    return result;
}

bool replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

std::string DocumentReader::LightNameFromIndex(eRPRM_Lights light)
{
    std::string result;
    switch (light)
    {
    case RPRM_Light_White_Full:
        result = "RPRM_Light_White_Full";
        break;
    case RPRM_Light_White_Front:
        result = "RPRM_Light_White_Front";
        break;
    case RPRM_Light_White_Gray:
        result = "RPRM_Light_White_Gray";
        break;
    case RPRM_Light_White_Bottom:
        result = "RPRM_Light_White_Bottom";
        break;
    case RPRM_Light_White_Side:
        result = "RPRM_Light_White_Side";
        break;
    case RPRM_Light_White_Top:
        result = "RPRM_Light_White_Top";
        break;
    case RPRM_Light_IR_Full:
        result = "RPRM_Light_IR_Full";
        break;
    case RPRM_Light_IR_Front:
        result = "RPRM_Light_IR_Front";
        break;
    case RPRM_Light_IR_Bottom:
        result = "RPRM_Light_IR_Bottom";
        break;
    case RPRM_Light_IR_Side:
        result = "RPRM_Light_IR_Side";
        break;
    case RPRM_Light_IR_Top:
        result = "RPRM_Light_IR_Top";
        break;
    case RPRM_Light_UV:
        result = "RPRM_Light_UV";
        break;
    default:
        result = "<unknown>";
        break;
    }
    replace(result, "RPRM_Light_", "");
    return result;
}

std::string DocumentReader::GraphicNameFromType(eGraphicFieldType type)
{
    std::string result;
    switch (type)
    {
    case gf_Portrait:
        result = "gf_Portrait";
        break;
    case gf_Fingerprint:
        result = "gf_Fingerprint";
        break;
    case gf_Eye:
        result = "gf_Eye";
        break;
    case gf_Signature:
        result = "gf_Signature";
        break;
    case gf_BarCode:
        result = "gf_BarCode";
        break;
    case gf_Proof_Of_Citizenship:
        result = "gf_Proof_Of_Citizenship";
        break;
    case gf_Document_Front:
        result = "gf_Document_Front";
        break;
    case gf_Document_Rear:
        result = "gf_Document_Rear";
        break;
    case gf_ColorDynamic:
        result = "gf_ColorDynamic";
        break;
    case gf_GhostPortrait:
        result = "gf_GhostPortrait";
        break;
    case gf_Other:
        result = "gf_Other";
        break;
    case gf_Finger_LeftThumb:
        result = "gf_Finger_LeftThumb";
        break;
    case gf_Finger_LeftIndex:
        result = "gf_Finger_LeftIndex";
        break;
    case gf_Finger_LeftMiddle:
        result = "gf_Finger_LeftMiddle";
        break;
    case gf_Finger_LeftRing:
        result = "gf_Finger_LeftRing";
        break;
    case gf_Finger_LeftLittle:
        result = "gf_Finger_LeftLittle";
        break;
    case gf_Finger_RightThumb:
        result = "gf_Finger_RightThumb";
        break;
    case gf_Finger_RightIndex:
        result = "gf_Finger_RightIndex";
        break;
    case gf_Finger_RightMiddle:
        result = "gf_Finger_RightMiddle";
        break;
    case gf_Finger_RightRing:
        result = "gf_Finger_RightRing";
        break;
    case gf_Finger_RightLittle:
        result = "gf_Finger_RightLittle";
        break;
    default:
        result = "<unknown>";
        break;
    }
    replace(result, "gf_", "");
    return result;
}
