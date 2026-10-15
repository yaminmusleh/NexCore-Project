#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSaveFile>
#include <QSizePolicy>
#include <QSplitter>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QTextCursor>
#include <QTextDocument>
#include <QToolBar>
#include <QToolButton>

#include "CodeEditor.h"
#include "NexHighlighter.h"
#include "Sidebar.h"

namespace {

QString sourceFilter()
{
    return QObject::tr("NexCore source (*.nex);;All files (*)");
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_tabs = new QTabWidget(this);
    m_tabs->setDocumentMode(true);
    m_tabs->setMovable(true);
    m_tabs->setElideMode(Qt::ElideRight);
    m_tabs->tabBar()->setDrawBase(false);

    m_sidebar = new Sidebar(this);
    m_sidebar->setMinimumWidth(180);
    m_sidebar->setMaximumWidth(480);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setHandleWidth(1);
    m_splitter->addWidget(m_sidebar);
    m_splitter->addWidget(m_tabs);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({260, 1020});
    setCentralWidget(m_splitter);

    createFileActions();
    createEditActions();
    createViewActions();
    createToolBar();
    createStatusBar();

    connect(m_tabs, &QTabWidget::currentChanged, this, &MainWindow::onCurrentTabChanged);
    connect(m_tabs->tabBar(), &QTabBar::tabMoved, this, [this] { refreshSidebar(); });

    connect(m_sidebar, &Sidebar::openFileSelected, m_tabs, &QTabWidget::setCurrentIndex);
    connect(m_sidebar, &Sidebar::fileActivated, this, [this](const QString &path) {
        openPath(path);
    });
    connect(m_sidebar, &Sidebar::openFolderRequested, this, &MainWindow::openFolder);

    addEditor();

    resize(1280, 800);
    setMinimumSize(800, 500);
}

QAction *MainWindow::createAction(QMenu *menu, const QString &text, const QKeySequence &key)
{
    auto *action = new QAction(text, this);
    if (!key.isEmpty())
        action->setShortcut(key);
    menu->addAction(action);
    return action;
}

void MainWindow::createFileActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    m_newAct = createAction(fileMenu, tr("&New"), QKeySequence::New);
    m_openAct = createAction(fileMenu, tr("&Open..."), QKeySequence::Open);
    m_openAct->setIconText(tr("Open"));
    QAction *openFolderAct = createAction(fileMenu, tr("Open &Folder..."),
                                          QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
    fileMenu->addSeparator();
    m_saveAct = createAction(fileMenu, tr("&Save"), QKeySequence::Save);
    QAction *saveAsAct = createAction(fileMenu, tr("Save &As..."), QKeySequence::SaveAs);
    fileMenu->addSeparator();
    QAction *closeTabAct = createAction(fileMenu, tr("&Close Tab"), QKeySequence::Close);
    QAction *quitAct = createAction(fileMenu, tr("&Quit"), QKeySequence::Quit);

    connect(m_newAct, &QAction::triggered, this, &MainWindow::newFile);
    connect(m_openAct, &QAction::triggered, this, &MainWindow::openFiles);
    connect(openFolderAct, &QAction::triggered, this, &MainWindow::openFolder);
    connect(m_saveAct, &QAction::triggered, this, &MainWindow::saveFile);
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::saveFileAs);
    connect(closeTabAct, &QAction::triggered, this, &MainWindow::closeCurrentTab);
    // close() goes through closeEvent(), which asks about every unsaved tab.
    connect(quitAct, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::createEditActions()
{
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));

    m_undoAct = createAction(editMenu, tr("&Undo"), QKeySequence::Undo);
    m_redoAct = createAction(editMenu, tr("&Redo"), QKeySequence::Redo);
    editMenu->addSeparator();
    m_cutAct = createAction(editMenu, tr("Cu&t"), QKeySequence::Cut);
    m_copyAct = createAction(editMenu, tr("&Copy"), QKeySequence::Copy);
    QAction *pasteAct = createAction(editMenu, tr("&Paste"), QKeySequence::Paste);
    editMenu->addSeparator();
    QAction *selectAllAct = createAction(editMenu, tr("Select &All"), QKeySequence::SelectAll);

    // The actions always work on whichever editor tab is current.
    connect(m_undoAct, &QAction::triggered, this, [this] {
        if (CodeEditor *editor = currentEditor())
            editor->undo();
    });
    connect(m_redoAct, &QAction::triggered, this, [this] {
        if (CodeEditor *editor = currentEditor())
            editor->redo();
    });
    connect(m_cutAct, &QAction::triggered, this, [this] {
        if (CodeEditor *editor = currentEditor())
            editor->cut();
    });
    connect(m_copyAct, &QAction::triggered, this, [this] {
        if (CodeEditor *editor = currentEditor())
            editor->copy();
    });
    connect(pasteAct, &QAction::triggered, this, [this] {
        if (CodeEditor *editor = currentEditor())
            editor->paste();
    });
    connect(selectAllAct, &QAction::triggered, this, [this] {
        if (CodeEditor *editor = currentEditor())
            editor->selectAll();
    });

    updateEditActions();
}

void MainWindow::createViewActions()
{
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    m_toggleSidebarAct = createAction(viewMenu, tr("Show &Sidebar"),
                                      QKeySequence(Qt::CTRL | Qt::Key_B));
    m_toggleSidebarAct->setCheckable(true);
    m_toggleSidebarAct->setChecked(true);
    connect(m_toggleSidebarAct, &QAction::toggled, m_sidebar, &QWidget::setVisible);

    viewMenu->addSeparator();

    QAction *nextTabAct = createAction(viewMenu, tr("&Next Tab"));
    nextTabAct->setShortcuts({QKeySequence(QKeySequence::NextChild),
                              QKeySequence(Qt::CTRL | Qt::Key_PageDown)});
    QAction *previousTabAct = createAction(viewMenu, tr("&Previous Tab"));
    previousTabAct->setShortcuts({QKeySequence(QKeySequence::PreviousChild),
                                  QKeySequence(Qt::CTRL | Qt::Key_PageUp)});

    connect(nextTabAct, &QAction::triggered, this, [this] {
        if (m_tabs->count() > 1)
            m_tabs->setCurrentIndex((m_tabs->currentIndex() + 1) % m_tabs->count());
    });
    connect(previousTabAct, &QAction::triggered, this, [this] {
        if (m_tabs->count() > 1)
            m_tabs->setCurrentIndex((m_tabs->currentIndex() + m_tabs->count() - 1) % m_tabs->count());
    });
}

void MainWindow::createToolBar()
{
    QToolBar *toolBar = addToolBar(tr("Main"));
    toolBar->setMovable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    toolBar->addAction(m_newAct);
    toolBar->addAction(m_openAct);
    toolBar->addAction(m_saveAct);
}

void MainWindow::createStatusBar()
{
    m_fileLabel = new QLabel(this);
    // Ignored lets a long path shrink instead of widening the window.
    m_fileLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    m_infoLabel = new QLabel(tr("UTF-8  \u00b7  LF  \u00b7  Spaces: %1").arg(CodeEditor::kIndentWidth), this);
    m_infoLabel->setContentsMargins(8, 0, 8, 0);

    m_cursorLabel = new QLabel(this);
    m_cursorLabel->setContentsMargins(8, 0, 8, 0);

    statusBar()->addWidget(m_fileLabel, 1);
    statusBar()->addPermanentWidget(m_infoLabel);
    statusBar()->addPermanentWidget(m_cursorLabel);
}

// ---- tabs -----------------------------------------------------------------

CodeEditor *MainWindow::currentEditor() const
{
    return qobject_cast<CodeEditor *>(m_tabs->currentWidget());
}

CodeEditor *MainWindow::editorAt(int index) const
{
    return qobject_cast<CodeEditor *>(m_tabs->widget(index));
}

CodeEditor *MainWindow::editorForPath(const QString &path) const
{
    const QString wanted = QFileInfo(path).canonicalFilePath();
    if (wanted.isEmpty())
        return nullptr;

    for (int i = 0; i < m_tabs->count(); ++i) {
        CodeEditor *editor = editorAt(i);
        if (editor && !editor->filePath().isEmpty()
            && QFileInfo(editor->filePath()).canonicalFilePath() == wanted) {
            return editor;
        }
    }
    return nullptr;
}

// The single empty, untouched "untitled" tab that exists right after start-up.
// Opening a file replaces it instead of leaving it behind.
CodeEditor *MainWindow::pristineEditor() const
{
    if (m_tabs->count() != 1)
        return nullptr;

    CodeEditor *editor = editorAt(0);
    if (editor && editor->filePath().isEmpty() && !editor->document()->isModified()
        && editor->document()->isEmpty()) {
        return editor;
    }
    return nullptr;
}

CodeEditor *MainWindow::addEditor()
{
    auto *editor = new CodeEditor;
    // The highlighter is a child of the document and is deleted with it.
    new NexHighlighter(editor->document());

    editor->setUntitledName(m_untitledCounter == 0
                                ? tr("untitled.nex")
                                : tr("untitled-%1.nex").arg(m_untitledCounter + 1));
    ++m_untitledCounter;

    connect(editor->document(), &QTextDocument::modificationChanged, this,
            [this, editor] { updateTab(editor); });

    auto cursorChanged = [this, editor] {
        if (editor == currentEditor())
            updateCursorStatus();
    };
    connect(editor, &QPlainTextEdit::cursorPositionChanged, this, cursorChanged);
    connect(editor, &QPlainTextEdit::selectionChanged, this, cursorChanged);

    auto actionsChanged = [this, editor] {
        if (editor == currentEditor())
            updateEditActions();
    };
    connect(editor, &QPlainTextEdit::undoAvailable, this, actionsChanged);
    connect(editor, &QPlainTextEdit::redoAvailable, this, actionsChanged);
    connect(editor, &QPlainTextEdit::copyAvailable, this, actionsChanged);

    const int index = m_tabs->addTab(editor, editor->displayName());

    // A plain button instead of the style's close icon, which looks wrong on
    // the dark theme. The index is looked up on click because tabs can move.
    auto *closeButton = new QToolButton;
    closeButton->setObjectName(QStringLiteral("tabClose"));
    closeButton->setText(QString::fromUtf8("\u00d7"));
    closeButton->setToolTip(tr("Close"));
    closeButton->setFocusPolicy(Qt::NoFocus);
    connect(closeButton, &QToolButton::clicked, this, [this, editor] {
        closeTab(m_tabs->indexOf(editor));
    });
    m_tabs->tabBar()->setTabButton(index, QTabBar::RightSide, closeButton);

    m_tabs->setCurrentIndex(index);
    updateTab(editor);
    editor->setFocus();
    return editor;
}

void MainWindow::updateTab(CodeEditor *editor)
{
    const int index = m_tabs->indexOf(editor);
    if (index < 0)
        return;

    QString name = editor->displayName();
    name.replace(QLatin1Char('&'), QStringLiteral("&&")); // '&' would become a mnemonic
    m_tabs->setTabText(index, editor->document()->isModified()
                                  ? tr("%1  \u25cf").arg(name)
                                  : name);
    m_tabs->setTabToolTip(index, editor->filePath().isEmpty()
                                     ? tr("Not saved yet")
                                     : QDir::toNativeSeparators(editor->filePath()));

    if (editor == currentEditor())
        updateWindowState();
    refreshSidebar();
}

void MainWindow::updateWindowState()
{
    CodeEditor *editor = currentEditor();

    if (!editor) {
        setWindowTitle(tr("NexCore Studio"));
        setWindowModified(false);
        m_fileLabel->clear();
        m_fileLabel->setToolTip(QString());
        return;
    }

    setWindowTitle(tr("%1[*] \u2014 NexCore Studio").arg(editor->displayName()));
    setWindowModified(editor->document()->isModified());

    const QString path = QDir::toNativeSeparators(editor->filePath());
    m_fileLabel->setText(editor->filePath().isEmpty() ? tr("Untitled (not saved)") : path);
    m_fileLabel->setToolTip(path);
}

void MainWindow::updateEditActions()
{
    CodeEditor *editor = currentEditor();
    const bool hasSelection = editor && editor->textCursor().hasSelection();

    m_undoAct->setEnabled(editor && editor->document()->isUndoAvailable());
    m_redoAct->setEnabled(editor && editor->document()->isRedoAvailable());
    m_cutAct->setEnabled(hasSelection);
    m_copyAct->setEnabled(hasSelection);
}

void MainWindow::updateCursorStatus()
{
    CodeEditor *editor = currentEditor();
    if (!editor) {
        m_cursorLabel->clear();
        return;
    }

    const QTextCursor cursor = editor->textCursor();
    QString text = tr("Ln %1, Col %2")
                       .arg(cursor.blockNumber() + 1)
                       .arg(cursor.positionInBlock() + 1);

    const int selected = cursor.selectionEnd() - cursor.selectionStart();
    if (selected > 0)
        text += tr("  (%1 selected)").arg(selected);

    m_cursorLabel->setText(text);
}

void MainWindow::refreshSidebar()
{
    QList<Sidebar::OpenFile> files;
    for (int i = 0; i < m_tabs->count(); ++i) {
        if (CodeEditor *editor = editorAt(i)) {
            files.append({editor->displayName(), editor->filePath(),
                          editor->document()->isModified()});
        }
    }
    m_sidebar->setOpenFiles(files, m_tabs->currentIndex());
}

void MainWindow::onCurrentTabChanged(int index)
{
    CodeEditor *editor = editorAt(index);

    updateWindowState();
    updateCursorStatus();
    updateEditActions();
    refreshSidebar();

    if (!editor)
        return;

    m_sidebar->revealFile(editor->filePath());

    // Keep keyboard focus in the sidebar while it is being used.
    QWidget *focused = QApplication::focusWidget();
    if (!(focused && m_sidebar->isAncestorOf(focused)))
        editor->setFocus();
}

// ---- file actions -----------------------------------------------------------

void MainWindow::newFile()
{
    addEditor();
}

void MainWindow::openFiles()
{
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, tr("Open Files"), startDirectory(currentEditor()), sourceFilter());

    for (const QString &path : paths)
        openPath(path);
}

void MainWindow::openFolder()
{
    const QString folder = QFileDialog::getExistingDirectory(
        this, tr("Open Folder"), startDirectory(currentEditor()));
    if (folder.isEmpty())
        return;

    m_sidebar->setRootFolder(folder);
    if (CodeEditor *editor = currentEditor())
        m_sidebar->revealFile(editor->filePath());
}

bool MainWindow::openPath(const QString &path)
{
    const QString absolute = QFileInfo(path).absoluteFilePath();

    if (CodeEditor *open = editorForPath(absolute)) {
        m_tabs->setCurrentWidget(open);
        return true;
    }

    QFile file(absolute);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(
            this, tr("Cannot open file"),
            tr("Could not read \"%1\":\n%2")
                .arg(QDir::toNativeSeparators(absolute), file.errorString()));
        return false;
    }

    // The editor stores "\n" line breaks, so a file with Windows line endings
    // is saved back with "\n".
    QString text = QString::fromUtf8(file.readAll());
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));

    CodeEditor *editor = pristineEditor();
    if (!editor)
        editor = addEditor();

    editor->setPlainText(text);
    editor->setFilePath(absolute);
    editor->document()->setModified(false);
    m_tabs->setCurrentWidget(editor);
    updateTab(editor);
    editor->setFocus();

    if (m_sidebar->rootFolder().isEmpty())
        m_sidebar->setRootFolder(QFileInfo(absolute).absolutePath());
    m_sidebar->revealFile(absolute);
    return true;
}

bool MainWindow::saveFile()
{
    CodeEditor *editor = currentEditor();
    return editor && saveEditor(editor);
}

bool MainWindow::saveFileAs()
{
    CodeEditor *editor = currentEditor();
    return editor && saveEditorAs(editor);
}

bool MainWindow::saveEditor(CodeEditor *editor)
{
    if (editor->filePath().isEmpty())
        return saveEditorAs(editor);

    if (!writeFile(editor, editor->filePath()))
        return false;

    editor->document()->setModified(false);
    return true;
}

bool MainWindow::saveEditorAs(CodeEditor *editor)
{
    QFileDialog dialog(this, tr("Save As"), startDirectory(editor), sourceFilter());
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix(QStringLiteral("nex"));
    // Overwrites are confirmed below, so the prompt is identical on every
    // platform dialog and also covers a name that gained the .nex suffix.
    dialog.setOption(QFileDialog::DontConfirmOverwrite, true);
    dialog.selectFile(editor->displayName());

    if (dialog.exec() != QDialog::Accepted || dialog.selectedFiles().isEmpty())
        return false;

    QString path = QFileInfo(dialog.selectedFiles().constFirst()).absoluteFilePath();
    if (QFileInfo(path).suffix().isEmpty())
        path += QStringLiteral(".nex");

    CodeEditor *other = editorForPath(path);
    if (other && other != editor) {
        QMessageBox::warning(
            this, tr("File is already open"),
            tr("\"%1\" is open in another tab.\nClose that tab first, or choose a different name.")
                .arg(QDir::toNativeSeparators(path)));
        return false;
    }

    if (path != editor->filePath() && QFileInfo::exists(path)) {
        const auto answer = QMessageBox::question(
            this, tr("Replace file?"),
            tr("\"%1\" already exists.\nDo you want to replace it?")
                .arg(QDir::toNativeSeparators(path)),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (answer != QMessageBox::Yes)
            return false;
    }

    if (!writeFile(editor, path))
        return false;

    editor->setFilePath(path);
    editor->document()->setModified(false);
    updateTab(editor);

    if (m_sidebar->rootFolder().isEmpty())
        m_sidebar->setRootFolder(QFileInfo(path).absolutePath());
    m_sidebar->revealFile(path);
    return true;
}

bool MainWindow::writeFile(CodeEditor *editor, const QString &path)
{
    // QSaveFile writes to a temporary file and renames it on commit, so a
    // failed save never leaves a half-written file behind.
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(
            this, tr("Cannot save file"),
            tr("Could not write \"%1\":\n%2")
                .arg(QDir::toNativeSeparators(path), file.errorString()));
        return false;
    }

    const QByteArray data = editor->toPlainText().toUtf8();
    if (file.write(data) != data.size() || !file.commit()) {
        QMessageBox::warning(
            this, tr("Cannot save file"),
            tr("Could not write \"%1\":\n%2")
                .arg(QDir::toNativeSeparators(path), file.errorString()));
        return false;
    }
    return true;
}

bool MainWindow::maybeSave(CodeEditor *editor)
{
    if (!editor->document()->isModified())
        return true;

    // Show the tab the question is about.
    m_tabs->setCurrentWidget(editor);

    QMessageBox box(QMessageBox::Warning, tr("Unsaved changes"),
                    tr("Do you want to save the changes to \"%1\"?")
                        .arg(editor->displayName()),
                    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                    this);
    box.setInformativeText(tr("Your changes will be lost if you don't save them."));
    box.setDefaultButton(QMessageBox::Save);

    switch (box.exec()) {
    case QMessageBox::Save:
        return saveEditor(editor);
    case QMessageBox::Discard:
        return true;
    default:
        return false;
    }
}

void MainWindow::closeCurrentTab()
{
    closeTab(m_tabs->currentIndex());
}

void MainWindow::closeTab(int index)
{
    CodeEditor *editor = editorAt(index);
    if (!editor || !maybeSave(editor))
        return;

    m_tabs->removeTab(index);
    editor->deleteLater();

    // There is always at least one tab to type into.
    if (m_tabs->count() == 0)
        addEditor();

    refreshSidebar();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    for (int i = 0; i < m_tabs->count(); ++i) {
        CodeEditor *editor = editorAt(i);
        if (editor && !maybeSave(editor)) {
            event->ignore();
            return;
        }
    }
    event->accept();
}

QString MainWindow::startDirectory(const CodeEditor *editor) const
{
    if (editor && !editor->filePath().isEmpty())
        return QFileInfo(editor->filePath()).absolutePath();
    if (!m_sidebar->rootFolder().isEmpty())
        return m_sidebar->rootFolder();
    return QDir::homePath();
}