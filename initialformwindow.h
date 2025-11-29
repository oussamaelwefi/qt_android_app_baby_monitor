#ifndef INITIALFORMWINDOW_H
#define INITIALFORMWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
struct BabyData {
    QString gender = "male";
    float gestational_age_weeks = 0.0f;
    float birth_weight_kg = 0.0f;
    float birth_length_cm = 0.0f;
    float age_days = 0.0f;
    float weight_kg = 0.0f;
    float length_cm = 0.0f;
    // NOTE: temperature_c and heart_rate_bpm will be received via BLE
    float temperature_c = 0.0f;
    float heart_rate_bpm = 0.0f;
};
// Include the struct definition from guiwindow.h
// Note: We need a forward declaration or include. Since the struct is simple,
// we define it again locally or rely on the main app structure, but the safest
// way is to ensure GuiWindow's header is included if we can't define the struct globally.
// For simplicity and dependency management, we will rely on the type being defined
// in guiwindow.h (which is included in guiwindow.cpp). Since this is a dedicated file,
// we will put the struct definition in guiwindow.h and ensure this file knows about it.

// Let's rely on the GuiWindow::PatientData definition, but for a clean signal definition,
// we might need to redefine or forward-declare. Since we included guiwindow.h in guiwindow.cpp,
// we'll explicitly include it here to ensure the struct is known for the signal.


class InitialFormWindow : public QWidget
{
    Q_OBJECT

public:
    explicit InitialFormWindow(QWidget *parent = nullptr);
    ~InitialFormWindow() override = default;

signals:
    // Signal to send the collected data back to the main window
    void dataSubmitted(const BabyData& data);

private slots:
    void submitForm();
    void forceUpdate(); // <-- ADDED: The slot for the redraw workaround

private:
    // Form fields
    QComboBox *m_genderInput;
    QLineEdit *m_gaWeeksInput;
    QLineEdit *m_birthWeightInput;
    QLineEdit *m_birthLengthInput;
    QLineEdit *m_ageDaysInput;
    QLineEdit *m_weightInput;
    QLineEdit *m_lengthInput;
    QPushButton *m_submitButton;

    void setupUi();
};

#endif // INITIALFORMWINDOW_H
