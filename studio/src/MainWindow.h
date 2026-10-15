#pragma once

#include <QKeySequence>
#include <QMainWindow>
#include <QString>

class CodeEditor;
class QAction;
class QCloseEvent;
class QLabel;
class QMenu;

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
    void createFileActions();
    void createEditActions();
    void createToolBar();
    void createStatusBar();
    QAction *createAction(QMenu *menu, const QString &text,
                          QKeySequence::StandardKey key);

    bool maybeSave();
    bool loadFile(const QString &path);
    bool writeFile(const QString &path);
    void setCurrentFile(const QString &path);
    QString startDirectory() const;
    QString displayName() const;

    CodeEditor *m_editor = nullptr;
    QAction *m_newAct = nullptr;
    QAction *m_openAct = nullptr;
    QAction *m_saveAct = nullptr;
    QLabel *m_fileLabel = nullptr;
    QLabel *m_infoLabel = nullptr;
    QLabel *m_cursorLabel = nullptr;

    // Absolute path of the open file; empty while the buffer is untitled.
    QString m_currentFile;
};