#include "MainWindow.h"

#include <QAction>
#include <QCloseEvent>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
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

namespace {

QString sourceFilter()
{
    return QObject::tr("NexCore source (*.nex);;All files (*)");
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_editor = new QPlainTextEdit(this);
    m_editor->setFrameShape(QFrame::NoFrame);
    m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);

    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(11);
    m_editor->setFont(font);
    m_editor->setTabStopDistance(
        4 * m_editor->fontMetrics().horizontalAdvance(QLatin1Char(' ')));

    setCentralWidget(m_editor);

    createActions();
    createStatusBar();

    connect(m_editor->document(), &QTextDocument::modificationChanged,
            this, &QWidget::setWindowModified);
    connect(m_editor, &QPlainTextEdit::cursorPositionChanged,
            this, &MainWindow::updateCursorStatus);

    setCurrentFile(QString());
    updateCursorStatus();

    resize(1280, 800);
    setMinimumSize(800, 500);
}

void MainWindow::createActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    auto add = [this, fileMenu](const QString &text,
                                QKeySequence::StandardKey key, auto slot) {
        auto *action = new QAction(text, this);
        action->setShortcut(key);
        connect(action, &QAction::triggered, this, slot);
        fileMenu->addAction(action);
    };

    add(tr("&New"), QKeySequence::New, &MainWindow::newFile);
    add(tr("&Open..."), QKeySequence::Open, &MainWindow::openFile);
    fileMenu->addSeparator();
    add(tr("&Save"), QKeySequence::Save, &MainWindow::saveFile);
    add(tr("Save &As..."), QKeySequence::SaveAs, &MainWindow::saveFileAs);
    fileMenu->addSeparator();
    // close() goes through closeEvent(), so unsaved changes are checked.
    add(tr("&Quit"), QKeySequence::Quit, &QWidget::close);
}

void MainWindow::createStatusBar()
{
    m_fileLabel = new QLabel(this);
    // Ignored lets a long path shrink instead of widening the window.
    m_fileLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    m_cursorLabel = new QLabel(this);

    statusBar()->addWidget(m_fileLabel, 1);
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
    m_cursorLabel->setText(tr("Ln %1, Col %2")
                               .arg(cursor.blockNumber() + 1)
                               .arg(cursor.positionInBlock() + 1));
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