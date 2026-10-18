#pragma once

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QWidget>

class QLabel;
class QVBoxLayout;

// Shown instead of the editor tabs while no file is open: the NexCore
// wordmark, a few ways to start, the recent files and a link to the website.
class WelcomePage : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomePage(QWidget *parent = nullptr);

    static QUrl websiteUrl();

    // The shortcut texts shown next to the three buttons (empty = none).
    void setShortcutHints(const QString &newFile, const QString &openFile,
                          const QString &openFolder);

    // Lists the first few of these as buttons; the section is hidden when empty.
    void setRecentFiles(const QStringList &paths);

signals:
    void newFileRequested();
    void openFileRequested();
    void openFolderRequested();
    void recentFileRequested(const QString &path);

private:
    QLabel *m_newHint = nullptr;
    QLabel *m_openHint = nullptr;
    QLabel *m_folderHint = nullptr;
    QWidget *m_recentSection = nullptr;
    QVBoxLayout *m_recentLayout = nullptr;
};