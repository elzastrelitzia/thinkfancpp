#pragma once

#include <QColor>
#include <QDBusVariant>
#include <QObject>
#include <QString>
#include <QTimer>

#include <memory>
#include <optional>

#include "core/ProcFanParser.h"
#include "core/SensorReader.h"
#include "core/ThinkfanDetector.h"
#include "gui/PrivilegedFanController.h"
#include "gui/Theme.h"

class QThread;

namespace thinkfan {

class FanBackendThinkfanDetectorWorker;

class FanBackend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString cpuTemp READ cpuTemp NOTIFY cpuTempChanged)
    Q_PROPERTY(QString fanRpm READ fanRpm NOTIFY fanRpmChanged)
    Q_PROPERTY(QString fanLevel READ fanLevel NOTIFY fanLevelChanged)
    Q_PROPERTY(QString fanMode READ fanMode NOTIFY fanModeChanged)
    Q_PROPERTY(bool fanEnabled READ fanEnabled NOTIFY fanEnabledChanged)
    Q_PROPERTY(bool readable READ readable NOTIFY readableChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString thinkfanWarning READ thinkfanWarning NOTIFY thinkfanWarningChanged)
    Q_PROPERTY(bool darkMode READ darkMode NOTIFY darkModeChanged)
    Q_PROPERTY(QColor themeWindowColor READ themeWindowColor NOTIFY themeChanged)
    Q_PROPERTY(QColor themeTextColor READ themeTextColor NOTIFY themeChanged)
    Q_PROPERTY(QColor themeMutedColor READ themeMutedColor NOTIFY themeChanged)
    Q_PROPERTY(QColor themeWarningBackground READ themeWarningBackground NOTIFY themeChanged)
    Q_PROPERTY(QColor themeWarningBorder READ themeWarningBorder NOTIFY themeChanged)
    Q_PROPERTY(QColor themeWarningText READ themeWarningText NOTIFY themeChanged)
    Q_PROPERTY(QColor themeSuccessColor READ themeSuccessColor NOTIFY themeChanged)
    Q_PROPERTY(QColor themeErrorColor READ themeErrorColor NOTIFY themeChanged)
    Q_PROPERTY(QColor themeDangerButtonText READ themeDangerButtonText NOTIFY themeChanged)
    Q_PROPERTY(int manualValue READ manualValue WRITE setManualValue NOTIFY manualValueChanged)

public:
    explicit FanBackend(QObject* parent = nullptr);
    ~FanBackend() override;

    QString cpuTemp() const;
    QString fanRpm() const;
    QString fanLevel() const;
    QString fanMode() const;
    bool fanEnabled() const;
    bool readable() const;
    QString statusText() const;
    QString thinkfanWarning() const;
    bool darkMode() const;
    QColor themeWindowColor() const;
    QColor themeTextColor() const;
    QColor themeMutedColor() const;
    QColor themeWarningBackground() const;
    QColor themeWarningBorder() const;
    QColor themeWarningText() const;
    QColor themeSuccessColor() const;
    QColor themeErrorColor() const;
    QColor themeDangerButtonText() const;
    int manualValue() const;

    void setManualValue(int value);

    Q_INVOKABLE void setAuto();
    Q_INVOKABLE void setManual(int value);
    Q_INVOKABLE void setFullSpeed();
    Q_INVOKABLE void setDisengaged();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void showStatus(const QString& text);

signals:
    void cpuTempChanged();
    void fanRpmChanged();
    void fanLevelChanged();
    void fanModeChanged();
    void fanEnabledChanged();
    void readableChanged();
    void statusTextChanged();
    void thinkfanWarningChanged();
    void darkModeChanged();
    void themeChanged();
    void manualValueChanged();

private slots:
    void onPollTimer();
    void onThinkfanDetected(const ThinkfanStatus& status);
    void onCommandSucceeded(const QString& action);
    void onCommandFailed(const QString& action, const QString& error);
    void onPortalSettingChanged(const QString& namespaceName, const QString& key, const QDBusVariant& value);

private:
    void updateUiFromState(const ProcFanState& fanState, const SensorSnapshot& sensors,
                           const ThinkfanStatus& thinkfanStatus);
    void updateThinkfanWarning(const ThinkfanStatus& status);
    void applyTheme(ThemeMode mode);
    void setupThemeFollow();
    void applyPortalThemeValue(const QVariant& value);
    void applySystemThemeFallback();
    QString formatFanLevel(const std::string& level) const;
    QString formatFanMode(const std::string& level) const;
    QString formatTemperature(std::optional<double> temp_c) const;
    QString formatRpm(std::optional<int> rpm) const;
    void setStatusText(const QString& text);

    QString m_cpuTemp = QStringLiteral("—");
    QString m_fanRpm = QStringLiteral("—");
    QString m_fanLevel = QStringLiteral("—");
    QString m_fanMode = QStringLiteral("auto");
    bool m_fanEnabled = false;
    bool m_readable = false;
    QString m_statusText;
    QString m_thinkfanWarning;
    bool m_darkMode = false;
    int m_manualValue = 4;

    QTimer m_pollTimer;
    QTimer m_thinkfanCheckTimer;
    std::unique_ptr<PrivilegedFanController> m_controller;

    std::unique_ptr<FanBackendThinkfanDetectorWorker> m_thinkfanWorker;
    QThread* m_thinkfanThread = nullptr;
    ThinkfanStatus m_lastThinkfanStatus;
    ThemeMode m_themeMode = ThemeMode::Light;

    const QString m_procFanPath = QStringLiteral("/proc/acpi/ibm/fan");
    const QString m_hwmonRoot = QStringLiteral("/sys/class/hwmon");
};

} // namespace thinkfan
