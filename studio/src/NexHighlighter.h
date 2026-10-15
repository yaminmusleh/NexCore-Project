#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>

// Highlights .nex source. Every category below comes from the lexical rules
// in docs/Nex.atg (TOKENS, COMMENTS and IGNORE sections).
class NexHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit NexHighlighter(QTextDocument *document);

protected:
    void highlightBlock(const QString &text) override;

private:
    QTextCharFormat m_controlKeyword;   // iff otherwise for whilst
    QTextCharFormat m_statementKeyword; // set print exit
    QTextCharFormat m_number;
    QTextCharFormat m_string;
    QTextCharFormat m_unterminatedString;
    QTextCharFormat m_comment;
    QTextCharFormat m_operator;
    QTextCharFormat m_punctuation;
    QTextCharFormat m_invalid;
};