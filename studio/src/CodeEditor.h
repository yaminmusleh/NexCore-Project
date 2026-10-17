#pragma once

#include <QList>
#include <QPlainTextEdit>
#include <QString>

class QEvent;
class QKeyEvent;
class QPaintEvent;
class QPoint;
class QResizeEvent;

// Plain-text editor with a line-number gutter, current-line highlight,
// simple automatic indentation (4 spaces) and compiler error marks.
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    static constexpr int kIndentWidth = 4;

    // A reported error position. Line and column are 1-based, as the compiler
    // prints them (a tab counts as one column).
    struct ErrorMark {
        int line = 0;
        int column = 0;
        QString message;
    };

    explicit CodeEditor(QWidget *parent = nullptr);

    int lineNumberAreaWidth() const;

    // The file this editor is bound to; empty for a buffer that was never saved.
    QString filePath() const { return m_filePath; }
    void setFilePath(const QString &path) { m_filePath = path; }
    void setUntitledName(const QString &name) { m_untitledName = name; }
    // File name for tabs and titles ("untitled.nex" for an unsaved buffer).
    QString displayName() const;

    // Underlines the token at each position and puts a dot in the gutter. The
    // marks disappear as soon as the text is edited, because their positions
    // would no longer be right.
    void setErrorMarks(const QList<ErrorMark> &marks);
    void clearErrorMarks();
    bool hasErrorMarks() const { return !m_marks.isEmpty(); }
    // Message of the underlined token under a point (viewport coordinates),
    // or of the first error on a line (0-based). Empty if there is none.
    QString errorMessageAt(const QPoint &viewportPos) const;
    QString errorMessageForLine(int blockNumber) const;

    // Called by the gutter widget; not meant to be called from elsewhere.
    void lineNumberAreaPaintEvent(QPaintEvent *event);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool viewportEvent(QEvent *event) override;

private slots:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();

private:
    struct Mark {
        int position = 0;    // document position of the first character
        int length = 0;      // 0 = nothing to underline (empty line)
        int blockNumber = 0; // 0-based line
        QString message;
    };

    void insertNewlineWithIndent();
    void insertIndent();
    void indentLines(bool indent);
    bool dedentForClosingBrace();

    QWidget *m_lineNumberArea = nullptr;
    QString m_filePath;
    QString m_untitledName;
    QList<Mark> m_marks;
};