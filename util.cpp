#include "util.h"
#include <QFile>
#include <QDebug>
#include <QHttpServerResponse>
#include <QMimeDatabase>
#include <QDir>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>

#define B * 1
#define KB * 1024 B
#define MB * 1024 KB
#define GB * 1024 MB
#define TB * 1024 GB
#define PB * 1024 TB

class UtilPrivate
{
public:
    static QString ROOT_DIR;
};
QString UtilPrivate::ROOT_DIR = "";
Util::Util() {}

QString Util::readTemplateFile(const QString &filePath)
{
    if (auto *file = new QFile(filePath); !file->open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法打开模板文件";
        return QString();
    }
    else
    {
        QString html = file->readAll();
        file->close();
        return html;
    }
}

void Util::respondFile(QHttpServerResponder &responder, const QString &filePath)
{
    if (auto *file = new QFile(filePath); file->open(QIODevice::ReadOnly)) {

        QByteArray mime = QMimeDatabase().mimeTypeForFile(filePath).name().toUtf8();

        // 大于100M
        if(file->size() > 100 MB) {
            responder.write(file, mime);
        }
        else {
            responder.write(file->readAll(), mime);
        }
        file->close();

    }
    else
    {
        qWarning() << "无法打开文件" << filePath;
        responder.write(errorJson("Directory or file not found"), QHttpServerResponse::StatusCode::NotFound);
    }
}

QString Util::itemTemplate(const QString &icon, const QString &displayName, const QString &href, const QString &fileSize, const QString &lastModified)
{
    QString html = readTemplateFile(":/html/file-list-item-template.html");
    html.replace("${item-icon}$", icon);
    html.replace("${display-name}$", displayName);
    html.replace("${href}$", href);
    html.replace("${file-size}$", fileSize);
    html.replace("${last-modified}$", lastModified);
    return html;
}

bool Util::setRootDir(const QString &dir)
{
    if(dir != UtilPrivate::ROOT_DIR)
    {
        UtilPrivate::ROOT_DIR = dir;
        return true;
    }
    return false;
}

QString Util::rootDir()
{
    return UtilPrivate::ROOT_DIR;
}

QJsonDocument Util::errorJson(QString message)
{
    QJsonObject json;
    json.insert("message", message);
    return QJsonDocument(json);
}

Util::HtmlItemAccumulator::HtmlItemAccumulator(const QString &path)
    : pathStr(path)
{

}

QString Util::HtmlItemAccumulator::operator()(const QString &list, const QFileInfo &info) const {
    QString fileName = info.fileName();
    QString fileSizeStr;

    if(info.isDir()) {
        fileName += "/";
    } else {
        qreal fileSize = info.size();
        if (fileSize >= 1 GB)
            fileSizeStr = QString::number(fileSize / (1 GB)) + " GB";
        else if (fileSize >= 1 MB)
            fileSizeStr = QString::number(fileSize / (1 MB)) + " MB";
        else if (fileSize >= 1 KB)
            fileSizeStr = QString::number(fileSize / (1 KB)) + " KB";
        else
            fileSizeStr = QString::number(fileSize / (1 B)) + " B";
    }

    return list + Util::itemTemplate(info.isDir() ? "icon-folder-close" : "icon-file",
                                            fileName,
                                            pathStr + fileName,
                                            fileSizeStr,
                                            info.lastModified().toString("yyyy-MM-dd hh:mm:ss"));
}
