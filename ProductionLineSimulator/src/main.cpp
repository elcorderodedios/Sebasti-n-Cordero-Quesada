#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include "ui/MainWindow.h"

void setupApplicationDirectories() {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.prodline";
    QDir().mkpath(configDir);
    QDir().mkpath(configDir + "/logs");
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Production Line Simulator");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("IF-4001 Group 21");
    
    // Setup application directories
    setupApplicationDirectories();
    
    // Create and show main window
    MainWindow window;
    window.show();
    
    return app.exec();
}
