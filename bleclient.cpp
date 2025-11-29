#include "bleclient.h"
#include <QDebug>
#include <QList>
#include <QThread>
#include <QTimer> // QTimer header kept for other potential uses, but singleShot removed

// --- Helper Setters (Manage state and emit signals) ---
void BleClient::setStatus(const QString &newStatus)
{
    if (m_status != newStatus) {
        m_status = newStatus;
        emit statusChanged(m_status);
    }
}

void BleClient::setIsScanning(bool scanning)
{
    if (m_isScanning != scanning) {
        m_isScanning = scanning;
        // Emit signal when scanning status changes (used by the GUI to enable/disable buttons)
        emit scanFinished();
    }
}

void BleClient::setData(const QString &newData)
{
    // In a real application, you might append data instead of overwriting,
    // but for the status demonstration, we overwrite.
    if (m_data != newData) {
        m_data = newData;
        emit dataReceived(m_data);
        //qDebug() << "Received : " << m_data;
    }
}

// --- Constructor ---

BleClient::BleClient(QObject *parent)
    : QObject(parent)
{
    setStatus(tr("Ready to scan."));
    m_deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);

    // Connect discovery signals
    connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
            this, &BleClient::deviceDiscovered);
    connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, &BleClient::scanError);
    connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, [this]() {
                if (m_isScanning) {
                    setIsScanning(false);
                    if (m_control == nullptr) {
                        setStatus(tr("Scan finished. Device not found."));
                    }
                }
            });
}

// --- Public Slots ---

void BleClient::startScan()
{
    if (m_isScanning)
        return;

    // Clean up previous connection if any
    disconnectDevice();

    setStatus(tr("Scanning for ESP32-CAM-Data..."));
    setIsScanning(true);

    // Start device discovery (scanning)
    m_deviceDiscoveryAgent->start();
}

void BleClient::disconnectDevice()
{
    if (m_control) {
        m_control->disconnectFromDevice();
        // The QLowEnergyController will be deleted in deviceDisconnected slot
    }
    // Only reset status if not currently scanning
    if (!m_isScanning) {
        setStatus(tr("Disconnected. Ready to scan."));
    }
}

// --- Discovery Slots ---

void BleClient::deviceDiscovered(const QBluetoothDeviceInfo &device)
{
    // Filter for Low Energy devices by name
    if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration &&
        device.name() == "ESP32-CAM-Data") {

        setStatus(tr("Target found. Connecting..."));
        m_deviceDiscoveryAgent->stop(); // Stop scanning immediately after finding the target
        setIsScanning(false);
        m_deviceInfo = device;

        // Create the low energy controller (responsible for connection management)
        m_control = QLowEnergyController::createCentral(m_deviceInfo, this);

        // Connect connection signals
        connect(m_control, &QLowEnergyController::connected,
                this, &BleClient::deviceConnected);
        connect(m_control, &QLowEnergyController::disconnected,
                this, &BleClient::deviceDisconnected);
        connect(m_control, &QLowEnergyController::errorOccurred,
                this, &BleClient::controllerError);
        connect(m_control, &QLowEnergyController::serviceDiscovered,
                this, &BleClient::serviceDiscovered);
        connect(m_control, &QLowEnergyController::discoveryFinished,
                this, &BleClient::serviceScanDone);

        // Initiate connection
        m_control->connectToDevice();
    }
}

void BleClient::scanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    Q_UNUSED(error);
    setIsScanning(false);
    setStatus(tr("Error during scan: ") + m_deviceDiscoveryAgent->errorString());
}

// --- Connection Slots ---

void BleClient::deviceConnected()
{
    setStatus(tr("Connected. Discovering services..."));

    // The key step immediately after connection: start service discovery.
    // The stability issue will be solved on the ESP32 side (see section 2).
    m_control->discoverServices();
}

void BleClient::deviceDisconnected()
{
    setStatus(tr("Disconnected. Ready to scan."));
    if (m_control) {
        m_control->deleteLater();
        m_control = nullptr;
    }
    m_service = nullptr;
}

void BleClient::controllerError(QLowEnergyController::Error error)
{
    Q_UNUSED(error);
    setStatus(tr("Connection Error: ") + m_control->errorString());
}

// --- Service & Characteristic Slots ---

void BleClient::serviceDiscovered(const QBluetoothUuid &uuid)
{
    // Explicitly cast SERVICE_UUID (QUuid) to QBluetoothUuid to avoid ambiguity
    if (uuid == QBluetoothUuid(SERVICE_UUID)) {
        setStatus(tr("Target service found."));
    }
}

void BleClient::serviceScanDone()
{
    // Check if the desired service was found
    // Explicitly cast SERVICE_UUID to QBluetoothUuid to resolve ambiguity in QList::contains
    if (m_control->services().contains(QBluetoothUuid(SERVICE_UUID))) {
        setStatus(tr("Service details discovered."));

        // Create the service object
        m_service = m_control->createServiceObject(SERVICE_UUID, this);
        if (!m_service) {
            setStatus(tr("Error: Cannot create service object."));
            return;
        }

        // Connect service state signal
        connect(m_service, &QLowEnergyService::stateChanged,
                this, &BleClient::serviceStateChanged);

        // Start discovering service characteristics (details)
        if (m_service->state() == QLowEnergyService::RemoteService) {
            m_service->discoverDetails();
        } else {
            // Some platforms (like macOS) may have details available immediately
            serviceStateChanged(m_service->state());
        }

    } else {
        setStatus(tr("Target service not found."));
        disconnectDevice();
    }
}

void BleClient::serviceStateChanged(QLowEnergyService::ServiceState newState)
{
    // Use the Qt 6 compliant enum value: RemoteServiceDiscovered
    if (newState == QLowEnergyService::RemoteServiceDiscovered) {
        setStatus(tr("Subscribing to characteristic..."));

        // Find the characteristic using the target UUID
        QLowEnergyCharacteristic dataChar = m_service->characteristic(CHARACTERISTIC_UUID);

        if (!dataChar.isValid()) {
            setStatus(tr("Error: Data characteristic not found."));
            return;
        }

            connect(m_service, &QLowEnergyService::characteristicRead, this, &BleClient::characteristicChanged);

        // Connect to characteristic value change signal
        connect(m_service, &QLowEnergyService::characteristicChanged,
                this, &BleClient::characteristicChanged);

        connect(m_service, &QLowEnergyService::errorOccurred,
                this, &BleClient::serviceError);

        // Find the Client Characteristic Configuration Descriptor (CCCD)
        // Use the raw 16-bit UUID (0x2902) which is stable across Qt versions
        QLowEnergyDescriptor notificationDesc = dataChar.descriptor(
            QBluetoothUuid(quint16(0x2902)));

        if (notificationDesc.isValid()) {
            // Write the value to enable notifications (0x0001)
            // Use writeDescriptor on the service object
            m_service->writeDescriptor(notificationDesc, QByteArray::fromHex("0100"));
            setStatus(tr("Subscribed successfully. Waiting for data..."));
        } else {
            setStatus(tr("Warning: Cannot subscribe (CCCD not found)."));
        }
    }
}

void BleClient::characteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    // Explicitly cast CHARACTERISTIC_UUID (QUuid) to QBluetoothUuid to avoid ambiguity
    if (characteristic.uuid() == QBluetoothUuid(CHARACTERISTIC_UUID)) {
        // The data is assumed to be a simple UTF-8 string from the ESP32
        QString receivedString = QString::fromUtf8(value);
        setData(receivedString);

    }
}

void BleClient::serviceError(QLowEnergyService::ServiceError newError)
{
    // This slot is called if ANY service operation fails, including the CCCD write.
    if (newError != QLowEnergyService::NoError) {

        setStatus(tr("Service Error (Code %1). Disconnecting.").arg(newError));

        disconnectDevice();
    }
}
