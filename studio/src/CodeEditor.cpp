#include "CodeEditor.h"

#include <QColor>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QToolTip>

namespace {

const QColor kGutterBackground(0x1b, 0x1d, 0x22);
const QColor kGutterBorder(0x2a, 0x2d, 0x34);
const QColor kGutterText(0x5c, 0x61, 0x70);
const QColor kGutterCurrentText(0xd7, 0xda, 0xe0);
const QColor kCurrentLine(0x21, 0x24, 0x2a);
const QColor kErrorColor(0xff, 0x6b, 0x6b);

constexpr int kGutterPadding = 12;

bool isWordChar(QChar c)
{
    const char16_t u = c.unicode();
    return (u >= u'a' && u <= u'z') || (u >= u'A' && u <= u'Z')
           || (u >= u'0' && u <= u'9') || u == u'_';
}

// How many characters to underline for an error that starts at `start`: the
// whole word, string literal or two-character operator there, else one char.
int tokenLength(const QString &text, int start)
{
    const QChar first = text.at(start);
    int end = start + 1;

    if (isWordChar(first)) {
        while (end < text.size() && isWordChar(text.at(end)))
            ++end;
    } else if (first == QLatin1Char('"')) {
        while (end < text.size() && text.at(end) != QLatin1Char('"'))
            ++end;
        if (end < text.size())
            ++end;
    } else if (end < text.size()) {
        const QChar next = text.at(end);
        const bool pair = (next == QLatin1Char('=') && QStringLiteral("=!<>").contains(first))
                          || (first == QLatin1Char('&') && next == QLatin1Char('&'))
                          || (first == QLatin1Char('|') && next == QLatin1Char('|'));
        if (pair)
            ++end;
    }
    return end - start;
}

} // namespace

class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(CodeEditor *editor)
        : QWidget(editor), m_editor(editor)
    {
    }

    QSize sizeHint() const override
    {
        return QSize(m_editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        m_editor->lineNumberAreaPaintEvent(event);
    }

    // Hovering the red dot shows the error message. The gutter and the text
    // area share the same vertical coordinates.
    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::ToolTip) {
            const auto *help = static_cast<QHelpEvent *>(event);
            const int line = m_editor->cursorForPosition(QPoint(0, help->pos().y())).blockNumber();
            const QString message = m_editor->errorMessageForLine(line);
            if (!message.isEmpty()) {
                QToolTip::showText(help->globalPos(), message, this);
            } else {
                QToolTip::hideText();
                event->ignore();
            }
            return true;
        }
        return QWidget::event(event);
    }

private:
    CodeEditor *m_editor;
};

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    m_lineNumberArea = new LineNumberArea(this);

    setFrameShape(QFrame::NoFrame);
    setLineWrapMode(QPlainTextEdit::NoWrap);

    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(11);
    setFont(font);
    setTabStopDistance(kIndentWidth * fontMetrics().horizontalAdvance(QLatin1Char(' ')));

    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &CodeEditor::highlightCurrentLine);

    // Error marks describe the text as it was when it was compiled, so any
    // edit invalidates them.
    connect(document(), &QTextDocument::contentsChange, this, [this](int, int removed, int added) {
        if (!m_marks.isEmpty() && (removed > 0 || added > 0))
            clearErrorMarks();
    });

    updateLineNumberAreaWidth();
    highlightCurrentLine();
}

QString CodeEditor::displayName() const
{
    return m_filePath.isEmpty() ? m_untitledName : QFileInfo(m_filePath).fileName();
}

int CodeEditor::lineNumberAreaWidth() const
{
    int digits = 3;
    int lines = qMax(1, blockCount());
    while (lines >= 1000) {
        lines /= 10;
        ++digits;
    }
    const QFontMetrics metrics(document()->defaultFont());
    return 2 * kGutterPadding + metrics.horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeEditor::updateLineNumberAreaWidth()
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth();
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    const QRect area = contentsRect();
    m_lineNumberArea->setGeometry(
        QRect(area.left(), area.top(), lineNumberAreaWidth(), area.height()));
}

// Current-line background plus the error underlines.
void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> selections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(kCurrentLine);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        selections.append(selection);
    }

    for (const Mark &mark : std::as_const(m_marks)) {
        if (mark.length <= 0)
            continue;

        QTextEdit::ExtraSelection selection;
        selection.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        selection.format.setUnderlineColor(kErrorColor);
        selection.format.setBackground(QColor(0xff, 0x6b, 0x6b, 40));
        selection.cursor = QTextCursor(document());
        selection.cursor.setPosition(mark.position);
        selection.cursor.setPosition(mark.position + mark.length, QTextCursor::KeepAnchor);
        selections.append(selection);
    }

    setExtraSelections(selections);
    m_lineNumberArea->update();
}

void CodeEditor::setErrorMarks(const QList<ErrorMark> &marks)
{
    m_marks.clear();

    for (const ErrorMark &mark : marks) {
        if (mark.line < 1)
            continue;
        const QTextBlock block = document()->findBlockByNumber(mark.line - 1);
        if (!block.isValid())
            continue;

        const QString text = block.text();
        int offset = qMax(0, mark.column - 1);
        int length = 0;
        if (offset >= text.size()) {
            // Reported at or after the end of the line: mark the last character.
            offset = qMax(0, text.size() - 1);
            length = text.isEmpty() ? 0 : 1;
        } else {
            length = tokenLength(text, offset);
        }

        Mark entry;
        entry.position = block.position() + offset;
        entry.length = length;
        entry.blockNumber = mark.line - 1;
        entry.message = mark.message;
        m_marks.append(entry);
    }

    highlightCurrentLine();
}

void CodeEditor::clearErrorMarks()
{
    if (m_marks.isEmpty())
        return;
    m_marks.clear();
    highlightCurrentLine();
}

QString CodeEditor::errorMessageAt(const QPoint &viewportPos) const
{
    for (const Mark &mark : m_marks) {
        if (mark.length <= 0)
            continue;

        QTextCursor start(document());
        start.setPosition(mark.position);
        QTextCursor end(document());
        end.setPosition(mark.position + mark.length);

        const QRect first = cursorRect(start);
        const QRect last = cursorRect(end);
        const QRect hit(first.left(), first.top(), qMax(1, last.left() - first.left()), first.height());
        if (hit.contains(viewportPos))
            return mark.message;
    }
    return QString();
}

QString CodeEditor::errorMessageForLine(int blockNumber) const
{
    for (const Mark &mark : m_marks) {
        if (mark.blockNumber == blockNumber)
            return mark.message;
    }
    return QString();
}

bool CodeEditor::viewportEvent(QEvent *event)
{
    if (event->type() == QEvent::ToolTip && !m_marks.isEmpty()) {
        const auto *help = static_cast<QHelpEvent *>(event);
        const QString message = errorMessageAt(help->pos());
        if (!message.isEmpty()) {
            QToolTip::showText(help->globalPos(), message, viewport());
        } else {
            QToolTip::hideText();
            event->ignore();
        }
        return true;
    }
    return QPlainTextEdit::viewportEvent(event);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(event->rect(), kGutterBackground);
    painter.setPen(kGutterBorder);
    painter.drawLine(m_lineNumberArea->width() - 1, event->rect().top(),
                     m_lineNumberArea->width() - 1, event->rect().bottom());
    painter.setFont(document()->defaultFont());

    const int currentLine = textCursor().blockNumber();
    const int textWidth = m_lineNumberArea->width() - kGutterPadding;

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    qreal top = blockBoundingGeometry(block).translated(contentOffset()).top();
    qreal bottom = top + blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.setPen(blockNumber == currentLine ? kGutterCurrentText : kGutterText);
            painter.drawText(0, qRound(top), textWidth, qRound(bottom - top),
                             Qt::AlignRight | Qt::AlignVCenter,
                             QString::number(blockNumber + 1));

            if (!errorMessageForLine(blockNumber).isEmpty()) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(kErrorColor);
                painter.drawEllipse(QPointF(6.5, top + (bottom - top) / 2), 3.0, 3.0);
            }
        }
        block = block.next();
        top = bottom;
        bottom = top + blockBoundingRect(block).height();
        ++blockNumber;
    }
}

void CodeEditor::keyPressEvent(QKeyEvent *event)
{
    const Qt::KeyboardModifiers modifiers = event->modifiers() & ~Qt::KeypadModifier;

    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (modifiers == Qt::NoModifier || modifiers == Qt::ShiftModifier) {
            insertNewlineWithIndent();
            return;
        }
        break;
    case Qt::Key_Tab:
        if (modifiers == Qt::NoModifier) {
            insertIndent();
            return;
        }
        break;
    case Qt::Key_Backtab: // Shift+Tab
        indentLines(false);
        return;
    default:
        break;
    }

    if (event->text() == QLatin1String("}") && dedentForClosingBrace())
        return;

    QPlainTextEdit::keyPressEvent(event);
}

void CodeEditor::insertNewlineWithIndent()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    cursor.removeSelectedText();

    const QString line = cursor.block().text();
    const int column = cursor.positionInBlock();
    const QString before = line.left(column);
    const QString after = line.mid(column);

    // Copy the leading whitespace that lies before the cursor.
    QString indent;
    for (const QChar ch : before) {
        if (ch != QLatin1Char(' ') && ch != QLatin1Char('\t'))
            break;
        indent += ch;
    }

    const bool opensBlock = before.trimmed().endsWith(QLatin1Char('{'));
    const bool betweenBraces = opensBlock && after.trimmed().startsWith(QLatin1Char('}'));
    const QString extra = opensBlock ? QString(kIndentWidth, QLatin1Char(' ')) : QString();

    cursor.insertText(QStringLiteral("\n") + indent + extra);
    if (betweenBraces) {
        const int resume = cursor.position();
        cursor.insertText(QStringLiteral("\n") + indent);
        cursor.setPosition(resume);
    }
    cursor.endEditBlock();

    setTextCursor(cursor);
    ensureCursorVisible();
}

void CodeEditor::insertIndent()
{
    QTextCursor cursor = textCursor();
    const QTextDocument *doc = document();

    if (cursor.hasSelection()
        && doc->findBlock(cursor.selectionStart()) != doc->findBlock(cursor.selectionEnd())) {
        indentLines(true);
        return;
    }

    cursor.beginEditBlock();
    cursor.removeSelectedText();
    const int column = cursor.positionInBlock();
    cursor.insertText(QString(kIndentWidth - column % kIndentWidth, QLatin1Char(' ')));
    cursor.endEditBlock();
    setTextCursor(cursor);
}

void CodeEditor::indentLines(bool indent)
{
    QTextCursor cursor = textCursor();
    const QTextDocument *doc = document();

    const int firstLine = doc->findBlock(cursor.selectionStart()).blockNumber();
    const QTextBlock lastBlock = doc->findBlock(cursor.selectionEnd());
    int lastLine = lastBlock.blockNumber();
    // A selection that ends at the very start of a line does not include it.
    if (cursor.hasSelection() && cursor.selectionEnd() == lastBlock.position()
        && lastLine > firstLine) {
        --lastLine;
    }

    cursor.beginEditBlock();
    for (int number = firstLine; number <= lastLine; ++number) {
        const QTextBlock block = doc->findBlockByNumber(number);
        const QString text = block.text();
        QTextCursor edit(block);

        if (indent) {
            if (!text.isEmpty())
                edit.insertText(QString(kIndentWidth, QLatin1Char(' ')));
        } else {
            int remove = 0;
            if (text.startsWith(QLatin1Char('\t'))) {
                remove = 1;
            } else {
                while (remove < kIndentWidth && remove < text.size()
                       && text.at(remove) == QLatin1Char(' ')) {
                    ++remove;
                }
            }
            if (remove > 0) {
                edit.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, remove);
                edit.removeSelectedText();
            }
        }
    }
    cursor.endEditBlock();
}

// Typing "}" on a line that holds only indentation moves it one level left.
bool CodeEditor::dedentForClosingBrace()
{
    QTextCursor cursor = textCursor();
    if (cursor.hasSelection())
        return false;

    const QString before = cursor.block().text().left(cursor.positionInBlock());
    if (before.isEmpty() || !before.trimmed().isEmpty())
        return false;

    int remove = 0;
    if (before.endsWith(QLatin1Char('\t'))) {
        remove = 1;
    } else if (before == QString(before.size(), QLatin1Char(' '))) {
        const int remainder = before.size() % kIndentWidth;
        remove = remainder == 0 ? kIndentWidth : remainder;
    }

    cursor.beginEditBlock();
    if (remove > 0) {
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, remove);
        cursor.removeSelectedText();
    }
    cursor.insertText(QStringLiteral("}"));
    cursor.endEditBlock();
    setTextCursor(cursor);
    return true;
}