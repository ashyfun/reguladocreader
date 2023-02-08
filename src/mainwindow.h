#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"
#include <QMainWindow>
#include "documentreader.h"
#include "documentsender.h"
#include <thread>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

Q_SIGNALS:
    void documentInserted();
    void askCalibrationOject(int index);
    void deviceDisconnected();
    void containerIsReady(TResultContainer* container);

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void CheckInitiateScan()
    {
        if(Reader.IsConnected())
        {
            if(Reader.HasDocument())
            {
                on_ProcessButton_clicked();
            }
        }
    }

private Q_SLOTS:
    void on_ConnectButton_clicked();

    void on_DisconnectButton_clicked();

    void on_ProcessButton_clicked();

    void on_DocumentInserted();

    void on_CalibrateButton_clicked();

    void on_AskCalibrationObject(int index);

    void on_DeviceDisconnected();

    void showVdResults(TResultContainer* container);

private:
    static MainWindow* currentWindow;
    Ui::MainWindow *ui;
    DocumentReader Reader;
    bool isDocumentProcessed = false;

    std::string lex = "";
    std::string docType = "";
    DocumentSender *sender = nullptr;

    void NotificationCallbackHandler(intptr_t code, intptr_t value);

    static void StaticNotificationCallbackHandler(intptr_t code, intptr_t value)
    {
        if(currentWindow)
        {
            currentWindow->NotificationCallbackHandler(code, value);
        }
    }

    static void VdResultsHandler(TResultContainer* container)
    {
        if(!currentWindow)
            return;

        currentWindow->containerIsReady(container);
    }

    void ClearTabs();
    void InsertTextTabsForContainer(const TResultContainer* container, const std::string& labelBase);
    void InsertTextTabsForType(eRPRM_ResultType type, const std::string& labelBase);

    void setStates(bool);
};

#endif // MAINWINDOW_H
