#include "initialformwindow.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QDoubleValidator> // Added this required header for completeness

InitialFormWindow::InitialFormWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("Initial Patient Data Form"));
    setFixedSize(400, 350); // Fixed size for a form

    setupUi();

    // Connect the submit button
    connect(m_submitButton, &QPushButton::clicked, this, &InitialFormWindow::submitForm);
}

void InitialFormWindow::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QLabel *titleLabel = new QLabel(tr("Enter Initial Patient Vitals"), this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; margin-bottom: 10px; color: #1E40AF;");
    mainLayout->addWidget(titleLabel, 0, Qt::AlignHCenter);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(12);
    formLayout->setLabelAlignment(Qt::AlignRight);

    // --- Input Fields ---

    // 1. Gender (String, ComboBox)
    m_genderInput = new QComboBox(this);
    m_genderInput->addItem("male");
    m_genderInput->addItem("female");
    formLayout->addRow(tr("Gender:"), m_genderInput);

    // 2. Gestational Age (float)
    m_gaWeeksInput = new QLineEdit(this);
    m_gaWeeksInput->setPlaceholderText(tr("e.g., 38.5"));
    m_gaWeeksInput->setValidator(new QDoubleValidator(0.0, 50.0, 1, m_gaWeeksInput));
    formLayout->addRow(tr("GA (Weeks):"), m_gaWeeksInput);
    connect(m_gaWeeksInput, &QLineEdit::textChanged, this, &InitialFormWindow::forceUpdate); // <-- WORKAROUND

    // 3. Birth Weight (float)
    m_birthWeightInput = new QLineEdit(this);
    m_birthWeightInput->setPlaceholderText(tr("e.g., 3.2"));
    m_birthWeightInput->setValidator(new QDoubleValidator(0.0, 10.0, 2, m_birthWeightInput));
    formLayout->addRow(tr("Birth Weight (kg):"), m_birthWeightInput);
    connect(m_birthWeightInput, &QLineEdit::textChanged, this, &InitialFormWindow::forceUpdate); // <-- WORKAROUND

    // 4. Birth Length (float)
    m_birthLengthInput = new QLineEdit(this);
    m_birthLengthInput->setPlaceholderText(tr("e.g., 50.0"));
    m_birthLengthInput->setValidator(new QDoubleValidator(0.0, 100.0, 2, m_birthLengthInput));
    formLayout->addRow(tr("Birth Length (cm):"), m_birthLengthInput);
    connect(m_birthLengthInput, &QLineEdit::textChanged, this, &InitialFormWindow::forceUpdate); // <-- WORKAROUND

    // 5. Age (float, days)
    m_ageDaysInput = new QLineEdit(this);
    m_ageDaysInput->setPlaceholderText(tr("e.g., 15"));
    m_ageDaysInput->setValidator(new QDoubleValidator(0.0, 1000.0, 0, m_ageDaysInput));
    formLayout->addRow(tr("Age (days):"), m_ageDaysInput);
    connect(m_ageDaysInput, &QLineEdit::textChanged, this, &InitialFormWindow::forceUpdate); // <-- WORKAROUND

    // 6. Current Weight (float)
    m_weightInput = new QLineEdit(this);
    m_weightInput->setPlaceholderText(tr("e.g., 3.5"));
    m_weightInput->setValidator(new QDoubleValidator(0.0, 10.0, 2, m_weightInput));
    formLayout->addRow(tr("Current Weight (kg):"), m_weightInput);
    connect(m_weightInput, &QLineEdit::textChanged, this, &InitialFormWindow::forceUpdate); // <-- WORKAROUND

    // 7. Current Length (float)
    m_lengthInput = new QLineEdit(this);
    m_lengthInput->setPlaceholderText(tr("e.g., 52.0"));
    m_lengthInput->setValidator(new QDoubleValidator(0.0, 100.0, 2, m_lengthInput));
    formLayout->addRow(tr("Current Length (cm):"), m_lengthInput);
    connect(m_lengthInput, &QLineEdit::textChanged, this, &InitialFormWindow::forceUpdate); // <-- WORKAROUND


    mainLayout->addLayout(formLayout);

    // Submit Button
    m_submitButton = new QPushButton(tr("Submit Data"), this);
    m_submitButton->setStyleSheet("QPushButton { background-color: #10B981; color: white; font-weight: bold; border-radius: 8px; padding: 10px; margin-top: 15px; }"
                                  "QPushButton:hover { background-color: #059669; }");
    mainLayout->addWidget(m_submitButton);

    mainLayout->addStretch();
}

// ADDED: Implementation of the workaround slot
void InitialFormWindow::forceUpdate()
{
    this->update();
}

void InitialFormWindow::submitForm()
{
    // Simple validation: check if any required field is empty
    if (m_gaWeeksInput->text().isEmpty() ||
        m_birthWeightInput->text().isEmpty() ||
        m_birthLengthInput->text().isEmpty() ||
        m_ageDaysInput->text().isEmpty() ||
        m_weightInput->text().isEmpty() ||
        m_lengthInput->text().isEmpty())
    {
        QMessageBox::warning(this, tr("Input Error"), tr("All fields must be filled out before submission."));
        return;
    }

    // Create and populate the PatientData struct
    BabyData data;
    bool ok = true;

    // String input
    data.gender = m_genderInput->currentText();

    // Float inputs (using toFloat with checks for robust conversion)
    data.gestational_age_weeks = m_gaWeeksInput->text().toFloat(&ok); if (!ok) { QMessageBox::critical(this, tr("Error"), tr("Invalid Gestational Age.")); return; }
    data.birth_weight_kg = m_birthWeightInput->text().toFloat(&ok); if (!ok) { QMessageBox::critical(this, tr("Error"), tr("Invalid Birth Weight.")); return; }
    data.birth_length_cm = m_birthLengthInput->text().toFloat(&ok); if (!ok) { QMessageBox::critical(this, tr("Error"), tr("Invalid Birth Length.")); return; }
    data.age_days = m_ageDaysInput->text().toFloat(&ok); if (!ok) { QMessageBox::critical(this, tr("Error"), tr("Invalid Age.")); return; }
    data.weight_kg = m_weightInput->text().toFloat(&ok); if (!ok) { QMessageBox::critical(this, tr("Error"), tr("Invalid Current Weight.")); return; }
    data.length_cm = m_lengthInput->text().toFloat(&ok); if (!ok) { QMessageBox::critical(this, tr("Error"), tr("Invalid Current Length.")); return; }

    // Emit the signal with the collected data
    emit dataSubmitted(data);
}
