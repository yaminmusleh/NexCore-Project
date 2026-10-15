#include "MainWindow.h"

#include <QAction>
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
#include <QStatusBar>
#include <QTextCursor>
#include <QTextDocument>
#include <QToolBar>

#include "CodeEditor.h"
#include "NexHighlighter.h"

namespace {

QString sourceFilter()
{
    return QObject::tr("NexCore source (*.nex);;All files (*)");
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_editor = new CodeEditor(this);
    setCentralWidget(m_editor);

    // The highlighter is a child of the document and is deleted with it.
    new NexHighlighter(m_editor->document());

    createFileActions();
    createEditActions();
    createToolBar();
    createStatusBar();

    connect(m_editor->document(), &QTextDocument::modificationChanged,
            this, &QWidget::setWindowModified);
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged,
            this, &MainWindow::updateCursorStatus);
    connect(m_editor, &QPlainTextEdit::selectionChanged,
            this, &MainWindow::updateCursorStatus);

    setCurrentFile(QString());
    updateCursorStatus();

    resize(1280, 800);
    setMinimumSize(800, 500);
}

QAction *MainWindow::createAction(QMenu *menu, const QString &text,
                                  QKeySequence::StandardKey key)
{
    auto *action = new QAction(text, this);
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
    fileMenu->addSeparator();
    m_saveAct = createAction(fileMenu, tr("&Save"), QKeySequence::Save);
    QAction *saveAsAct = createAction(fileMenu, tr("Save &As..."), QKeySequence::SaveAs);
    fileMenu->addSeparator();
    QAction *quitAct = createAction(fileMenu, tr("&Quit"), QKeySequence::Quit);

    connect(m_newAct, &QAction::triggered, this, &MainWindow::newFile);
    connect(m_openAct, &QAction::triggered, this, &MainWindow::openFile);
    connect(m_saveAct, &QAction::triggered, this, &MainWindow::saveFile);
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::saveFileAs);
    // close() goes through closeEvent(), so unsaved changes are checked.
    connect(quitAct, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::createEditActions()
{
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));

    QAction *undoAct = createAction(editMenu, tr("&Undo"), QKeySequence::Undo);
    QAction *redoAct = createAction(editMenu, tr("&Redo"), QKeySequence::Redo);
    editMenu->addSeparator();
    QAction *cutAct = createAction(editMenu, tr("Cu&t"), QKeySequence::Cut);
    QAction *copyAct = createAction(editMenu, tr("&Copy"), QKeySequence::Copy);
    QAction *pasteAct = createAction(editMenu, tr("&Paste"), QKeySequence::Paste);
    editMenu->addSeparator();
    QAction *selectAllAct = createAction(editMenu, tr("Select &All"), QKeySequence::SelectAll);

    connect(undoAct, &QAction::triggered, m_editor, &QPlainTextEdit::undo);
    connect(redoAct, &QAction::triggered, m_editor, &QPlainTextEdit::redo);
    connect(cutAct, &QAction::triggered, m_editor, &QPlainTextEdit::cut);
    connect(copyAct, &QAction::triggered, m_editor, &QPlainTextEdit::copy);
    connect(pasteAct, &QAction::triggered, m_editor, &QPlainTextEdit::paste);
    connect(selectAllAct, &QAction::triggered, m_editor, &QPlainTextEdit::selectAll);

    undoAct->setEnabled(false);
    redoAct->setEnabled(false);
    cutAct->setEnabled(false);
    copyAct->setEnabled(false);
    connect(m_editor, &QPlainTextEdit::undoAvailable, undoAct, &QAction::setEnabled);
    connect(m_editor, &QPlainTextEdit::redoAvailable, redoAct, &QAction::setEnabled);
    connect(m_editor, &QPlainTextEdit::copyAvailable, cutAct, &QAction::setEnabled);
    connect(m_editor, &QPlainTextEdit::copyAvailable, copyAct, &QAction::setEnabled);
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}

void MainWindow::newFile()
{
    if (!maybeSave())
        return;

    m_editor->clear();
    setCurrentFile(QString());
}

void MainWindow::openFile()
{
    if (!maybeSave())
        return;

    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open File"), startDirectory(), sourceFilter());
    if (path.isEmpty())
        return;

    loadFile(path);
}

bool MainWindow::saveFile()
{
    if (m_currentFile.isEmpty())
        return saveFileAs();

    if (!writeFile(m_currentFile))
        return false;

    m_editor->document()->setModified(false);
    return true;
}

bool MainWindow::saveFileAs()
{
    QFileDialog dialog(this, tr("Save As"), startDirectory(), sourceFilter());
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix(QStringLiteral("nex"));
    // Overwrites are confirmed below, so the prompt is identical on every
    // platform dialog and also covers a name that gained the .nex suffix.
    dialog.setOption(QFileDialog::DontConfirmOverwrite, true);
    dialog.selectFile(m_currentFile.isEmpty()
                          ? QStringLiteral("untitled.nex")
                          : QFileInfo(m_currentFile).fileName());

    if (dialog.exec() != QDialog::Accepted || dialog.selectedFiles().isEmpty())
        return false;

    QString path = QFileInfo(dialog.selectedFiles().constFirst()).absoluteFilePath();
    if (QFileInfo(path).suffix().isEmpty())
        path += QStringLiteral(".nex");

    if (path != m_currentFile && QFileInfo::exists(path)) {
        const auto answer = QMessageBox::question(
            this, tr("Replace file?"),
            tr("\"%1\" already exists.\nDo you want to replace it?")
                .arg(QDir::toNativeSeparators(path)),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (answer != QMessageBox::Yes)
            return false;
    }

    if (!writeFile(path))
        return false;

    setCurrentFile(path);
    return true;
}

void MainWindow::updateCursorStatus()
{
    const QTextCursor cursor = m_editor->textCursor();
    QString text = tr("Ln %1, Col %2")
                       .arg(cursor.blockNumber() + 1)
                       .arg(cursor.positionInBlock() + 1);

    const int selected = cursor.selectionEnd() - cursor.selectionStart();
    if (selected > 0)
        text += tr("  (%1 selected)").arg(selected);

    m_cursorLabel->setText(text);
}

bool MainWindow::maybeSave()
{
    if (!m_editor->document()->isModified())
        return true;

    QMessageBox box(QMessageBox::Warning, tr("Unsaved changes"),
                    tr("Do you want to save the changes to \"%1\"?")
                        .arg(displayName()),
                    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                    this);
    box.setInformativeText(tr("Your changes will be lost if you don't save them."));
    box.setDefaultButton(QMessageBox::Save);

    switch (box.exec()) {
    case QMessageBox::Save:
        return saveFile();
    case QMessageBox::Discard:
        return true;
    default:
        return false;
    }
}

bool MainWindow::loadFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(
            this, tr("Cannot open file"),
            tr("Could not read \"%1\":\n%2")
                .arg(QDir::toNativeSeparators(path), file.errorString()));
        return false;
    }

    // The editor stores "\n" line breaks, so a file with Windows line endings
    // is saved back with "\n".
    QString text = QString::fromUtf8(file.readAll());
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));

    m_editor->setPlainText(text);
    setCurrentFile(path);
    return true;
}

bool MainWindow::writeFile(const QString &path)
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

    const QByteArray data = m_editor->toPlainText().toUtf8();
    if (file.write(data) != data.size() || !file.commit()) {
        QMessageBox::warning(
            this, tr("Cannot save file"),
            tr("Could not write \"%1\":\n%2")
                .arg(QDir::toNativeSeparators(path), file.errorString()));
        return false;
    }
    return true;
}

void MainWindow::setCurrentFile(const QString &path)
{
    m_currentFile = path.isEmpty() ? QString() : QFileInfo(path).absoluteFilePath();
    m_editor->document()->setModified(false);

    setWindowTitle(tr("%1[*] \u2014 NexCore Studio").arg(displayName()));
    setWindowModified(m_editor->document()->isModified());

    m_fileLabel->setText(m_currentFile.isEmpty()
                             ? tr("Untitled (not saved)")
                             : QDir::toNativeSeparators(m_currentFile));
    m_fileLabel->setToolTip(QDir::toNativeSeparators(m_currentFile));
}

QString MainWindow::startDirectory() const
{
    return m_currentFile.isEmpty() ? QDir::homePath()
                                   : QFileInfo(m_currentFile).absolutePath();
}

QString MainWindow::displayName() const
{
    return m_currentFile.isEmpty() ? tr("untitled.nex")
                                   : QFileInfo(m_currentFile).fileName();
}