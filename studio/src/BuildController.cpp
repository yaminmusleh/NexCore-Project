#include "BuildController.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>

namespace {

constexpr qsizetype kMaxPendingChars = 4 * 1024 * 1024;
constexpr qsizetype kKeepChars = 1024 * 1024;
constexpr int kFlushIntervalMs = 30;
constexpr int kKillAfterMs = 1500;

QString compilerKey()
{
    return QStringLiteral("compilerPath");
}

QString quoted(const QString &text)
{
    return text.contains(QLatin1Char(' ')) ? QStringLiteral("\"%1\"").arg(text) : text;
}

QString decode(QStringDecoder &decoder, const QByteArray &bytes)
{
    return decoder(bytes);
}

// A fresh directory per build. The user cache directory is preferred over
// /tmp, which can be mounted noexec.
std::unique_ptr<QTemporaryDir> makeBuildDir()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (!base.isEmpty() && QDir().mkpath(base)) {
        auto dir = std::make_unique<QTemporaryDir>(QDir(base).filePath(QStringLiteral("build-XXXXXX")));
        if (dir->isValid())
            return dir;
    }

    auto fallback = std::make_unique<QTemporaryDir>();
    if (fallback->isValid())
        return fallback;
    return nullptr;
}

} // namespace

BuildController::BuildController(QObject *parent)
    : QObject(parent)
    , m_buildOutDecoder(QStringConverter::Utf8)
    , m_buildErrDecoder(QStringConverter::Utf8)
    , m_programOutDecoder(QStringConverter::Utf8)
    , m_programErrDecoder(QStringConverter::Utf8)
{
    m_flushTimer.setSingleShot(true);
    m_flushTimer.setInterval(kFlushIntervalMs);
    connect(&m_flushTimer, &QTimer::timeout, this, &BuildController::flushProgramOutput);

    m_killTimer.setSingleShot(true);
    m_killTimer.setInterval(kKillAfterMs);
    connect(&m_killTimer, &QTimer::timeout, this, [this] {
        if (m_process)
            m_process->kill();
    });
}

BuildController::~BuildController()
{
    m_flushTimer.stop();
    m_killTimer.stop();

    // Never leave a compiler or a user program running after the window is gone.
    if (m_process) {
        m_process->disconnect(this);
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

QString BuildController::outputPath() const
{
    return m_buildDir ? m_buildDir->filePath(QStringLiteral("out")) : QString();
}

bool BuildController::isUsableCompiler(const QString &path)
{
    if (path.isEmpty())
        return false;
    const QFileInfo info(path);
    return info.isFile() && info.isExecutable();
}

void BuildController::saveCompilerPath(const QString &path)
{
    QSettings settings;
    settings.setValue(compilerKey(), path);
}

QString BuildController::locateCompiler()
{
    QStringList candidates;
    candidates << QSettings().value(compilerKey()).toString();
    candidates << qEnvironmentVariable("NEXC_PATH");

    // Typical layouts: nexc built next to the app, or in ../build.
    const QDir appDir(QCoreApplication::applicationDirPath());
    candidates << appDir.filePath(QStringLiteral("nexc"))
               << appDir.filePath(QStringLiteral("../build/nexc"))
               << appDir.filePath(QStringLiteral("../nexc"))
               << appDir.filePath(QStringLiteral("../../build/nexc"));

    for (const QString &candidate : std::as_const(candidates)) {
        if (isUsableCompiler(candidate))
            return QDir::cleanPath(QFileInfo(candidate).absoluteFilePath());
    }

    return QStandardPaths::findExecutable(QStringLiteral("nexc"));
}

void BuildController::setState(State state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged(state);
}

// ---- building ---------------------------------------------------------------

void BuildController::build(const QString &sourcePath, bool runAfterBuild)
{
    if (m_state != State::Idle)
        return; // builds and runs never overlap

    m_runAfterBuild = runAfterBuild;
    m_stopRequested = false;
    m_source = QFileInfo(sourcePath).absoluteFilePath();

    // Check everything that can be checked before starting.
    QString problem;
    if (!QFileInfo(m_source).isFile()) {
        problem = tr("Source file not found: %1").arg(QDir::toNativeSeparators(m_source));
    } else if (!isUsableCompiler(m_compiler)) {
        problem = tr("The compiler was not found or is not executable: %1")
                      .arg(QDir::toNativeSeparators(m_compiler));
    } else if (QStandardPaths::findExecutable(QStringLiteral("nasm")).isEmpty()) {
        problem = tr("nasm was not found in PATH. Install it with: sudo apt install nasm");
    } else if (QStandardPaths::findExecutable(QStringLiteral("ld")).isEmpty()) {
        problem = tr("ld was not found in PATH. Install it with: sudo apt install binutils");
    }

    if (problem.isEmpty()) {
        m_buildDir.reset(); // removes the previous build's files
        m_buildDir = makeBuildDir();
        if (!m_buildDir)
            problem = tr("Could not create a temporary build directory.");
    }

    if (!problem.isEmpty()) {
        emit buildOutput(problem + QLatin1Char('\n'), true);
        emit buildFinished(false, problem, {{0, 0, problem}});
        return;
    }

    m_stdoutText.clear();
    m_stderrText.clear();
    m_stdoutPartial.clear();
    m_buildOutDecoder = QStringDecoder(QStringConverter::Utf8);
    m_buildErrDecoder = QStringDecoder(QStringConverter::Utf8);

    setState(State::Building);
    emit buildStarted(QStringLiteral("%1 %2").arg(quoted(QDir::toNativeSeparators(m_compiler)),
                                                  quoted(QDir::toNativeSeparators(m_source))));

    // The source path is an argument, never part of a shell command line, and
    // nexc runs in the fresh directory so its output files land there.
    QProcess *process = new QProcess(this);
    m_process = process;
    process->setProgram(m_compiler);
    process->setArguments({m_source});
    process->setWorkingDirectory(m_buildDir->path());

    connect(process, &QProcess::readyReadStandardOutput, this, [this, process] { readBuildStdout(process); });
    connect(process, &QProcess::readyReadStandardError, this, [this, process] { readBuildStderr(process); });
    connect(process, &QProcess::finished, this, [this, process](int code, QProcess::ExitStatus status) {
        onBuildFinished(process, code, status);
    });
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            onBuildFailedToStart(process);
    });

    process->start();
}

void BuildController::readBuildStdout(QProcess *process)
{
    const QByteArray bytes = process->readAllStandardOutput();
    const QString text = decode(m_buildOutDecoder, bytes);
    if (text.isEmpty())
        return;

    m_stdoutText += text;
    m_stdoutPartial += text;

    // Whole lines only, so a line can be recognised as an error line.
    qsizetype newline;
    while ((newline = m_stdoutPartial.indexOf(QLatin1Char('\n'))) >= 0) {
        const QString line = m_stdoutPartial.left(newline + 1);
        m_stdoutPartial.remove(0, newline + 1);
        emit buildOutput(line, line.startsWith(QLatin1String("-- line ")));
    }
}

void BuildController::readBuildStderr(QProcess *process)
{
    const QByteArray bytes = process->readAllStandardError();
    const QString text = decode(m_buildErrDecoder, bytes);
    if (text.isEmpty())
        return;

    m_stderrText += text;
    emit buildOutput(text, true);
}

void BuildController::onBuildFailedToStart(QProcess *process)
{
    if (process != m_process)
        return;
    m_process = nullptr;
    process->deleteLater();

    const QString message = tr("Could not start the compiler: %1").arg(process->errorString());
    emit buildOutput(message + QLatin1Char('\n'), true);
    emit buildFinished(false, message, {{0, 0, message}});
    setState(State::Idle);
}

void BuildController::onBuildFinished(QProcess *process, int exitCode, QProcess::ExitStatus status)
{
    if (process != m_process)
        return;

    // Pick up anything that arrived just before the process ended.
    readBuildStdout(process);
    readBuildStderr(process);
    if (!m_stdoutPartial.isEmpty()) {
        const QString rest = m_stdoutPartial;
        m_stdoutPartial.clear();
        emit buildOutput(rest + QLatin1Char('\n'), rest.startsWith(QLatin1String("-- line ")));
    }

    m_process = nullptr;
    m_killTimer.stop();
    process->deleteLater();

    QList<Diagnostic> diagnostics = parseDiagnostics(m_stdoutText + QLatin1Char('\n') + m_stderrText);

    // Success needs a normal exit with code 0, no reported errors, and an
    // executable. The exit code alone is not trusted: a compiler that keeps
    // going after a syntax error can still exit with 0.
    bool success = false;
    QString summary;
    if (m_stopRequested) {
        summary = tr("Build stopped.");
    } else if (status == QProcess::CrashExit) {
        summary = tr("The compiler crashed.");
    } else if (exitCode != 0) {
        summary = tr("Build failed (exit code %1).").arg(exitCode);
    } else if (!diagnostics.isEmpty()) {
        summary = tr("Build failed: the compiler reported %1 error(s).").arg(diagnostics.size());
    } else if (!QFileInfo(outputPath()).isExecutable()) {
        summary = tr("The compiler finished without producing an executable.");
    } else {
        success = true;
        summary = tr("Build succeeded.");
    }

    // A failure without a located error (a crash, a failed nasm or ld step)
    // still gets one entry, so it shows up in the Problems tab.
    if (!success && !m_stopRequested && diagnostics.isEmpty()) {
        QString detail = failureDetail(m_stdoutText, m_stderrText);
        if (detail.isEmpty())
            detail = summary;
        diagnostics.append({0, 0, detail});
    }

    emit buildFinished(success, summary, diagnostics);

    if (success && m_runAfterBuild)
        startProgram();
    else
        setState(State::Idle);
}

QList<BuildController::Diagnostic> BuildController::parseDiagnostics(const QString &text)
{
    // Coco/R's format: "-- line 3 col 9: message" (both numbers 1-based; a tab
    // counts as one column).
    static const QRegularExpression pattern(
        QStringLiteral("^-- line (\\d+) col (\\d+): (.*)$"),
        QRegularExpression::MultilineOption);

    QList<Diagnostic> result;
    auto matches = pattern.globalMatch(text);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        result.append({match.captured(1).toInt(), match.captured(2).toInt(),
                       match.captured(3).trimmed()});
    }
    return result;
}

// For failures that have no "-- line" message: the text of an uncaught C++
// exception ("what():  ..."), else the last line the compiler printed.
QString BuildController::failureDetail(const QString &stdoutText, const QString &stderrText)
{
    static const QRegularExpression what(QStringLiteral("what\\(\\):\\s*(.+)"));
    const QRegularExpressionMatch match = what.match(stderrText);
    if (match.hasMatch())
        return match.captured(1).trimmed();

    auto lastLine = [](const QString &text) {
        const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        return lines.isEmpty() ? QString() : lines.last().trimmed();
    };

    QString detail = lastLine(stderrText);
    if (detail.isEmpty())
        detail = lastLine(stdoutText);
    return detail;
}

// ---- running ----------------------------------------------------------------

void BuildController::startProgram()
{
    m_pendingOut.clear();
    m_pendingErr.clear();
    m_programOutDecoder = QStringDecoder(QStringConverter::Utf8);
    m_programErrDecoder = QStringDecoder(QStringConverter::Utf8);

    QProcess *process = new QProcess(this);
    m_process = process;
    process->setProgram(outputPath());
    process->setWorkingDirectory(QFileInfo(m_source).absolutePath());

    connect(process, &QProcess::readyReadStandardOutput, this, [this, process] {
        queueProgramOutput(decode(m_programOutDecoder, process->readAllStandardOutput()), false);
    });
    connect(process, &QProcess::readyReadStandardError, this, [this, process] {
        queueProgramOutput(decode(m_programErrDecoder, process->readAllStandardError()), true);
    });
    connect(process, &QProcess::finished, this, [this, process](int code, QProcess::ExitStatus status) {
        onProgramFinished(process, code, status);
    });
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            onProgramFailedToStart(process);
    });

    setState(State::Running);
    emit programStarted();

    process->start();
    process->closeWriteChannel(); // the language has no input
}

// Program output is collected and handed over every few milliseconds, so a
// program that prints in a tight loop cannot freeze the window.
void BuildController::queueProgramOutput(const QString &text, bool isError)
{
    if (text.isEmpty())
        return;

    QString &pending = isError ? m_pendingErr : m_pendingOut;
    pending += text;
    if (pending.size() > kMaxPendingChars)
        pending = tr("[output truncated]\n") + pending.right(kKeepChars);

    if (!m_flushTimer.isActive())
        m_flushTimer.start();
}

void BuildController::flushProgramOutput()
{
    if (!m_pendingOut.isEmpty()) {
        const QString text = m_pendingOut;
        m_pendingOut.clear();
        emit programOutput(text, false);
    }
    if (!m_pendingErr.isEmpty()) {
        const QString text = m_pendingErr;
        m_pendingErr.clear();
        emit programOutput(text, true);
    }
}

void BuildController::onProgramFailedToStart(QProcess *process)
{
    if (process != m_process)
        return;
    m_process = nullptr;
    process->deleteLater();

    emit programFinished(tr("Could not start the program: %1").arg(process->errorString()),
                         false, false);
    setState(State::Idle);
}

void BuildController::onProgramFinished(QProcess *process, int exitCode, QProcess::ExitStatus status)
{
    if (process != m_process)
        return;

    // Output that arrived just before the end, in order, then the summary.
    queueProgramOutput(decode(m_programOutDecoder, process->readAllStandardOutput()), false);
    queueProgramOutput(decode(m_programErrDecoder, process->readAllStandardError()), true);
    m_flushTimer.stop();
    flushProgramOutput();

    m_process = nullptr;
    m_killTimer.stop();
    process->deleteLater();

    if (m_stopRequested)
        emit programFinished(tr("Program stopped"), false, true);
    else if (status == QProcess::CrashExit)
        emit programFinished(tr("Program crashed"), false, false);
    else
        emit programFinished(tr("Program exited with code %1").arg(exitCode), exitCode == 0, false);

    setState(State::Idle);
}

void BuildController::stop()
{
    if (!m_process || m_stopRequested)
        return;

    m_stopRequested = true;
    m_process->terminate();        // polite first,
    m_killTimer.start();           // then forced if it does not end in time
}