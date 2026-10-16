#pragma once

#include <QList>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringConverter>
#include <QTemporaryDir>
#include <QTimer>

#include <memory>

// Runs the NexCore compiler (nexc) on a saved .nex file and then, optionally,
// the program it produced. It has no widgets: the window listens to its
// signals. Only one build or run can be active at a time.
//
// nexc writes out.asm, out.o and out into its working directory and runs
// nasm and ld itself, so every build gets its own fresh directory and only
// the "out" from that directory is ever run.
class BuildController : public QObject
{
    Q_OBJECT

public:
    enum class State { Idle, Building, Running };

    struct Diagnostic {
        int line = 0;   // 1-based; 0 = the message has no location
        int column = 0; // 1-based
        QString message;
    };

    explicit BuildController(QObject *parent = nullptr);
    ~BuildController() override;

    State state() const { return m_state; }
    bool isBusy() const { return m_state != State::Idle; }

    QString compilerPath() const { return m_compiler; }
    void setCompilerPath(const QString &path) { m_compiler = path; }

    // The executable produced by the most recent build (may not exist).
    QString outputPath() const;

    // Looks for nexc: saved setting, NEXC_PATH, next to the application,
    // then PATH. Returns an empty string if none of them is usable.
    static QString locateCompiler();
    static bool isUsableCompiler(const QString &path);
    static void saveCompilerPath(const QString &path);

    // Ignored while another build or run is active.
    void build(const QString &sourcePath, bool runAfterBuild);

    // Stops the active build or program.
    void stop();

signals:
    void stateChanged(BuildController::State state);
    void buildStarted(const QString &commandLine);
    // Complete lines from nexc's standard output, and raw chunks of its
    // standard error.
    void buildOutput(const QString &text, bool isError);
    void buildFinished(bool success, const QString &summary,
                       const QList<BuildController::Diagnostic> &diagnostics);
    void programStarted();
    void programOutput(const QString &text, bool isError);
    void programFinished(const QString &summary, bool success, bool stopped);

private:
    void setState(State state);
    void readBuildStdout(QProcess *process);
    void readBuildStderr(QProcess *process);
    void onBuildFinished(QProcess *process, int exitCode, QProcess::ExitStatus status);
    void onBuildFailedToStart(QProcess *process);
    void startProgram();
    void queueProgramOutput(const QString &text, bool isError);
    void flushProgramOutput();
    void onProgramFinished(QProcess *process, int exitCode, QProcess::ExitStatus status);
    void onProgramFailedToStart(QProcess *process);

    static QList<Diagnostic> parseDiagnostics(const QString &text);
    static QString failureDetail(const QString &stdoutText, const QString &stderrText);

    State m_state = State::Idle;
    QString m_compiler;
    QString m_source;
    bool m_runAfterBuild = false;
    bool m_stopRequested = false;

    std::unique_ptr<QTemporaryDir> m_buildDir;
    QProcess *m_process = nullptr;

    QString m_stdoutText;       // everything nexc printed to stdout
    QString m_stderrText;       // everything nexc printed to stderr
    QString m_stdoutPartial;    // an unfinished stdout line

    QString m_pendingOut;       // program output waiting for the next flush
    QString m_pendingErr;
    QTimer m_flushTimer;
    QTimer m_killTimer;

    QStringDecoder m_buildOutDecoder;
    QStringDecoder m_buildErrDecoder;
    QStringDecoder m_programOutDecoder;
    QStringDecoder m_programErrDecoder;
};