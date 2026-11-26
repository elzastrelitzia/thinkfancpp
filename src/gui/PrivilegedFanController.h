#pragma once

#include <QObject>
#include <QProcess>
#include <QQueue>
#include <QString>
#include <QTimer>
#include <optional>
#include <string>

namespace thinkfan {

class PrivilegedFanController : public QObject {
    Q_OBJECT

public:
    explicit PrivilegedFanController(QObject* parent = nullptr);
    explicit PrivilegedFanController(const QString& helperPath, QObject* parent = nullptr);

    static bool isAllowedLevel(const QString& level);
    static std::optional<std::string> formatLevelCommand(const QString& level);

    Q_SLOT void setLevel(const QString& level);
    Q_SLOT void enable();
    Q_SLOT void disable();
    Q_SLOT void check();
    Q_SLOT void cancel();

    QString helperPath() const;
    void setHelperPath(const QString& path);

signals:
    void commandSucceeded(const QString& action);
    void commandFailed(const QString& action, const QString& error);

private:
    struct PendingCommand {
        QString line;
        QString action;
    };

    QString m_helperPath;
    QString resolveHelperPath() const;

    void ensureDaemon();
    void sendCommand(const QString& line, const QString& actionDescription);
    void handleDaemonLine(const QString& line);
    void failPending(const QString& error);
    void cleanupDaemon();

    QProcess* m_daemon = nullptr;
    QQueue<PendingCommand> m_queue;
    PendingCommand m_current;
    bool m_hasCurrent = false;
    bool m_starting = false;
    QTimer* m_timeoutTimer = nullptr;
};

} // namespace thinkfan
