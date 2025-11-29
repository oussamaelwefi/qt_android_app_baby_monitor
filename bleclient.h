#ifndef BLECLIENT_H
#define BLECLIENT_H

#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QByteArray>
#include <QUuid>

// UUIDs for the ESP32 Service and Characteristic
// Match these to the ESP32 sketch!
const QUuid SERVICE_UUID("{4fafc201-1fb5-459e-8fcc-c5c9c331914b}");
const QUuid CHARACTERISTIC_UUID("{beb5483e-36e1-4688-b7f5-ea07361b26a8}");

class BleClient : public QObject
{
    Q_OBJECT
    // 1. Properties exposed to QML (used for signals)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString data READ data NOTIFY dataReceived)
    Q_PROPERTY(bool isScanning READ isScanning NOTIFY scanFinished)

public:
    explicit BleClient(QObject *parent = nullptr);

    // Getters for the properties (REQUIRED by Q_PROPERTY)
    QString status() const { return m_status; }
    QString data() const { return m_data; }
    bool isScanning() const { return m_isScanning; }

public slots:
    void startScan();
    void disconnectDevice();

signals:
    // Signals to notify the UI of state changes
    void statusChanged(const QString &newStatus);
    void dataReceived(const QString &newData);
    void scanFinished();

private slots:
    // Discovery
    void deviceDiscovered(const QBluetoothDeviceInfo &device);
    void scanError(QBluetoothDeviceDiscoveryAgent::Error error);

    // Connection
    void deviceConnected();
    void deviceDisconnected();
    void controllerError(QLowEnergyController::Error error);

    // Service & Characteristic
    void serviceDiscovered(const QBluetoothUuid &uuid);
    void serviceScanDone();
    void serviceStateChanged(QLowEnergyService::ServiceState newState);
    void characteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void serviceError(QLowEnergyService::ServiceError newError);
private:
    QBluetoothDeviceDiscoveryAgent *m_deviceDiscoveryAgent = nullptr;
    QLowEnergyController *m_control = nullptr;
    QLowEnergyService *m_service = nullptr;
    QBluetoothDeviceInfo m_deviceInfo;

    // Private member variables holding the state
    QString m_status;
    QString m_data;
    bool m_isScanning = false;

    void setStatus(const QString &newStatus);
    void setIsScanning(bool scanning);
    void setData(const QString &newData);
};

#endif // BLECLIENT_H
