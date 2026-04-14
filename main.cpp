#ifdef ENABLE_GUI
#include <QApplication>
using Application = QApplication;
#include <QSystemTrayIcon>
#else
#include <QCoreApplication>
using Application = QCoreApplication;
#endif
#include <QCommandLineParser>
#include <QVersionNumber>
#include <QSharedPointer>
#include "httpfileserver.h"

int main(int argc, char *argv[])
{
#ifdef ENABLE_GUI
    Application::setQuitOnLastWindowClosed(false);
#endif

    Application app(argc, argv);
    QVersionNumber version(0, 0, 1);
    Application::setApplicationVersion(version.toString());
    QCommandLineParser parser;
    parser.setApplicationDescription("Http File Server");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption pathOption(QStringList() << "P" << "path",
        "Path to serve files from.", QString("./"));
    QCommandLineOption addressOption(QStringList() << "a" << "address",
        "Address to bind to.", QHostAddress(QHostAddress::Any).toString());
    QCommandLineOption portOption(QStringList() << "p" << "port",
        "Port to listen on.", QString::number(80));
    QCommandLineOption browserOption(QStringList() << "b" << "browser",
        "Open the root index in the default web browser.");
    QList<QCommandLineOption> options = {pathOption, addressOption, portOption, browserOption};
    parser.addOptions(options);
    parser.process(app);

    QString path = parser.value("path");
    QString address = parser.value("address");
    QString port = parser.value("port");
    qInfo() << parser.helpText().toStdString().c_str();

    HttpFileServer server;
    server.setRootDir(path.isEmpty() ? Application::applicationDirPath() : path);
    server.listen(address.isEmpty() ? QHostAddress::Any : QHostAddress(address),
                  port.isEmpty() ? 80 : static_cast<quint16>(port.toInt()));

    if(parser.isSet(browserOption))
    {
        server.openRootIndexInBrowser();
    }
    return app.exec();
}
