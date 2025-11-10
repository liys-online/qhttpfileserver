#include "httpfileserver.h"
#include "filerouter.h"
#include "router.h"
#include <QHttpServer>
#include <QTcpServer>
#include <QDir>
#include <QCoreApplication>
#include <QMimeDatabase>
#ifdef ENABLE_GUI
#include <QDesktopServices>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QFileDialog>
#else
#include <QProcess>
#endif
#include "util.h"
#include "staticresourcerouter.h"
class HttpFileServerPrivate : public QObject
{
public:

    explicit HttpFileServerPrivate(HttpFileServer *parent)
        :q_ptr{parent}
    {

    }

    void activateTrayIcon(QSystemTrayIcon::ActivationReason reason)
    {
        switch (reason) {
        case QSystemTrayIcon::Unknown:
            break;
        case QSystemTrayIcon::Context:
            break;
        case QSystemTrayIcon::DoubleClick:
            break;
        case QSystemTrayIcon::Trigger:
            if (isListening) q_ptr->openRootIndexInBrowser(); break;
        case QSystemTrayIcon::MiddleClick:
            break;
        }
    }
#ifdef ENABLE_GUI
    bool showTrayIcon()
    {
        if (QSystemTrayIcon::isSystemTrayAvailable() && !trayIcon) {
            trayIcon.reset(new QSystemTrayIcon(QIcon(APP_ICON), qApp));
            QSystemTrayIcon::connect(trayIcon.data(), &QSystemTrayIcon::activated,
                                     this ,&HttpFileServerPrivate::activateTrayIcon);
            trayIcon->setToolTip("Http File Server");
            trayIcon->setContextMenu(new QMenu);
            trayIcon->contextMenu()->addAction("Change Directory", this, &HttpFileServerPrivate::changeRootDir);
            trayIcon->contextMenu()->addAction("Exit", qApp, &QCoreApplication::quit);
            trayIcon->show();
        }
        if(isListening)
        {
            trayIcon->showMessage("提示", "Http File Server 正在运行...", QIcon(APP_ICON), 1000);
        }
        else
        {
            trayIcon->showMessage("提示", "Http File Server 停止运行", QIcon(APP_ICON), 1000);
            QThread::msleep(500);
        }
        return trayIcon != nullptr;
    }
#endif
    void changeRootDir()
    {
        QString directory = QFileDialog::getExistingDirectory(nullptr, "选择文件目录", q_ptr->rootDir());
        if (!directory.isEmpty()) {
            q_ptr->setRootDir(directory);
            qDebug() << "选择的目录:" << directory;
        }
    }
    HttpFileServerPrivate(const HttpFileServerPrivate &) = delete;
    HttpFileServerPrivate& operator=(const HttpFileServerPrivate &) = delete;

    ~HttpFileServerPrivate()
    {
        QStringList keys = routerMap.keys();
        for (const QString &key : std::as_const(keys))
        {
            if (routerMap[key])
            {
                routerMap[key]->deleteLater();
            }
        }
        routerMap.clear();
    }

    QScopedPointer<QHttpServer> server;
#ifdef ENABLE_GUI
    QScopedPointer<QSystemTrayIcon> trayIcon;
#endif
    RouterMap routerMap;
    QHostAddress hostAddress = QHostAddress::Any;
    quint16 port = 80;
    bool isListening = false;
private:
    HttpFileServer *q_ptr = nullptr;
};
HttpFileServer::HttpFileServer(QObject *parent)
    : QObject{parent},
    d_ptr{new HttpFileServerPrivate(this)}
{

    setRootDir(QCoreApplication::applicationDirPath());
}

HttpFileServer::~HttpFileServer()
{
    close();
}

QString HttpFileServer::rootDir() const
{
    return Util::rootDir();
}

void HttpFileServer::setRootDir(const QString &newRootDir)
{
    Util::setRootDir(newRootDir) ? emit rootDirChanged(newRootDir) : void();
}

bool HttpFileServer::listen(const QHostAddress &address, quint16 port)
{
    qInfo() << "监听地址:" << address.toString() << "端口:" << port;
    if (isListening())
    {
        qWarning() << "服务器已经在监听中";
        return isListening();
    }
    d_ptr->hostAddress = address;
    d_ptr->port = port;
    auto *tcpServer = new QTcpServer(this);
    if (d_ptr->isListening = tcpServer->listen(d_ptr->hostAddress, d_ptr->port) ; !isListening()) {

        qCritical() << "监听失败：" << tcpServer->errorString();
        tcpServer->close();
        tcpServer->deleteLater();
        return isListening();
    }

    d_ptr->server.reset(new QHttpServer(this));
    if (d_ptr->isListening = d_ptr->server->bind(tcpServer) ; !isListening()) {
        close();
        qCritical() << "QHttpServer 绑定失败";
        return isListening();
    }
    addRouter(QSharedPointer<StaticResourceRouter>::create());
    addRouter(QSharedPointer<FileRouter>::create());

    auto host = d_ptr->hostAddress.toString();
    if (d_ptr->hostAddress == QHostAddress::Any) {
        host = QHostAddress(QHostAddress::LocalHost).toString();
    }

    qInfo() << "目录浏览服务器已启动, 文件目录:" << Util::rootDir();
    qInfo() << QString("浏览：http://%1:%2/").arg(host, QString::number(d_ptr->port));
#ifdef ENABLE_GUI
    d_ptr->showTrayIcon();
#endif
    return isListening();
}

bool HttpFileServer::isListening() const
{
    return d_ptr->isListening;
}

void HttpFileServer::close()
{
    if (d_ptr->server)
    {
        auto servers = d_ptr->server->servers();
        for(auto *server : std::as_const(servers))
        {
            server->close();
            server->deleteLater();
        }
        d_ptr->isListening = false;
    }
#ifdef ENABLE_GUI
    d_ptr->showTrayIcon();
#endif
    d_ptr->server.reset();
}

void HttpFileServer::openRootIndexInBrowser()
{
    if (isListening())
    {
        QHostAddress address = d_ptr->hostAddress == QHostAddress::Any ? QHostAddress::LocalHost : d_ptr->hostAddress;
        QString addressStr = address.toString();
        QString portStr = QString::number(d_ptr->port);
#ifdef ENABLE_GUI
        QDesktopServices::openUrl(QString("http://%1:%2").arg(addressStr, portStr));
#elif defined(Q_OS_WIN)
        QProcess::startDetached("cmd", {"/c", "start", QString("http://%1:%2").arg(addressStr, portStr)});
#elif defined(Q_OS_MAC)
        QProcess::startDetached("open", {QString("http://%1:%2").arg(addressStr, portStr)});
#else
        QProcess::startDetached("xdg-open", {QString("http://%1:%2").arg(addressStr, portStr)});
#endif
    }
}

QHostAddress HttpFileServer::hostAddress() const
{
    return d_ptr->hostAddress;
}

quint16 HttpFileServer::port() const
{
    return d_ptr->port;
}

void HttpFileServer::addRouter(const QSharedPointer<Router> &router)
{
    if (!d_ptr->server) return;
    if (!router) return;

    d_ptr->server->route(router->pathPattern(), router->requestHandler());
    d_ptr->routerMap.insert(router->pathPattern(), router);
}
