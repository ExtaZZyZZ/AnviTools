#include <QApplication>
#include <QFont>
#include <QIcon>
#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    const QIcon appIcon(":/resources/app_icon.png");

    app.setApplicationName("AnviTools");
    app.setApplicationDisplayName("AnviTools");
    app.setOrganizationName("AnviTools");
    app.setWindowIcon(appIcon);
    app.setStyle("Fusion");
    app.setFont(QFont("Bahnschrift", 10));

    MainWindow w;
    w.setWindowIcon(appIcon);
    w.show();

    return app.exec();
}
