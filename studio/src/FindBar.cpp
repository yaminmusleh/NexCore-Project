#include "FindBar.h"

#include <QApplication>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>
#include <QTextCursor>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

// Counting matches for the "3 of 12" label stops here on huge files.
constexpr int kCountLimit = 5000;

// A search term is taken from a selection only if it is a single short line.
constexpr int kMaxSeedLength = 200;

} // namespace

FindBar::FindBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("findBar"));
    setAttribute(Qt::WA_StyledBackground, true);

    m_findEdit = new QLineEdit(this);
    m_findEdit->setPlaceholderText(tr("Find"));
    m_findEdit->setMinimumWidth(180);

    auto *previousButton = new QPushButton(tr("Previous"), this);
    auto *nextButton = new QPushButton(tr("Next"), this);
    previousButton->setFocusPolicy(Qt::NoFocus);
    nextButton->setFocusPolicy(Qt::NoFocus);

    m_caseBox = new QCheckBox(tr("Match case"), this);
    m_caseBox->setFocusPolicy(Qt::NoFocus);

    m_status = new QLabel(this);
    m_status->setMinimumWidth(110);

    auto *closeButton = new QToolButton(this);
    closeButton->setObjectName(QStringLiteral("findClose"));
    closeButton->setText(QString::fromUtf8("\u00d7"));
    closeButton->setToolTip(tr("Close (Esc)"));
    closeButton->setFocusPolicy(Qt::NoFocus);

    auto *findRow = new QHBoxLayout;
    findRow->setSpacing(8);
    findRow->addWidget(m_findEdit, 1);
    findRow->addWidget(previousButton);
    findRow->addWidget(nextButton);
    findRow->addWidget(m_caseBox);
    findRow->addWidget(m_status);
    findRow->addWidget(closeButton);

    m_replaceEdit = new QLineEdit(this);
    m_replaceEdit->setPlaceholderText(tr("Replace with"));
    m_replaceEdit->setMinimumWidth(180);

    auto *replaceButton = new QPushButton(tr("Replace"), this);
    auto *replaceAllButton = new QPushButton(tr("Replace All"), this);
    replaceButton->setFocusPolicy(Qt::NoFocus);
    replaceAllButton->setFocusPolicy(Qt::NoFocus);

    m_replaceRow = new QWidget(this);
    auto *replaceLayout = new QHBoxLayout(m_replaceRow);
    replaceLayout->setContentsMargins(0, 0, 0, 0);
    replaceLayout->setSpacing(8);
    replaceLayout->addWidget(m_replaceEdit, 1);
    replaceLayout->addWidget(replaceButton);
    replaceLayout->addWidget(replaceAllButton);
    replaceLayout->addStretch(1);
    m_replaceRow->hide();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(6);
    layout->addLayout(findRow);
    layout->addWidget(m_replaceRow);

    // Esc closes the bar while focus is anywhere inside it.
    auto *escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    escape->setContext(Qt::WidgetWithChildrenShortcut);
    connect(escape, &QShortcut::activated, this, &FindBar::closeBar);

    connect(m_findEdit, &QLineEdit::textChanged, this, &FindBar::onSearchChanged);
    connect(m_caseBox, &QCheckBox::toggled, this, &FindBar::onSearchChanged);
    connect(m_findEdit, &QLineEdit::returnPressed, this, [this] {
        if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
            findPrevious();
        else
            findNext();
    });
    connect(m_replaceEdit, &QLineEdit::returnPressed, this, &FindBar::replaceCurrent);
    connect(nextButton, &QPushButton::clicked, this, &FindBar::findNext);
    connect(previousButton, &QPushButton::clicked, this, &FindBar::findPrevious);
    connect(replaceButton, &QPushButton::clicked, this, &FindBar::replaceCurrent);
    connect(replaceAllButton, &QPushButton::clicked, this, &FindBar::replaceAll);
    connect(closeButton, &QToolButton::clicked, this, &FindBar::closeBar);
}

void FindBar::setEditor(QPlainTextEdit *editor)
{
    if (m_editor == editor)
        return;
    m_editor = editor;
    showStatus(QString());
}

void FindBar::openFind()
{
    open(false);
}

void FindBar::openReplace()
{
    open(true);
}

void FindBar::open(bool withReplace)
{
    if (!m_editor)
        return;

    show();
    m_replaceRow->setVisible(withReplace);

    // Start from the selected text when it is a short single line.
    const QString selected = m_editor->textCursor().selectedText();
    if (!selected.isEmpty() && selected.size() <= kMaxSeedLength
        && !selected.contains(QChar::ParagraphSeparator)) {
        m_findEdit->setText(selected);
    }

    m_findEdit->setFocus();
    m_findEdit->selectAll();
}

void FindBar::closeBar()
{
    hide();
    showStatus(QString());
    if (m_editor)
        m_editor->setFocus();
}

// Makes sure the bar is visible for F3 / Shift+F3 and that there is a term.
bool FindBar::prepareSearch()
{
    if (!m_editor)
        return false;

    if (isHidden())
        show();

    if (m_findEdit->text().isEmpty()) {
        m_findEdit->setFocus();
        return false;
    }
    return true;
}

void FindBar::findNext()
{
    if (prepareSearch())
        find(true, false);
}

void FindBar::findPrevious()
{
    if (prepareSearch())
        find(false, false);
}

void FindBar::onSearchChanged()
{
    if (m_findEdit->text().isEmpty()) {
        showStatus(QString());
        return;
    }
    find(true, true);
}

QTextDocument::FindFlags FindBar::flags(bool backward) const
{
    QTextDocument::FindFlags result;
    if (backward)
        result |= QTextDocument::FindBackward;
    if (m_caseBox->isChecked())
        result |= QTextDocument::FindCaseSensitively;
    return result;
}

// `incremental` = the term changed while typing: search from the start of the
// current match so the match does not jump forward on every keystroke.
bool FindBar::find(bool forward, bool incremental)
{
    if (!m_editor)
        return false;

    const QString term = m_findEdit->text();
    QTextDocument *doc = m_editor->document();

    QTextCursor from = m_editor->textCursor();
    if (incremental)
        from.setPosition(from.selectionStart());

    QTextCursor found = doc->find(term, from, flags(!forward));
    bool wrapped = false;
    if (found.isNull()) {
        QTextCursor edge(doc);
        edge.movePosition(forward ? QTextCursor::Start : QTextCursor::End);
        found = doc->find(term, edge, flags(!forward));
        wrapped = true;
    }

    if (found.isNull()) {
        showStatus(tr("No results"), true);
        return false;
    }

    m_editor->setTextCursor(found);

    int total = 0;
    int index = 0;
    QTextCursor scan(doc);
    while (total < kCountLimit) {
        scan = doc->find(term, scan, flags(false));
        if (scan.isNull())
            break;
        ++total;
        if (scan.selectionStart() == found.selectionStart())
            index = total;
    }

    QString text = total >= kCountLimit ? tr("%1+ matches").arg(kCountLimit)
                                        : tr("%1 of %2").arg(index).arg(total);
    if (wrapped)
        text += tr("  (wrapped)");
    showStatus(text);
    return true;
}

void FindBar::replaceCurrent()
{
    if (!m_editor || m_findEdit->text().isEmpty())
        return;

    // Replace the current match first, then move on to the next one.
    QTextCursor selection = m_editor->textCursor();
    const Qt::CaseSensitivity sensitivity =
        m_caseBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    if (selection.hasSelection()
        && selection.selectedText().compare(m_findEdit->text(), sensitivity) == 0) {
        selection.insertText(m_replaceEdit->text());
        m_editor->setTextCursor(selection);
    }
    find(true, false);
}

void FindBar::replaceAll()
{
    if (!m_editor || m_findEdit->text().isEmpty())
        return;

    const QString term = m_findEdit->text();
    const QString replacement = m_replaceEdit->text();
    QTextDocument *doc = m_editor->document();

    int count = 0;
    QTextCursor edit(doc);
    edit.beginEditBlock(); // one undo step for the whole operation

    QTextCursor position(doc);
    while (true) {
        QTextCursor found = doc->find(term, position, flags(false));
        if (found.isNull())
            break;
        found.insertText(replacement);
        position = found; // continues after the inserted text, so a
                          // replacement that contains the term cannot loop
        ++count;
    }
    edit.endEditBlock();

    if (count == 0)
        showStatus(tr("No results"), true);
    else
        showStatus(tr("%1 replaced").arg(count));
}

void FindBar::showStatus(const QString &text, bool error)
{
    m_status->setText(text);
    m_status->setStyleSheet(error ? QStringLiteral("color: #ff6b6b;")
                                  : QStringLiteral("color: #6b7080;"));
}