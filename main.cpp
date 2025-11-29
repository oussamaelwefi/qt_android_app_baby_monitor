#include <QApplication>
#include "guiwindow.h"
#include "bleclient.h"

int main(int argc, char *argv[])
{
    // QApplication is required for Qt Widgets applications
    QApplication a(argc, argv);

    // Instantiate the BLE client logic
    BleClient bleClient;

    InitialFormWindow *initialForm = new InitialFormWindow();
    QObject::connect(initialForm, &InitialFormWindow::dataSubmitted,
                     &a, [&bleClient, &initialForm](const BabyData& data) {

                         // This lambda executes when the form is submitted

                         // 1. Close and delete the form
                         initialForm->close();
                         initialForm->deleteLater();

                         // 2. Create the main window, passing the client and the form data
                         GuiWindow *mainWindow = new GuiWindow(&bleClient);

                         // Manually call the handler to set the initial data
                         mainWindow->handleFormData(data);

                         // 3. Show the main window for the first time
                         // NOTE: This call is redundant as mainWindow->handleFormData(data) now calls show()
                         // but leaving it doesn't harm anything.
                         // mainWindow->show();
                     });

    initialForm->show();
    return a.exec();
}
