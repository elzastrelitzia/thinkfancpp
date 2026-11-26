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
    executeHelper(QStringList{"--set-level", level}, QString("set level to %1").arg(level));
}

void PrivilegedFanController::enable() {
    executeHelper(QStringList{"--enable"}, "enable fan control");
}

void PrivilegedFanController::disable() {
    executeHelper(QStringList{"--disable"}, "disable fan control");
}

void PrivilegedFanController::cancel() {
    if (m_activeProcess) {
        m_activeProcess->kill();
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

void PrivilegedFanController::executeHelper(const QStringList& arguments, const QString& actionDescription) {
    if (m_activeProcess) {
        emit commandFailed(actionDescription, "Another operation is already in progress");
        return;
    }

    QString helper = m_helperPath;
    if (helper.contains('/') || helper.contains('\\')) {
        QFileInfo info(helper);
        if (!info.exists() || !info.isExecutable()) {
            emit commandFailed(actionDescription, QString("Helper not found or not executable: %1").arg(helper));
            return;
        }
    }

    if (QStandardPaths::findExecutable("pkexec").isEmpty()) {
        emit commandFailed(actionDescription, "pkexec not found; install polkit/pkexec");
        return;
    }

    auto* process = new QProcess(this);
    m_activeProcess = process;
    m_activeActionDescription = actionDescription;

    process->setProgram("pkexec");
    process->setArguments(QStringList{helper} + arguments);

    connect(process, &QProcess::started, this, [this, actionDescription]() {
        m_timeoutTimer = new QTimer(this);
        m_timeoutTimer->setSingleShot(true);
        connect(m_timeoutTimer, &QTimer::timeout, this, [this, actionDescription]() {
            if (m_activeProcess) {
                m_activeProcess->kill();
            }
        });
        m_timeoutTimer->start(30000);
    });

    connect(process, QOverload<QProcess::ProcessError>::of(&QProcess::errorOccurred),
            this, [this, actionDescription](QProcess::ProcessError error) {
        if (!m_activeProcess) {
            return;
        }
        QString errorMsg = m_activeProcess->errorString();
        if (error == QProcess::FailedToStart) {
            errorMsg = QString("Failed to start pkexec for %1: %2").arg(actionDescription, errorMsg);
        }
        emit commandFailed(actionDescription, errorMsg);
        cleanupProcess();
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, actionDescription](int exitCode, QProcess::ExitStatus exitStatus) {
        if (!m_activeProcess) {
            return;
        }

        QString stdoutOutput = QString::fromUtf8(m_activeProcess->readAllStandardOutput()).trimmed();
        QString stderrOutput = QString::fromUtf8(m_activeProcess->readAllStandardError()).trimmed();

        if (exitStatus == QProcess::CrashExit) {
            emit commandFailed(actionDescription, QString("Process crashed during %1: %2").arg(actionDescription, stderrOutput));
        } else if (exitCode != 0) {
            QString error = stderrOutput.isEmpty() ? QString("Exit code %1").arg(exitCode) : stderrOutput;
            emit commandFailed(actionDescription, error);
        } else {
            emit commandSucceeded(actionDescription);
        }

        cleanupProcess();
    });

    process->start();
}

void PrivilegedFanController::cleanupProcess() {
    if (m_activeProcess) {
        m_activeProcess->deleteLater();
        m_activeProcess = nullptr;
        m_activeActionDescription.clear();
    }
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
        m_timeoutTimer->deleteLater();
        m_timeoutTimer = nullptr;
    }
}

} // namespace thinkfan
