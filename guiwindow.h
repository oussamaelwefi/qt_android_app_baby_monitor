#ifndef GUIWINDOW_H
#define GUIWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
// #include <QTextEdit> // Removed
#include <QVBoxLayout>
#include "bleclient.h"
#include <QLineEdit>
#include <QTimer>

#include <QDebug>
#include <QFile>
#include <QStandardPaths>
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>


#include "initialformwindow.h"

class GuiWindow : public QWidget
{
    Q_OBJECT

public:
    explicit GuiWindow(BleClient *client, QWidget *parent = nullptr);
    ~GuiWindow() override = default;

    void setNotification(const QString &notification);
    QString notification() const;
    void handleFormData(const BabyData& data);

signals:
    void notificationChanged();

private slots:
    void updateStatus(const QString &newStatus);
    void updateData(const QString &newData);
    void updateScanButtonState();
    void onTestButtonClicked();
    void updateAndroidNotification();

private:
    BleClient *m_bleClient;
    BabyData m_babyData;

    // UI Widgets
    QLabel *m_statusLabel;
    QLabel *m_tempLabel;
    QLabel *m_hrLabel;
    QLabel *m_predictionResultLabel; // <-- ADDED
    QPushButton *m_scanButton;
    QPushButton *m_disconnectButton;
    QPushButton *m_testButton;

    QTimer *m_predictionTimer;

    QString m_notification;

    void setupUi();
    void setupConnections();
    QString testPrediction(); // Return string modified for easier parsing
};

#endif // GUIWINDOW_H
