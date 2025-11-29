#include "guiwindow.h"

// Required for layout management
#include <QHBoxLayout>
#include <QFrame>
#include <QDateTime>

#include <QDir>
#include <QFileInfo>

#include <QtCore/qjniobject.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/private/qandroidextras_p.h>
#include <QStringLiteral>

GuiWindow::GuiWindow(BleClient *client, QWidget *parent)
    : QWidget(parent), m_bleClient(client)
{
    setWindowTitle(tr("ESP32 BLE Client"));
    setMinimumSize(300, 400);

    setupUi();
    setupConnections();

    // Set initial state from client properties
    updateStatus(m_bleClient->status());
    updateData(m_bleClient->data());
    updateScanButtonState();

    if (QNativeInterface::QAndroidApplication::sdkVersion() >= __ANDROID_API_T__) {
        const auto notificationPermission = QStringLiteral("android.permission.POST_NOTIFICATIONS");        auto requestResult = QtAndroidPrivate::requestPermission(notificationPermission);
        if (requestResult.result() != QtAndroidPrivate::Authorized) {
            qWarning() << "Failed to acquire permission to post notifications "
                          "(required for Android 13+)";
        }
    }

    connect(this, &GuiWindow::notificationChanged,
            this, &GuiWindow::updateAndroidNotification);

    // QTimer for periodic prediction (10 seconds)
    m_predictionTimer = new QTimer(this);
    m_predictionTimer->setInterval(10000);
    connect(m_predictionTimer, &QTimer::timeout, this, &GuiWindow::onTestButtonClicked);
    m_predictionTimer->start();
}

void GuiWindow::setupUi()
{
    // Main vertical layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // --- Status Area ---
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("font-size: 18px; font-weight: bold; padding: 5px;");
    m_statusLabel->setTextFormat(Qt::RichText);
    mainLayout->addWidget(m_statusLabel);

    // --- Data Area: Two Labels for Temp and HR ---
    QHBoxLayout *dataDisplayLayout = new QHBoxLayout();
    dataDisplayLayout->setSpacing(20);
    dataDisplayLayout->setContentsMargins(10, 10, 10, 10);

    // 1. Temperature Label Setup
    m_tempLabel = new QLabel("Temperature: <br><b>-- °C</b>", this);
    m_tempLabel->setAlignment(Qt::AlignCenter);
    m_tempLabel->setTextFormat(Qt::RichText);
    m_tempLabel->setStyleSheet("QLabel { background-color: #E0F2F1; border-radius: 8px; padding: 15px; font-size: 16px; font-weight: bold; color: #004D40; border: 1px solid #B2DFDB; }");
    m_tempLabel->setMinimumHeight(80);

    // 2. Heart Rate Label Setup
    m_hrLabel = new QLabel("Heart Rate: <br><b>-- BPM</b>", this);
    m_hrLabel->setAlignment(Qt::AlignCenter);
    m_hrLabel->setTextFormat(Qt::RichText);
    m_hrLabel->setStyleSheet("QLabel { background-color: #E3F2FD; border-radius: 8px; padding: 15px; font-size: 16px; font-weight: bold; color: #1565C0; border: 1px solid #BBDEFB; }");
    m_hrLabel->setMinimumHeight(80);

    dataDisplayLayout->addWidget(m_tempLabel);
    dataDisplayLayout->addWidget(m_hrLabel);

    mainLayout->addLayout(dataDisplayLayout);

    // --- Prediction Result Label ---
    m_predictionResultLabel = new QLabel("Prediction: Not Run", this);
    m_predictionResultLabel->setAlignment(Qt::AlignCenter);
    m_predictionResultLabel->setTextFormat(Qt::RichText);
    m_predictionResultLabel->setStyleSheet("QLabel { background-color: #F3F4F6; border-radius: 8px; padding: 10px; font-size: 18px; font-weight: bold; color: #374151; border: 2px solid #D1D5DB; margin: 10px; }");
    m_predictionResultLabel->setMinimumHeight(60);
    mainLayout->addWidget(m_predictionResultLabel);
    // -------------------------------

    // --- Control Buttons ---
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_scanButton = new QPushButton(tr("Start Scan"), this);
    m_scanButton->setStyleSheet("QPushButton { background-color: #2563EB; color: white; font-weight: bold; border-radius: 8px; padding: 10px; }"
                                "QPushButton:disabled { background-color: #9CA3AF; }");

    m_disconnectButton = new QPushButton(tr("Disconnect"), this);
    m_disconnectButton->setStyleSheet("QPushButton { background-color: #DC2626; color: white; font-weight: bold; border-radius: 8px; padding: 10px; }"
                                      "QPushButton:disabled { background-color: #9CA3AF; }");

    m_testButton = new QPushButton(tr("Test"), this);
    m_testButton->setStyleSheet("QPushButton { background-color: #2563EB; color: white; font-weight: bold; border-radius: 8px; padding: 10px; }"
                                "QPushButton:disabled { background-color: #9CA3AF; }");

    buttonLayout->addWidget(m_scanButton);
    buttonLayout->addWidget(m_disconnectButton);

    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(m_testButton);

}

void GuiWindow::setupConnections()
{
    // Connections from UI to BleClient methods
    connect(m_scanButton, &QPushButton::clicked, m_bleClient, &BleClient::startScan);
    connect(m_disconnectButton, &QPushButton::clicked, m_bleClient, &BleClient::disconnectDevice);
    connect(m_testButton, &QPushButton::clicked, this, &GuiWindow::onTestButtonClicked);


    // Connections from BleClient signals to UI update slots
    connect(m_bleClient, &BleClient::statusChanged, this, &GuiWindow::updateStatus);
    connect(m_bleClient, &BleClient::dataReceived, this, &GuiWindow::updateData);
    connect(m_bleClient, &BleClient::scanFinished, this, &GuiWindow::updateScanButtonState);
}

void GuiWindow::updateStatus(const QString &newStatus)
{
    // Update color based on status (similar to QML logic)
    QString color = (newStatus.startsWith("Connected") || newStatus.startsWith("Subscribed")) ? "#10B981" : "#F87171";
    m_statusLabel->setText(QString("<span style='color:%1'>Status: %2</span>").arg(color, newStatus));

    // Update disconnect button enabled state
    m_disconnectButton->setEnabled(newStatus.startsWith("Subscribed") || newStatus.startsWith("Connected"));
}

void GuiWindow::updateData(const QString &newData)
{
    qDebug() << "Received : " << newData;

    // Data is expected to be a comma-separated string, e.g., "37.5,120"
    QStringList parts = newData.split(',');
    if (parts.size() == 2) {
        bool okT, okHR;
        float temp = parts[0].toFloat(&okT);
        float hr = parts[1].toFloat(&okHR);

        if (okT && okHR) {
            // VITAL: Update the stored patient data with the received BLE values
            m_babyData.temperature_c = temp;
            m_babyData.heart_rate_bpm = hr;

            // Update the new Labels with Rich Text for bold values
            m_tempLabel->setText(QString("Temperature: <br><b>%1 °C</b>").arg(temp, 0, 'f', 1));
            m_hrLabel->setText(QString("Heart Rate: <br><b>%1 BPM</b>").arg(hr, 0, 'f', 0));

            qDebug() << "Updated Patient Data: Temp =" << m_babyData.temperature_c << ", HR =" << m_babyData.heart_rate_bpm;

        } else {
            // Handle invalid numeric data
            m_tempLabel->setText("Temperature: <br><b>ERR</b>");
            m_hrLabel->setText("Heart Rate: <br><b>ERR</b>");
            qDebug() << "Invalid numeric BLE Data: " << newData;
        }
    } else {
        // Handle incorrect format or unexpected data
        m_tempLabel->setText("Temperature: <br><b>WAITING</b>");
        m_hrLabel->setText("Heart Rate: <br><b>WAITING</b>");
        qDebug() << "Raw BLE Data: " << newData;
    }
}

/**
 * @brief Runs an inference test using the compiled ONNX Runtime and the model.
 * @return A QString containing the prediction result prefixed with "PREDICTION_LABEL:X" or "ERROR:X" for easy parsing.
 */
QString GuiWindow::testPrediction() {
    // --- 1. Define Model and Data Parameters ---
    // ... (unchanged) ...
    std::vector<const char*> input_node_names = {
        "gender", "gestational_age_weeks", "birth_weight_kg",
        "birth_length_cm", "age_days", "weight_kg",
        "length_cm", "temperature_c", "heart_rate_bpm"
    };

    const char* output_node_names[] = {"label", "probabilities"};

    std::vector<int64_t> single_input_shape = {1, 1};

    // Test data
    std::string gender_str = m_babyData.gender.toStdString();
    std::vector<float> numeric_values = {
        m_babyData.gestational_age_weeks,  // gestational_age_weeks
        m_babyData.birth_weight_kg,   // birth_weight_kg
        m_babyData.birth_length_cm,  // birth_length_cm
        m_babyData.age_days,  // age_days
        m_babyData.weight_kg,   // weight_kg
        m_babyData.length_cm,  // length_cm
        m_babyData.temperature_c,  // temperature_c
        m_babyData.heart_rate_bpm// heart_rate_bpm
    };

    // --- 2. Initialize ONNX Runtime Environment ---
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "OrtTestPrediction");
    Ort::SessionOptions session_options;

    // --- 3. Find and Copy Model File (unchanged) ---
    QString modelFilePath;
    const QString ASSET_QRC_PATH = ":/health_classifier.onnx";

    QFile assetFile(ASSET_QRC_PATH);
    if (!assetFile.open(QIODevice::ReadOnly)) {
        return QString("ERROR:Failed to open asset file: %1").arg(ASSET_QRC_PATH);
    }

    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    modelFilePath = tempDir + QDir::separator() + QFileInfo("health_classifier.onnx").fileName();

    if (!assetFile.copy(modelFilePath)) {
        return QString("ERROR:Failed to copy asset to temp location: %1").arg(modelFilePath);
    }
    assetFile.close();

    // --- 4. Load the Model and Create Session ---
    try {
        Ort::Session session(env, modelFilePath.toStdString().c_str(), session_options);

        // --- 5. Create 9 Separate Input Tensors ---
        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
        std::vector<Ort::Value> input_tensors;

        static const OrtApi* ortApi = OrtGetApiBase()->GetApi(ORT_API_VERSION);
        Ort::AllocatorWithDefaultOptions default_allocator;

        // 1. Create the 'gender' string tensor
        const char* gender_c_str[] = {gender_str.c_str()};
        OrtValue* gender_ort_value = nullptr;

        OrtStatus* status = ortApi->CreateTensorAsOrtValue(
            default_allocator,
            single_input_shape.data(),
            single_input_shape.size(),
            ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING,
            &gender_ort_value
            );

        if (status) {
            QString error_msg = QString("Failed to create empty string tensor structure: %1").arg(ortApi->GetErrorMessage(status));
            ortApi->ReleaseStatus(status);
            throw Ort::Exception(error_msg.toStdString().c_str(), ORT_FAIL);
        }

        status = ortApi->FillStringTensor(
            gender_ort_value,
            gender_c_str,
            1
            );

        if (status) {
            QString error_msg = QString("Failed to fill string tensor data: %1").arg(ortApi->GetErrorMessage(status));
            ortApi->ReleaseStatus(status);
            ortApi->ReleaseValue(gender_ort_value);
            throw Ort::Exception(error_msg.toStdString().c_str(), ORT_FAIL);
        }

        input_tensors.emplace_back(Ort::Value(gender_ort_value));

        // 2. Create the 8 float tensors
        for (float val : numeric_values) {
            float* buffer = new float[1];
            buffer[0] = val;

            input_tensors.emplace_back(
                Ort::Value::CreateTensor<float>(
                    memory_info,
                    buffer,
                    1,
                    single_input_shape.data(),
                    single_input_shape.size()
                    )
                );
        }

        // --- 6. Run Inference ---
        auto output_tensors = session.Run(
            Ort::RunOptions{nullptr},
            input_node_names.data(),
            input_tensors.data(),
            input_tensors.size(),
            output_node_names,
            2
            );

        // --- 7. Process Output ---
        int64_t predicted_label = output_tensors[0].GetTensorData<int64_t>()[0];
        const float* probabilities = output_tensors[1].GetTensorData<float>();
        size_t num_classes = output_tensors[1].GetTensorTypeAndShapeInfo().GetShape()[1];

        QString probabilityString = "";
        for (size_t i = 0; i < num_classes; ++i) {
            probabilityString += QString("Class %1: %2 | ").arg(i).arg(probabilities[i], 0, 'f', 4);
        }

        // --- 8. Cleanup and Return Result ---
        QFile::remove(modelFilePath);

        // MODIFIED RETURN FORMAT: Prefix with the predicted label for easy parsing in the slot
        return QString("PREDICTION_LABEL:%1 | Scores: %2 | Raw Data: Temp:%3, HR:%4")
            .arg(predicted_label)
            .arg(probabilityString)
            .arg(m_babyData.temperature_c, 0, 'f', 1)
            .arg(m_babyData.heart_rate_bpm, 0, 'f', 0);


    } catch (const Ort::Exception& e) {
        QFile::remove(modelFilePath);
        // MODIFIED ERROR RETURN: Prefix with ERROR:
        return QString("ERROR:ONNX Runtime Error: %1").arg(e.what());
    } catch (const std::exception& e) {
        QFile::remove(modelFilePath);
        // MODIFIED ERROR RETURN: Prefix with ERROR:
        return QString("ERROR:Standard C++ Error: %1").arg(e.what());
    }
}

void GuiWindow::onTestButtonClicked() {

    QString result = testPrediction();

    qDebug() << "Prediction Result:" << result;

    // --- Prediction Parsing and Label Update Logic ---
    if (result.startsWith("ERROR:")) {
        // Handle Error Case
        m_predictionResultLabel->setText(QString("❌ **Prediction Failed**<br>Details: %1").arg(result.mid(6)));
        m_predictionResultLabel->setStyleSheet("QLabel { background-color: #FEE2E2; border-radius: 8px; padding: 10px; font-size: 18px; font-weight: bold; color: #991B1B; border: 2px solid #FCA5A5; margin: 10px; }");
        setNotification("Prediction failed: Check debug logs.");
    } else if (result.startsWith("PREDICTION_LABEL:")) {
        // Handle Success Case
        QString labelString = result.mid(result.indexOf(':') + 1, 1); // Extract the single digit label (0 or 1)
        int predictedLabel = labelString.toInt();

        QString message;
        QString style;

        if (predictedLabel == 1) {
            // Class 1: At Risk (Warning/Danger Colors)
            message = "⚠️ **STATUS: AT RISK**";
            style = "QLabel { background-color: #FFFBEB; border-radius: 8px; padding: 10px; font-size: 18px; font-weight: bold; color: #92400E; border: 2px solid #FCD34D; margin: 10px; }";
            setNotification("Warning: Baby predicted to be AT RISK (Label 1).");
        } else {
            // Class 0: Not At Risk (Success/Safe Colors)
            message = "✅ **STATUS: NOT AT RISK**";
            style = "QLabel { background-color: #ECFDF5; border-radius: 8px; padding: 10px; font-size: 18px; font-weight: bold; color: #065F46; border: 2px solid #A7F3D0; margin: 10px; }";
            setNotification("Status normal: Baby predicted NOT AT RISK (Label 0).");
        }

        m_predictionResultLabel->setText(message);
        m_predictionResultLabel->setStyleSheet(style);
    }
    // -------------------------------------------------------------------

    emit notificationChanged();
}

void GuiWindow::handleFormData(const BabyData& data)
{
    m_babyData = data;

    // Debug output
    qDebug() << "--- Patient Data Stored Successfully ---";
    qDebug() << "  Gender:" << m_babyData.gender;
    qDebug() << "  GA (weeks):" << m_babyData.gestational_age_weeks;
    qDebug() << "  Birth Weight (kg):" << m_babyData.birth_weight_kg;
    qDebug() << "  Birth Length (cm):" << m_babyData.birth_length_cm;
    qDebug() << "  Age (days):" << m_babyData.age_days;
    qDebug() << "  Current Weight (kg):" << m_babyData.weight_kg;
    qDebug() << "  Current Length (cm):" << m_babyData.length_cm;
    qDebug() << "  Temperature (C, BLE init):" << m_babyData.temperature_c;
    qDebug() << "  Heart Rate (bpm, BLE init):" << m_babyData.heart_rate_bpm;
    qDebug() << "------------------------------------------";

    // Show the main BLE window
    this->show();

}

void GuiWindow::setNotification(const QString &notification)
{
    if (m_notification == notification)
        return;

    //! [notification changed signal]
    m_notification = notification;
    emit notificationChanged();
    //! [notification changed signal]
}

QString GuiWindow::notification() const
{
    return m_notification;
}

//! [Send notification message to Java]
void GuiWindow::updateAndroidNotification()
{
    QJniObject javaNotification = QJniObject::fromString(m_notification);
    QJniObject::callStaticMethod<void>(
        "org/qtproject/example/androidnotifier/NotificationClient",
        "notify",
        "(Landroid/content/Context;Ljava/lang/String;)V",
        QNativeInterface::QAndroidApplication::context(),
        javaNotification.object<jstring>());
}
void GuiWindow::updateScanButtonState()
{
    // The BleClient* m_bleClient must be available to call isScanning()
    bool isScanning = m_bleClient->isScanning();
    m_scanButton->setEnabled(!isScanning);
    m_scanButton->setText(isScanning ? tr("Scanning...") : tr("Start Scan"));
}
