#include "todoparser.h"
#include "mainwindow.h"
#include "editor.h"
#include "editorlist.h"
#include "HighlighterManager.h"
#include "qsynedit/Constants.h"

TodoParser::TodoParser(QObject *parent) : QObject(parent)
{
    mThread = nullptr;
}

void TodoParser::parseFile(const QString &filename)
{
    QMutexLocker locker(&mMutex);
    if (mThread) {
        return;
    }
    mThread = new TodoThread(filename);
    connect(mThread,&QThread::finished,
            [this] {
        QMutexLocker locker(&mMutex);
        if (mThread) {
            mThread->deleteLater();
            mThread = nullptr;
        }
    });
    connect(mThread, &TodoThread::parseStarted,
            pMainWindow, &MainWindow::onTodoParseStarted);
    connect(mThread, &TodoThread::todoFound,
            pMainWindow, &MainWindow::onTodoParsing);
    connect(mThread, &TodoThread::parseFinished,
            pMainWindow, &MainWindow::onTodoParseFinished);
    mThread->start();
}

bool TodoParser::parsing() const
{
    return (mThread!=nullptr);
}

TodoThread::TodoThread(const QString& filename, QObject *parent): QThread(parent)
{
    mFilename = filename;
}

void TodoThread::run()
{
    PSynHighlighter highlighter = highlighterManager.getCppHighlighter();
    emit parseStarted();
    auto action = finally([this]{
        emit parseFinished();
    });
    QStringList lines;
    if (!pMainWindow->editorList()->getContentFromOpenedEditor(mFilename,lines)) {
        return;
    }
    PSynHighlighterAttribute commentAttr = highlighter->getAttribute(SYNS_AttrComment);

    highlighter->resetState();
    for (int i =0;i<lines.count();i++) {
        highlighter->setLine(lines[i],i);
        while (!highlighter->eol()) {
            PSynHighlighterAttribute attr;
            attr = highlighter->getTokenAttribute();
            if (attr == commentAttr) {
                QString token = highlighter->getToken();
                int pos = token.indexOf("TODO:",Qt::CaseInsensitive);
                if (pos>=0) {
                    emit todoFound(
                                mFilename,
                                i+1,
                                pos+highlighter->getTokenPos(),
                                lines[i].trimmed()
                                );
                }
            }
            highlighter->next();
        }
    }
}

TodoModel::TodoModel(QObject *parent) : QAbstractListModel(parent)
{

}

void TodoModel::addItem(const QString &filename, int lineNo, int ch, const QString &line)
{
    beginInsertRows(QModelIndex(),mItems.count(),mItems.count());
    PTodoItem item = std::make_shared<TodoItem>();
    item->filename = filename;
    item->lineNo = lineNo;
    item->ch = ch;
    item->line = line;
    mItems.append(item);
    endInsertRows();
}

void TodoModel::clear()
{
    beginResetModel();
    mItems.clear();
    endResetModel();
}

PTodoItem TodoModel::getItem(const QModelIndex &index)
{
    if (!index.isValid())
        return PTodoItem();
    return mItems[index.row()];
}

int TodoModel::rowCount(const QModelIndex &parent) const
{
    return mItems.count();
}

QVariant TodoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role==Qt::DisplayRole) {
        PTodoItem item = mItems[index.row()];
        switch(index.column()) {
        case 0:
            return item->filename;
        case 1:
            return item->lineNo;
        case 2:
            return item->ch;
        case 3:
            return item->line;
        }
    }
    return QVariant();
}

QVariant TodoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section) {
        case 0:
            return tr("Filename");
        case 1:
            return tr("Line");
        case 2:
            return tr("Column");
        case 3:
            return tr("Content");
        }
    }
    return QVariant();
}

int TodoModel::columnCount(const QModelIndex &parent) const
{
    return 4;
}
