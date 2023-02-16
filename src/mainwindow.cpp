#include "ui_mainwindow.h"
#include "mainwindow.h"

#include <QPlainTextEdit>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QMessageBox>
#include <QShortcut>
#include <QSettings>
#include <QFile>
#include <QTextStream>

#include <iostream>
#include <fstream>
#include <sstream>
#include <functional>
//#include <tiffio.h>

MainWindow* MainWindow::currentWindow = nullptr;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    currentWindow = this;
    ui->setupUi(this);

    QSettings ui_settings("RegulaDocumentReader", "ui_settings");
    ui->VdCheckBox->setChecked(ui_settings.value("checkbox/videoDetection").toBool());
    // ui->JsonCheckBox->setChecked(ui_settings.value("checkbox/jsonFormat").toBool());
    if (ui_settings.contains("checkbox/autoscan")) {
        ui->AutoscanCheckBox->setChecked(ui_settings.value("checkbox/autoscan").toBool());
    }

    connect(this, SIGNAL(documentInserted()), SLOT(on_DocumentInserted()));
    connect(this, SIGNAL(askCalibrationOject(int)), SLOT(on_AskCalibrationObject(int)));
    connect(this, SIGNAL(deviceDisconnected()), SLOT(on_DeviceDisconnected()));
    connect(this, SIGNAL(containerIsReady(TResultContainer*)), SLOT(showVdResults(TResultContainer*)));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this, SLOT(close()));

    std::vector<std::string> headers;
    headers.push_back("Content-Type: multipart/form-data");
    headers.push_back("Connection: close");

    sender = new DocumentSender();
    sender->setHeaders(headers);
}

MainWindow::~MainWindow()
{
    QSettings ui_settings("RegulaDocumentReader", "ui_settings");
    ui_settings.setValue("checkbox/videoDetection", ui->VdCheckBox->isChecked());
    // ui_settings.setValue("checkbox/jsonFormat", ui->JsonCheckBox->isChecked());
    ui_settings.setValue("checkbox/autoscan", ui->AutoscanCheckBox->isChecked());

    delete ui;
    delete sender;
}

void MainWindow::setStates(bool readerIsConnected)
{
    ui->ConnectButton->setEnabled(!readerIsConnected);
    ui->DisconnectButton->setEnabled(readerIsConnected);
    ui->ProcessButton->setEnabled(readerIsConnected);
    ui->CalibrateButton->setEnabled(readerIsConnected);
    ui->VdCheckBox->setEnabled(!readerIsConnected);
    // ui->JsonCheckBox->setEnabled(!readerIsConnected);
    ui->AutoscanCheckBox->setEnabled(!readerIsConnected);
}

void MainWindow::on_ConnectButton_clicked()
{
    Reader.enableVd = ui->VdCheckBox->isChecked();
    Reader.enableJson = ui->JsonCheckBox->isChecked();
    Reader.enableAutoscan = ui->AutoscanCheckBox->isChecked();

    Reader.SetNotificationCallback(&MainWindow::StaticNotificationCallbackHandler);
    Reader.VdCallback = MainWindow::VdResultsHandler;
    Reader.Connect("");
    setStates(Reader.IsConnected());
}

void MainWindow::on_DisconnectButton_clicked()
{
    isDocumentProcessed = false;
    Reader.Disconnect();
    ui->tabWidget->clear(); // clear results
    setStates(Reader.IsConnected());
}

void MainWindow::InsertTextTabsForContainer(const TResultContainer *container, const std::string &labelBase)
{
    std::string xmlString { (char*) container->XML_buffer, container->XML_length };

    if(xmlString.empty())
        return;

    std::filebuf fb;
    fb.open ("tmp/" + labelBase + Reader.getFileExtension(), std::ios::out);
    std::ostream os(&fb);
    os << xmlString;
    fb.close();
    QPlainTextEdit *textEdit = new QPlainTextEdit(QString::fromStdString(xmlString), ui->tabWidget);
    std::string tabName = labelBase;
    ui->tabWidget->insertTab(0, textEdit, tabName.c_str());
}

void MainWindow::InsertTextTabsForType(eRPRM_ResultType type, const std::string& labelBase)
{
    long pageIndex = 0;
    auto count = Reader.GetReaderResultsCount(type);

    for(int i = 0; i < count; i++)
    {
        std::string xmlString = Reader.GetReaderResult(type, i, pageIndex);

        if(xmlString.empty())
            continue;

        if (Reader.enableJson) {
            if (labelBase == "Lex" && !lexJson.length()) {
                lexJson.assign(xmlString);
            } else if (labelBase == "DocType" && !docTypeJson.length() && lexJson.length()) {
                docTypeJson.assign(xmlString);
            }
        }

        std::filebuf fb;
        fb.open ("tmp/" + labelBase + "_" + std::to_string(i) + Reader.getFileExtension(), std::ios::out);
        std::ostream os(&fb);
        os << xmlString;
        fb.close();
        QPlainTextEdit *textEdit = new QPlainTextEdit(QString::fromStdString(xmlString), ui->tabWidget);
        std::string tabName = labelBase + "_" + std::to_string(i);
        ui->tabWidget->insertTab(0, textEdit, tabName.c_str());
    }
}

void MainWindow::showVdResults(TResultContainer* container)
{
    ClearTabs();

    if(container->result_type != RPRM_ResultType_OCRLexicalAnalyze)
        return;

    InsertTextTabsForContainer(container, "Lex");
}

void MainWindow::on_ProcessButton_clicked()
{
    ClearTabs();
    if(Reader.IsConnected())
    {
        auto proc_start = std::chrono::high_resolution_clock::now();
        try
        {
            intptr_t authCheckMode = (intptr_t)-1;
            Reader.SetAuthenticityChecks(authCheckMode);
            intptr_t processMode =
                RPRM_GetImage_Modes_GetImages
                | RPRM_GetImage_Modes_LocateDocument
                | RPRM_GetImage_Modes_OCR_MRZ
                | RPRM_GetImage_Modes_OCR_Visual
                | RPRM_GetImage_Modes_OCR_BarCodes
                | RPRM_GetImage_Modes_Authenticity
                | RPRM_GetImage_Modes_DocumentType
            ;

            if(Reader.Process(processMode) == RPRM_Error_NoError)
            {
                InsertTextTabsForType(RPRM_ResultType_OCRLexicalAnalyze, "Lex");
                InsertTextTabsForType(RPRM_ResultType_Authenticity, "Auth");
                InsertTextTabsForType(RPRM_ResultType_ChosenDocumentTypeCandidate, "DocType");

                long resultsCount = Reader.GetReaderResultsCount(RPRM_ResultType_RawImage);
                long docType = 0;
                std::string docSerial = "";
                for(long i = 0; i < resultsCount; ++i) // and images
                {
                    std::string lightType;
                    long pageIndex;
                    std::vector<uint8_t> imageVector = Reader.GetReaderResultImage(RPRM_ResultType_RawImage, i, lightType, pageIndex);
                    QImage qimg;
                    qimg.loadFromData(imageVector.data(), imageVector.size());
                    QGraphicsScene* scene = new QGraphicsScene();
                    scene->addPixmap(QPixmap::fromImage(qimg));
                    QGraphicsView *view = new QGraphicsView(scene);
                    view->setScene(scene);
                    view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
                    view->update();
                    ui->tabWidget->insertTab(ui->tabWidget->count(), view, QString(lightType.c_str()));

                    Json::Reader::MemberValue tmp;
                    if (docTypeJson.length()) {
                        Json::Reader *docTypeReader = new Json::Reader(docTypeJson);

                        docTypeReader->fetch("OneCandidate", "FDSIDList", "dType");
                        tmp = docTypeReader->getValue(Json::Reader::MemberType::Int);
                        docType = tmp.mvInt;

                        sender->addMimePart("type", std::to_string(docType));

                        docTypeJson = "";
                        delete docTypeReader;
                    }

                    if (lexJson.length()) {
                        Json::Reader *lexReader = new Json::Reader(lexJson);

                        lexReader->fetch("ListVerifiedFields", "pFieldMaps");
                        tmp = lexReader->searchElement("FieldType", "Field_Visual", 165);
                        docSerial = tmp.mvString;

                        sender->addMimePart("data", lexJson);

                        lexJson = "";
                        delete lexReader;
                    }

                    if (docType > 0 && docSerial.length()) {
                        QString filename = QString(std::string("tmp/%1_" + docSerial + "_%2.jpg").c_str())
                            .arg(docType)
                            .arg(pageIndex + 1);
                        qimg.save(filename);

                        sender->addMimePart("files", filename.toStdString(), true);
                    }
                }

                sender->addMimePart("deviceInfo", Reader.getDeviceInfo());
                if (sender->howManyMimeParts() > 3) {
                    sender->doPost("http://posts.elros.info/api/v1/regula/parse/");
                }

                resultsCount = Reader.GetReaderResultsCount(RPRM_ResultType_Graphics);
                for(long i = 0; i < resultsCount; ++i) // and graphics
                {
                    long pageIndex = 0;
                    std::string graphicXml = Reader.GetReaderResult(RPRM_ResultType_Graphics, i, pageIndex);
                    if(!graphicXml.empty())
                    {
                        std::stringstream ss;
                        ss << "tmp/graphic_" << i << ".xml";
                        std::fstream fstream;
                        fstream.open(ss.str(), std::ios_base::out | std::ios_base::binary);
                        fstream.write((const char *)graphicXml.data(), graphicXml.size());
                        fstream.close();
                    }
                    int graphicIndex = 0;
                    std::vector<uint8_t> graphicBufffer;
                    do
                    {
                        std::string fieldName;
                        graphicBufffer = Reader.GetReaderResultFromList(RPRM_ResultType_Graphics, i, graphicIndex, pageIndex, fieldName);
                        if(!graphicBufffer.empty() && !fieldName.empty())
                        {
                            QImage qimg;
                            qimg.loadFromData(graphicBufffer.data(), graphicBufffer.size());
                            QGraphicsScene* scene = new QGraphicsScene();
                            scene->addPixmap(QPixmap::fromImage(qimg));
                            QGraphicsView *view = new QGraphicsView(scene);
                            view->setScene(scene);
                            view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
                            view->update();
                            ui->tabWidget->insertTab(ui->tabWidget->count(), view, QString(fieldName.c_str()));

                            std::stringstream ss;
                            ss << "tmp/graphic_" << i << "_" << graphicIndex << "_" << fieldName << ".jpg";
                            std::fstream fstream;
                            fstream.open(ss.str(), std::ios_base::out | std::ios_base::binary);
                            fstream.write((const char *)graphicBufffer.data(), graphicBufffer.size());
                            fstream.close();
                        }
                        ++graphicIndex;
                    } while(!graphicBufffer.empty());
                }
                if(Reader.IsRFIDConnected())
                {
                    int graphicIndex = 0;
                    std::vector<uint8_t> graphicBufffer;
                    do
                    {
                        std::string fieldName;
                        graphicBufffer = Reader.GetRfidResultFromList(RFID_ResultType_RFID_ImageData, graphicIndex, fieldName);
                        if(!graphicBufffer.empty() && !fieldName.empty())
                        {
                            QImage qimg;
                            qimg.loadFromData(graphicBufffer.data(), graphicBufffer.size());
                            QGraphicsScene* scene = new QGraphicsScene();
                            scene->addPixmap(QPixmap::fromImage(qimg));
                            QGraphicsView *view = new QGraphicsView(scene);
                            view->setScene(scene);
                            view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
                            view->update();
                            ui->tabWidget->insertTab(ui->tabWidget->count(), view, QString(fieldName.c_str()));

                            std::stringstream ss;
                            ss << "tmp/rfid_" << graphicIndex << "_" << fieldName << ".jpg";
                            std::fstream fstream;
                            fstream.open(ss.str(), std::ios_base::out | std::ios_base::binary);
                            fstream.write((const char *)graphicBufffer.data(), graphicBufffer.size());
                            fstream.close();
                        }
                        ++graphicIndex;
                    } while(!graphicBufffer.empty());

                    std::string rfidResult = Reader.GetRfidResultXml(eRFID_ResultType::RFID_ResultType_RFID_BinaryData);
                    if(!rfidResult.empty())
                    {
                        std::filebuf fb;
                        fb.open ("tmp/rfid_binary.xml",std::ios::out);
                        std::ostream os(&fb);
                        os << rfidResult;
                        fb.close();
                        QPlainTextEdit *textEdit = new QPlainTextEdit(QString(rfidResult.c_str()), ui->tabWidget);
                        ui->tabWidget->insertTab(ui->tabWidget->count(), textEdit, "RFID binary");
                    }
                }
            }
        }
        catch (std::exception &ex)
        {

        }
        catch (...)
        {

        }
        auto proc_finish = std::chrono::high_resolution_clock::now();
        std::cout << "Processing time: " << std::chrono::duration<float>(proc_finish - proc_start).count() << std::endl;
    }
}

void MainWindow::on_CalibrateButton_clicked()
{
    ClearTabs();
    if(Reader.IsConnected())
    {
        long result = Reader.Calibrate();
        result = result;
    }
}

void MainWindow::on_AskCalibrationObject(int index)
{
    std::stringstream ss;
    ss << "Insert calibration object " << index << " and press OK";
    QMessageBox::information(this, QString("Calibration"), QString(ss.str().c_str()), QMessageBox::Ok);
}

void MainWindow::on_DocumentInserted()
{
    if (Reader.enableAutoscan) {
        on_ProcessButton_clicked();
    }
}

void MainWindow::on_DeviceDisconnected()
{
    on_DisconnectButton_clicked();
}

void MainWindow::NotificationCallbackHandler(intptr_t code, intptr_t value)
{
    switch (code)
    {
        case RPRM_Notification_DocumentReady:
            {
                if(value && !isDocumentProcessed)
                {
                    isDocumentProcessed = true;
                    Q_EMIT documentInserted();
                }
                if(!value)
                    isDocumentProcessed = false;
            }
            break;
        case RPRM_Notification_CalibrationStep:
            {
                Q_EMIT askCalibrationOject(((int)value) + 1);
            }
            break;
        case RPRM_Notification_DeviceDisconnected:
            {
                if (Reader.IsConnected())
                {
                    Q_EMIT deviceDisconnected();
                }
            }
            break;
        default:
            break;
    }
}

void MainWindow::ClearTabs()
{
    for(int i = 0; i < ui->tabWidget->count(); ++i)
    {
        QWidget* widgetIter = ui->tabWidget->widget(i);
        if(widgetIter)
        {
            std::string className = widgetIter->metaObject()->className();
            if(className == "QGraphicsView")
            {
                QGraphicsView *view = (QGraphicsView*)widgetIter;
                view->scene()->clear();
            }
            else
            {
                QList<QGraphicsView *> allGraphicsViews = widgetIter->findChildren<QGraphicsView *>();
                for(int j = 0; j < allGraphicsViews.count(); j++)
                {
                    QGraphicsView *view = allGraphicsViews[j];
                    view->scene()->clear();
                }
            }
        }
    }
    ui->tabWidget->clear(); // clear results
}
