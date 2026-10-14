#pragma once

#include <QMainWindow>
#include <QString>

class QCloseEvent;
class QLabel;
class QPlainTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void openFile();
    bool saveFile();
    bool saveFileAs();
    void updateCursorStatus();

private:
    void createActions();
    void createStatusBar();

    bool maybeSave();
    bool loadFile(const QString &path);
    bool writeFile(const QString &path);
    void setCurrentFile(const QString &path);
    QString startDirectory() const;
    QString displayName() const;

    QPlainTextEdit *m_editor = nullptr;
    QLabel *m_fileLabel = nullptr;
    QLabel *m_cursorLabel = nullptr;

    // Absolute path of the open file; empty while the buffer is untitled.
    QString m_currentFile;
};