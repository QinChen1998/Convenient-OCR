#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 设置应用程序信息，用于QSettings存储位置
    a.setOrganizationName("ConvenientOCRApp");
    a.setOrganizationDomain("ocrapp.local");
    a.setApplicationName("ConvenientOCRApplication");
    a.setApplicationVersion("1.0");

    MainWindow w;
    w.show();
    return a.exec();
}
