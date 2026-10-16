#pragma once

#include <QPlainTextEdit>
#include <QPointer>
#include <QTextDocument>
#include <QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;

// Find / replace bar shown under the editor tabs. It always works on the
// editor passed to setEditor(), which the main window keeps pointed at the
// current tab.
class FindBar : public QWidget
{
    Q_OBJECT

public:
    explicit FindBar(QWidget *parent = nullptr);

    void setEditor(QPlainTextEdit *editor);

public slots:
    void openFind();
    void openReplace();
    void findNext();
    void findPrevious();

private slots:
    void onSearchChanged();
    void replaceCurrent();
    void replaceAll();
    void closeBar();

private:
    void open(bool withReplace);
    bool prepareSearch();
    bool find(bool forward, bool incremental);
    QTextDocument::FindFlags flags(bool backward) const;
    void showStatus(const QString &text, bool error = false);

    QPointer<QPlainTextEdit> m_editor;
    QLineEdit *m_findEdit = nullptr;
    QLineEdit *m_replaceEdit = nullptr;
    QWidget *m_replaceRow = nullptr;
    QCheckBox *m_caseBox = nullptr;
    QLabel *m_status = nullptr;
};