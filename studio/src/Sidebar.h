#pragma once

#include <QList>
#include <QString>
#include <QWidget>

class QFileSystemModel;
class QLabel;
class QListWidget;
class QStackedWidget;
class QTreeView;

// Left panel: the list of open files and a folder explorer that shows
// .nex files.
class Sidebar : public QWidget
{
    Q_OBJECT

public:
    struct OpenFile {
        QString name;
        QString path; // empty for a buffer that was never saved
        bool modified = false;
    };

    explicit Sidebar(QWidget *parent = nullptr);

    // Mirrors the editor tabs, in tab order. `current` is the active tab.
    void setOpenFiles(const QList<OpenFile> &files, int current);

    void setRootFolder(const QString &path);
    QString rootFolder() const;

    // Selects `path` in the explorer if it lies inside the root folder.
    void revealFile(const QString &path);

signals:
    void openFileSelected(int index);
    void fileActivated(const QString &path);
    void openFolderRequested();

private:
    QListWidget *m_openList = nullptr;
    QLabel *m_explorerHeader = nullptr;
    QStackedWidget *m_explorerStack = nullptr;
    QFileSystemModel *m_model = nullptr;
    QTreeView *m_tree = nullptr;
    QString m_root;
};