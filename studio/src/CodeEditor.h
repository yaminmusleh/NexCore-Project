#pragma once

#include <QPlainTextEdit>

class QKeyEvent;
class QPaintEvent;
class QResizeEvent;

// Plain-text editor with a line-number gutter, current-line highlight and
// simple automatic indentation (4 spaces).
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    static constexpr int kIndentWidth = 4;

    explicit CodeEditor(QWidget *parent = nullptr);

    int lineNumberAreaWidth() const;

    // The file this editor is bound to; empty for a buffer that was never saved.
    QString filePath() const { return m_filePath; }
    void setFilePath(const QString &path) { m_filePath = path; }
    void setUntitledName(const QString &name) { m_untitledName = name; }
    // File name for tabs and titles ("untitled.nex" for an unsaved buffer).
    QString displayName() const;

    // Called by the gutter widget; not meant to be called from elsewhere.
    void lineNumberAreaPaintEvent(QPaintEvent *event);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();

private:
    void insertNewlineWithIndent();
    void insertIndent();
    void indentLines(bool indent);
    bool dedentForClosingBrace();

    QWidget *m_lineNumberArea = nullptr;
    QString m_filePath;
    QString m_untitledName;
};