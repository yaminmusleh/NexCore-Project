#pragma once

#include <QKeySequence>
#include <QMainWindow>
#include <QString>

class CodeEditor;
class CodeEditor;
class FindBar;
class QAction;
class QCloseEvent;
class QLabel;
class QMenu;
class QSplitter;
class QTabWidget;
class Sidebar;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    // Opens `path` in a tab, or switches to its tab if it is already open.
    bool openPath(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void openFiles();
    void openFolder();
    bool saveFile();
    bool saveFileAs();
    void closeCurrentTab();
    void closeTab(int index);
    void onCurrentTabChanged(int index);
    void updateCursorStatus();
    void refreshSidebar();

private:
    void createFileActions();
    void createEditActions();
    void createViewActions();
    void createToolBar();
    void createStatusBar();
    QAction *createAction(QMenu *menu, const QString &text,
                          const QKeySequence &key = QKeySequence());

    CodeEditor *currentEditor() const;
    CodeEditor *editorAt(int index) const;
    CodeEditor *editorForPath(const QString &path) const;
    CodeEditor *pristineEditor() const;
    CodeEditor *addEditor();

    void updateTab(CodeEditor *editor);
    void updateWindowState();
    void updateEditActions();

    bool maybeSave(CodeEditor *editor);
    bool saveEditor(CodeEditor *editor);
    bool saveEditorAs(CodeEditor *editor);
    bool writeFile(CodeEditor *editor, const QString &path);
    QString startDirectory(const CodeEditor *editor) const;

    QSplitter *m_splitter = nullptr;
    Sidebar *m_sidebar = nullptr;
    FindBar *m_findBar = nullptr;
    QTabWidget *m_tabs = nullptr;

    QAction *m_newAct = nullptr;
    QAction *m_openAct = nullptr;
    QAction *m_saveAct = nullptr;
    QAction *m_undoAct = nullptr;
    QAction *m_redoAct = nullptr;
    QAction *m_cutAct = nullptr;
    QAction *m_copyAct = nullptr;
    QAction *m_toggleSidebarAct = nullptr;

    QLabel *m_fileLabel = nullptr;
    QLabel *m_infoLabel = nullptr;
    QLabel *m_cursorLabel = nullptr;

    int m_untitledCounter = 0;
};