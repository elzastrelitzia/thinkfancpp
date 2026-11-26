#include "gui/PrivilegedFanController.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

#include "core/ProcFanParser.h"

#include <string>
#include <utility>

namespace thinkfan {

PrivilegedFanController::PrivilegedFanController(QObject* parent)
    : QObject(parent), m_helperPath(resolveHelperPath()) {}

PrivilegedFanController::PrivilegedFanController(const QString& helperPath, QObject* parent)
    : QObject(parent), m_helperPath(helperPath) {}

bool PrivilegedFanController::isAllowedLevel(const QString& level) {
    return formatLevelCommand(level).has_value();
}

std::optional<std::string> PrivilegedFanController::formatLevelCommand(const QString& level) {
    return format_level_command(level.toStdString());
}

void PrivilegedFanController::setLevel(const QString& level) {
    if (!isAllowedLevel(level)) {
        emit commandFailed("setLevel", QString("Level '%1' is not in the allowlist (allowed: auto, 0-7, full-speed, disengaged)").arg(level));
        return;
    }
    sendCommand(QString("--set-level %1").arg(level), QString("set level to %1").arg(level));
}

void PrivilegedFanController::enable() {
    sendCommand("--enable", "enable fan control");
}

void PrivilegedFanController::disable() {
    sendCommand("--disable", "disable fan control");
}

void PrivilegedFanController::check() {
    ensureDaemon();
}

void PrivilegedFanController::cancel() {
    if (m_daemon) {
        m_daemon->kill();
    }
}

QString PrivilegedFanController::helperPath() const {
    return m_helperPath;
}

void PrivilegedFanController::setHelperPath(const QString& path) {
    m_helperPath = path;
}

QString PrivilegedFanController::resolveHelperPath() const {
#if defined(HELPER_INSTALL_PATH)
    QString installPath = QString::fromUtf8(HELPER_INSTALL_PATH) + "/thinkfancpp-helper";
    if (QFile::exists(installPath) && QFileInfo(installPath).isExecutable()) {
        return installPath;
    }
#endif

    QString appDir = QCoreApplication::applicationDirPath();
    QDir appDirObj(appDir);
    QString candidate = QFileInfo(appDirObj, "thinkfancpp-helper").absoluteFilePath();
    if (QFile::exists(candidate) && QFileInfo(candidate).isExecutable()) {
        return candidate;
    }

    QString standardPath = QStandardPaths::findExecutable("thinkfancpp-helper");
    if (!standardPath.isEmpty() && QFileInfo(standardPath).isExecutable()) {
        return standardPath;
    }

    return "thinkfancpp-helper";
}

void PrivilegedFanController::ensureDaemon() {
    if (m_daemon) {
        return;
    }
    if (m_starting) {
        return;
    }

    const QString helper = m_helperPath;
    if (helper.contains('/') || helper.contains('\\')) {
        const QFileInfo info(helper);
        if (!info.exists() || !info.isExecutable()) {
            failPending(QString("Helper not found or not executable: %1").arg(helper));
            return;
        }
    }

    if (QStandardPaths::findExecutable("pkexec").isEmpty()) {
        failPending("pkexec not found; install polkit/pkexec");
        return;
    }

    m_starting = true;
    auto* process = new QProcess(this);
    m_daemon = process;
    process->setProgram("pkexec");
    process->setArguments(QStringList{helper, "--daemon"});

    connect(process, &QProcess::started, this, [this]() {
        m_starting = false;
        if (!m_hasCurrent && !m_queue.isEmpty()) {
            m_current = m_queue.dequeue();
            m_hasCurrent = true;
            m_daemon->write((m_current.line + "\n").toUtf8());
            m_timeoutTimer = new QTimer(this);
            m_timeoutTimer->setSingleShot(true);
            connect(m_timeoutTimer, &QTimer::timeout, this, [this]() {
                if (m_daemon) {
                    m_daemon->kill();
                }
            });
            m_timeoutTimer->start(30000);
        }
    });

    connect(process, &QProcess::readyReadStandardOutput, this, [this]() {
        while (m_daemon && m_daemon->canReadLine()) {
            const QString line = QString::fromUtf8(m_daemon->readLine()).trimmed();
            handleDaemonLine(line);
        }
    });

    connect(process, QOverload<QProcess::ProcessError>::of(&QProcess::errorOccurred),
            this, [this](QProcess::ProcessError error) {
        const QString msg = (error == QProcess::FailedToStart)
            ? QString("Failed to start pkexec: %1").arg(m_daemon ? m_daemon->errorString() : QString())
            : (m_daemon ? m_daemon->errorString() : QString("helper process error"));
        failPending(msg);
        cleanupDaemon();
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        QString err = m_daemon ? QString::fromUtf8(m_daemon->readAllStandardError()).trimmed() : QString();
        if (err.isEmpty()) {
            err = exitStatus == QProcess::CrashExit ? "helper crashed" : QString("helper exited (%1)").arg(exitCode);
        }
        failPending(err);
        cleanupDaemon();
    });

    process->start();
}

void PrivilegedFanController::sendCommand(const QString& line, const QString& actionDescription) {
    ensureDaemon();
    if (!m_daemon) {
        emit commandFailed(actionDescription, "helper not running");
        return;
    }

    if (m_starting || m_hasCurrent) {
        m_queue.enqueue(PendingCommand{line, actionDescription});
        return;
    }

    m_current = PendingCommand{line, actionDescription};
    m_hasCurrent = true;
    m_daemon->write((line + "\n").toUtf8());
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, [this]() {
        if (m_daemon) {
            m_daemon->kill();
        }
    });
    m_timeoutTimer->start(30000);
}

void PrivilegedFanController::handleDaemonLine(const QString& line) {
    if (!m_hasCurrent) {
        return;
    }
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
        m_timeoutTimer->deleteLater();
        m_timeoutTimer = nullptr;
    }

    const QString action = m_current.action;
    m_hasCurrent = false;

    if (line == "ok") {
        emit commandSucceeded(action);
    } else if (line.startsWith("err ")) {
        emit commandFailed(action, line.mid(4));
    } else {
        emit commandFailed(action, QString("unexpected helper response: %1").arg(line));
    }

    if (m_daemon && !m_queue.isEmpty()) {
        m_current = m_queue.dequeue();
        m_hasCurrent = true;
        m_daemon->write((m_current.line + "\n").toUtf8());
        m_timeoutTimer = new QTimer(this);
        m_timeoutTimer->setSingleShot(true);
        connect(m_timeoutTimer, &QTimer::timeout, this, [this]() {
            if (m_daemon) {
                m_daemon->kill();
            }
        });
        m_timeoutTimer->start(30000);
    }
}

void PrivilegedFanController::failPending(const QString& error) {
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
        m_timeoutTimer->deleteLater();
        m_timeoutTimer = nullptr;
    }
    if (m_hasCurrent) {
        emit commandFailed(m_current.action, error);
        m_hasCurrent = false;
    }
    while (!m_queue.isEmpty()) {
        const auto cmd = m_queue.dequeue();
        emit commandFailed(cmd.action, error);
    }
}

void PrivilegedFanController::cleanupDaemon() {
    m_starting = false;
    if (m_daemon) {
        m_daemon->deleteLater();
        m_daemon = nullptr;
    }
}

} // namespace thinkfan
