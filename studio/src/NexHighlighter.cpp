#include "NexHighlighter.h"

#include <QColor>
#include <QFont>
#include <QStringView>

namespace {

bool isLetter(char16_t c)
{
    return (c >= u'a' && c <= u'z') || (c >= u'A' && c <= u'Z');
}

bool isDigit(char16_t c)
{
    return c >= u'0' && c <= u'9';
}

QTextCharFormat colored(const QColor &color)
{
    QTextCharFormat format;
    format.setForeground(color);
    return format;
}

} // namespace

NexHighlighter::NexHighlighter(QTextDocument *document)
    : QSyntaxHighlighter(document)
{
    const QColor red(0xff, 0x6b, 0x6b);

    m_controlKeyword = colored(QColor(0xc7, 0x92, 0xea));
    m_controlKeyword.setFontWeight(QFont::DemiBold);

    m_statementKeyword = colored(QColor(0x5f, 0xb3, 0xf0));
    m_statementKeyword.setFontWeight(QFont::DemiBold);

    m_number = colored(QColor(0xf2, 0xa6, 0x5a));
    m_string = colored(QColor(0x98, 0xc3, 0x79));

    m_unterminatedString = m_string;
    m_unterminatedString.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    m_unterminatedString.setUnderlineColor(red);

    m_comment = colored(QColor(0x6b, 0x70, 0x80));
    m_comment.setFontItalic(true);

    m_operator = colored(QColor(0x7f, 0xd1, 0xe8));
    m_punctuation = colored(QColor(0x9a, 0xa1, 0xb2));

    m_invalid = colored(red);
    m_invalid.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    m_invalid.setUnderlineColor(red);
}

void NexHighlighter::highlightBlock(const QString &text)
{
    const int n = text.size();
    int i = 0;

    // Block state = nesting depth of "/* ... */" comments that are still
    // open at the end of the previous line (the grammar marks them NESTED).
    int depth = qMax(0, previousBlockState());

    auto at = [&](int index) -> char16_t {
        return index < n ? text.at(index).unicode() : u'\0';
    };

    auto scanBlockComment = [&](int start) {
        while (i < n && depth > 0) {
            if (at(i) == u'/' && at(i + 1) == u'*') {
                ++depth;
                i += 2;
            } else if (at(i) == u'*' && at(i + 1) == u'/') {
                --depth;
                i += 2;
            } else {
                ++i;
            }
        }
        setFormat(start, i - start, m_comment);
    };

    if (depth > 0)
        scanBlockComment(0);

    while (i < n) {
        const char16_t c = at(i);
        const char16_t next = at(i + 1);

        // IGNORE '\r' + '\n' + '\t' + ' '
        if (c == u' ' || c == u'\t' || c == u'\r') {
            ++i;
            continue;
        }

        // COMMENTS FROM "//" TO "\n"
        if (c == u'/' && next == u'/') {
            setFormat(i, n - i, m_comment);
            i = n;
            continue;
        }

        // COMMENTS FROM "/*" TO "*/" NESTED
        if (c == u'/' && next == u'*') {
            const int start = i;
            i += 2;
            depth = 1;
            scanBlockComment(start);
            continue;
        }

        // string_lit = '"' { stringChar } '"'  (no quote or line break inside)
        if (c == u'"') {
            const int start = i;
            ++i;
            while (i < n && at(i) != u'"')
                ++i;
            const bool closed = i < n;
            if (closed)
                ++i;
            setFormat(start, i - start, closed ? m_string : m_unterminatedString);
            continue;
        }

        // int_lit = digit {digit}
        // float_lit / double_lit = digit {digit} "." digit {digit} ("f" | "d")
        if (isDigit(c)) {
            const int start = i;
            while (isDigit(at(i)))
                ++i;
            if (at(i) == u'.' && isDigit(at(i + 1))) {
                ++i;
                while (isDigit(at(i)))
                    ++i;
                if (at(i) == u'f' || at(i) == u'd') {
                    ++i;
                    setFormat(start, i - start, m_number);
                } else {
                    // The grammar has no decimal literal without f or d.
                    setFormat(start, i - start, m_invalid);
                }
            } else {
                setFormat(start, i - start, m_number);
            }
            continue;
        }

        // identifier = letter { letter | digit | "_" }, or a keyword
        if (isLetter(c)) {
            const int start = i;
            while (isLetter(at(i)) || isDigit(at(i)) || at(i) == u'_')
                ++i;
            const QStringView word = QStringView(text).mid(start, i - start);
            if (word == u"iff" || word == u"otherwise" || word == u"for"
                || word == u"whilst") {
                setFormat(start, i - start, m_controlKeyword);
            } else if (word == u"set" || word == u"print" || word == u"exit") {
                setFormat(start, i - start, m_statementKeyword);
            }
            continue;
        }

        // two-character operators: == != <= >= && ||
        if ((next == u'=' && (c == u'=' || c == u'!' || c == u'<' || c == u'>'))
            || (c == u'&' && next == u'&') || (c == u'|' && next == u'|')) {
            setFormat(i, 2, m_operator);
            i += 2;
            continue;
        }

        switch (c) {
        case u'=': case u'<': case u'>': case u'+': case u'-':
        case u'*': case u'/': case u'%': case u'!':
            setFormat(i, 1, m_operator);
            ++i;
            continue;
        case u';': case u'(': case u')': case u'{': case u'}':
            setFormat(i, 1, m_punctuation);
            ++i;
            continue;
        default:
            break;
        }

        // Anything else cannot start a token in Nex.atg (a lone & or |, a
        // ".", a leading "_", characters outside ASCII, ...).
        setFormat(i, 1, m_invalid);
        ++i;
    }

    setCurrentBlockState(depth);
}