#include "OutputPanel.h"

#include <QColor>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QStackedWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

constexpr int kMaxLines = 20000;

QColor colorFor(OutputPanel::Kind kind)
{
    switch (kind) {
    case OutputPanel::Kind::Error:
        return QColor(0xff, 0x6b, 0x6b);
    case OutputPanel::Kind::Success:
        return QColor(0x98, 0xc3, 0x79);
    case OutputPanel::Kind::Info:
        return QColor(0x6b, 0x70, 0x80);
    case OutputPanel::Kind::Normal:
        break;
    }
    return QColor(0xd7, 0xda, 0xe0);
}

QPlainTextEdit *makeTextView(const QString &placeholder, QWidget *parent)
{
    auto *view = new QPlainTextEdit(parent);
    view->setReadOnly(true);
    view->setFrameShape(QFrame::NoFrame);
    view->setLineWrapMode(QPlainTextEdit::NoWrap);
    view->setMaximumBlockCount(kMaxLines);
    view->setPlaceholderText(placeholder);

    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(10);
    view->setFont(font);
    return view;
}

// Appends at the end without touching the user's selection, and keeps
// following the output only if the view was already scrolled to the bottom.
void appendText(QPlainTextEdit *view, QString text, OutputPanel::Kind kind)
{
    if (text.isEmpty())
        return;
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));

    QScrollBar *bar = view->verticalScrollBar();
    const bool atBottom = bar->value() >= bar->maximum() - 2;

    QTextCharFormat format;
    format.setForeground(colorFor(kind));

    QTextCursor cursor(view->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text, format);

    if (atBottom)
        bar->setValue(bar->maximum());
}

} // namespace

OutputPanel::OutputPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("outputPanel"));

    m_tabs = new QTabWidget(this);
    m_tabs->setObjectName(QStringLiteral("panelTabs"));
    // Not document mode: that mode makes Qt draw a bright base line under the
    // corner buttons.
    m_tabs->tabBar()->setDrawBase(false);

    m_build = makeTextView(tr("Build output will appear here."), this);
    m_program = makeTextView(tr("Program output will appear here."), this);

    // Problems: a list, or a short message while there is nothing to show.
    m_problemStack = new QStackedWidget(this);
    auto *noProblems = new QLabel(tr("No problems."), this);
    noProblems->setObjectName(QStringLiteral("panelHint"));
    noProblems->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_problemList = new QListWidget(this);
    m_problemList->setObjectName(QStringLiteral("problemList"));
    m_problemList->setFrameShape(QFrame::NoFrame);
    m_problemList->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_problemList->setUniformItemSizes(true);
    m_problemStack->addWidget(noProblems);
    m_problemStack->addWidget(m_problemList);

    connect(m_problemList, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        emit problemActivated(item->data(Qt::UserRole).toString(),
                              item->data(Qt::UserRole + 1).toInt(),
                              item->data(Qt::UserRole + 2).toInt());
    });

    m_tabs->addTab(m_build, tr("Build Output"));
    m_tabs->addTab(m_problemStack, tr("Problems"));
    m_tabs->addTab(m_program, tr("Program Output"));

    // Controls at the right end of the tab bar.
    auto *corner = new QWidget(this);
    auto *cornerLayout = new QHBoxLayout(corner);
    cornerLayout->setContentsMargins(0, 0, 6, 0);
    cornerLayout->setSpacing(2);

    auto makeButton = [this, corner](const QString &text, const QString &tip) {
        auto *button = new QToolButton(corner);
        button->setObjectName(QStringLiteral("panelButton"));
        button->setText(text);
        button->setToolTip(tip);
        button->setFocusPolicy(Qt::NoFocus);
        return button;
    };

    QToolButton *clearButton = makeButton(tr("Clear"), tr("Clear this tab"));
    m_maximizeButton = makeButton(QString::fromUtf8("\u25b2"), tr("Maximize panel"));
    QToolButton *hideButton = makeButton(QString::fromUtf8("\u00d7"), tr("Hide panel (Ctrl+J)"));
    cornerLayout->addWidget(clearButton);
    cornerLayout->addWidget(m_maximizeButton);
    cornerLayout->addWidget(hideButton);
    m_tabs->setCornerWidget(corner, Qt::TopRightCorner);

    connect(clearButton, &QToolButton::clicked, this, &OutputPanel::clearCurrent);
    connect(hideButton, &QToolButton::clicked, this, &OutputPanel::hideRequested);
    connect(m_maximizeButton, &QToolButton::clicked, this, [this] {
        setPanelMaximized(!m_maximized);
        emit maximizeToggled(m_maximized);
    });

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_tabs);
}

void OutputPanel::appendBuildOutput(const QString &text, Kind kind)
{
    appendText(m_build, text, kind);
}

void OutputPanel::appendProgramOutput(const QString &text, Kind kind)
{
    appendText(m_program, text, kind);
}

void OutputPanel::setProblems(const QList<Problem> &problems)
{
    m_problemList->clear();

    for (const Problem &problem : problems) {
        QString location;
        if (!problem.file.isEmpty()) {
            location = QStringLiteral("%1:%2:%3  ")
                           .arg(QFileInfo(problem.file).fileName())
                           .arg(problem.line)
                           .arg(problem.column);
        }

        auto *item = new QListWidgetItem(
            QStringLiteral("%1  %2%3")
                .arg(problem.isError ? tr("error") : tr("warning"), location, problem.message),
            m_problemList);
        item->setForeground(problem.isError ? QColor(0xff, 0x6b, 0x6b) : QColor(0xf2, 0xa6, 0x5a));
        item->setToolTip(problem.file);
        item->setData(Qt::UserRole, problem.file);
        item->setData(Qt::UserRole + 1, problem.line);
        item->setData(Qt::UserRole + 2, problem.column);
    }

    m_tabs->setTabText(int(Tab::Problems),
                       problems.isEmpty() ? tr("Problems")
                                          : tr("Problems (%1)").arg(problems.size()));
    m_problemStack->setCurrentIndex(problems.isEmpty() ? 0 : 1);
}

void OutputPanel::showTab(Tab tab)
{
    m_tabs->setCurrentIndex(int(tab));
}

void OutputPanel::clearAll()
{
    m_build->clear();
    m_program->clear();
    setProblems({});
}

void OutputPanel::setPanelMaximized(bool maximized)
{
    m_maximized = maximized;
    m_maximizeButton->setText(QString::fromUtf8(maximized ? "\u25bc" : "\u25b2"));
    m_maximizeButton->setToolTip(maximized ? tr("Restore panel size") : tr("Maximize panel"));
}

void OutputPanel::clearCurrent()
{
    switch (Tab(m_tabs->currentIndex())) {
    case Tab::Build:
        m_build->clear();
        break;
    case Tab::Problems:
        setProblems({});
        break;
    case Tab::Program:
        m_program->clear();
        break;
    }
}