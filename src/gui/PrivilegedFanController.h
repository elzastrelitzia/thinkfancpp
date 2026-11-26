#pragma once

#include <QObject>
#include <QProcess>
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
    Q_SLOT void cancel();

    QString helperPath() const;
    void setHelperPath(const QString& path);

signals:
    void commandSucceeded(const QString& action);
    void commandFailed(const QString& action, const QString& error);

private:
    QString m_helperPath;
    QString resolveHelperPath() const;

    void executeHelper(const QStringList& arguments, const QString& actionDescription);

    QProcess* m_activeProcess = nullptr;
    QString m_activeActionDescription;
    QTimer* m_timeoutTimer = nullptr;
    void cleanupProcess();
};

} // namespace thinkfan
