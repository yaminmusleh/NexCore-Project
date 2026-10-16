#pragma once

#include <QList>
#include <QString>
#include <QWidget>

class QListWidget;
class QPlainTextEdit;
class QStackedWidget;
class QTabWidget;
class QToolButton;

// Bottom panel with three tabs: build output, problems and program output.
// It only displays what it is given; the build and run code feeds it.
class OutputPanel : public QWidget
{
    Q_OBJECT

public:
    enum class Tab { Build = 0, Problems = 1, Program = 2 };
    enum class Kind { Normal, Error, Success, Info };

    struct Problem {
        QString file;      // empty if the message has no location
        int line = 0;      // 1-based
        int column = 0;    // 1-based
        QString message;
        bool isError = true;
    };

    explicit OutputPanel(QWidget *parent = nullptr);

    // Text is appended exactly as given, so callers pass the line breaks.
    void appendBuildOutput(const QString &text, Kind kind = Kind::Normal);
    void appendProgramOutput(const QString &text, Kind kind = Kind::Normal);
    void setProblems(const QList<Problem> &problems);

    void showTab(Tab tab);
    void clearAll();

    bool panelMaximized() const { return m_maximized; }
    void setPanelMaximized(bool maximized); // updates the button, emits nothing

signals:
    void problemActivated(const QString &file, int line, int column);
    void hideRequested();
    void maximizeToggled(bool maximized);

private:
    void clearCurrent();

    QTabWidget *m_tabs = nullptr;
    QPlainTextEdit *m_build = nullptr;
    QPlainTextEdit *m_program = nullptr;
    QStackedWidget *m_problemStack = nullptr;
    QListWidget *m_problemList = nullptr;
    QToolButton *m_maximizeButton = nullptr;
    bool m_maximized = false;
};