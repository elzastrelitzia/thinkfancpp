#include "gui/FanBackend.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#include <QFile>
#include <QGuiApplication>
#include <QMetaObject>
#include <QMetaType>
#include <QProcess>
#include <QStyleHints>
#include <QThread>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

Q_DECLARE_METATYPE(thinkfan::ThinkfanStatus)

namespace thinkfan {

class FanBackendThinkfanDetectorWorker : public QObject {
    Q_OBJECT

public:
    explicit FanBackendThinkfanDetectorWorker(QObject* parent = nullptr) : QObject(parent) {}

public slots:
    void detect() {
        const ThinkfanStatus status = detect_thinkfan([](const std::vector<std::string>& cmd) {
            CommandResult result;
            if (cmd.empty()) {
                result.error = "empty command";
                return result;
            }

            QStringList args;
            for (std::size_t i = 1; i < cmd.size(); ++i) {
                args << QString::fromStdString(cmd[i]);
            }

            QProcess process;
            process.start(QString::fromStdString(cmd.front()), args);
            if (!process.waitForStarted(1000) || !process.waitForFinished(5000)) {
                result.ok = false;
                result.exit_code = -1;
                result.error = process.errorString().toStdString();
                return result;
            }

            result.ok = process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
            result.exit_code = process.exitCode();
            result.output = QString::fromUtf8(process.readAllStandardOutput()).toStdString();
            result.error = QString::fromUtf8(process.readAllStandardError()).toStdString();
            return result;
        });
        emit finished(status);
    }

signals:
    void finished(const ThinkfanStatus& status);
};

FanBackend::FanBackend(QObject* parent)
    : QObject(parent)
    , m_controller(std::make_unique<PrivilegedFanController>(this))
    , m_thinkfanWorker(std::make_unique<FanBackendThinkfanDetectorWorker>())
    , m_thinkfanThread(new QThread(this)) {
    qRegisterMetaType<ThinkfanStatus>("ThinkfanStatus");
    m_thinkfanWorker->moveToThread(m_thinkfanThread);
    connect(m_thinkfanWorker.get(), &FanBackendThinkfanDetectorWorker::finished, this, &FanBackend::onThinkfanDetected);
    m_thinkfanThread->start();

    connect(m_controller.get(), &PrivilegedFanController::commandSucceeded, this,
            &FanBackend::onCommandSucceeded);
    connect(m_controller.get(), &PrivilegedFanController::commandFailed, this,
            &FanBackend::onCommandFailed);

    m_pollTimer.setInterval(1000);
    connect(&m_pollTimer, &QTimer::timeout, this, &FanBackend::onPollTimer);
    m_pollTimer.start();

    m_thinkfanCheckTimer.setInterval(30000);
    connect(&m_thinkfanCheckTimer, &QTimer::timeout, this, [this]() {
        QMetaObject::invokeMethod(m_thinkfanWorker.get(), "detect", Qt::QueuedConnection);
    });
    m_thinkfanCheckTimer.start();

    setupThemeFollow();
    QMetaObject::invokeMethod(m_thinkfanWorker.get(), "detect", Qt::QueuedConnection);
    refresh();
    m_controller->check();
}

FanBackend::~FanBackend() {
    if (m_thinkfanThread != nullptr) {
        m_thinkfanThread->quit();
        if (!m_thinkfanThread->wait(5000)) {
            m_thinkfanThread->terminate();
            m_thinkfanThread->wait();
        }
    }
    if (m_thinkfanWorker) {
        m_thinkfanWorker->moveToThread(QThread::currentThread());
        m_thinkfanWorker.reset();
    }
}

QString FanBackend::cpuTemp() const { return m_cpuTemp; }
QString FanBackend::fanRpm() const { return m_fanRpm; }
QString FanBackend::fanLevel() const { return m_fanLevel; }
bool FanBackend::fanEnabled() const { return m_fanEnabled; }
bool FanBackend::readable() const { return m_readable; }
QString FanBackend::statusText() const { return m_statusText; }
QString FanBackend::thinkfanWarning() const { return m_thinkfanWarning; }
bool FanBackend::darkMode() const { return m_darkMode; }
QColor FanBackend::themeWindowColor() const { return qmlTokensFor(m_themeMode).window; }
QColor FanBackend::themeTextColor() const { return qmlTokensFor(m_themeMode).text; }
QColor FanBackend::themeMutedColor() const { return qmlTokensFor(m_themeMode).muted; }
QColor FanBackend::themeWarningBackground() const { return qmlTokensFor(m_themeMode).warningBackground; }
QColor FanBackend::themeWarningBorder() const { return qmlTokensFor(m_themeMode).warningBorder; }
QColor FanBackend::themeWarningText() const { return qmlTokensFor(m_themeMode).warningText; }
QColor FanBackend::themeSuccessColor() const { return qmlTokensFor(m_themeMode).success; }
QColor FanBackend::themeErrorColor() const { return qmlTokensFor(m_themeMode).error; }
QColor FanBackend::themeDangerButtonText() const { return qmlTokensFor(m_themeMode).dangerButtonText; }
int FanBackend::manualValue() const { return m_manualValue; }

void FanBackend::setManualValue(int value) {
    const int clamped = std::clamp(value, 0, 7);
    if (m_manualValue == clamped) {
        return;
    }
    m_manualValue = clamped;
    emit manualValueChanged();
}

void FanBackend::setAuto() { m_controller->setLevel(QStringLiteral("auto")); }

void FanBackend::setManual(int value) {
    setManualValue(value);
    m_controller->setLevel(QString::number(m_manualValue));
}

void FanBackend::setFullSpeed() { m_controller->setLevel(QStringLiteral("full-speed")); }
void FanBackend::setDisengaged() { m_controller->setLevel(QStringLiteral("disengaged")); }
void FanBackend::refresh() { onPollTimer(); }
void FanBackend::showStatus(const QString& text) { setStatusText(text); }

void FanBackend::onPollTimer() {
    QFile fanFile(m_procFanPath);
    ProcFanState fanState;
    if (fanFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString content = QString::fromUtf8(fanFile.readAll());
        fanState = parse_proc_fan(content.toStdString());
    } else {
        fanState.readable = false;
        fanState.error = "Cannot open " + m_procFanPath.toStdString();
    }

    const SensorSnapshot sensors = read_sensors(m_procFanPath.toStdString(), m_hwmonRoot.toStdString());
    updateUiFromState(fanState, sensors, m_lastThinkfanStatus);
}

void FanBackend::onThinkfanDetected(const ThinkfanStatus& status) {
    m_lastThinkfanStatus = status;
    updateThinkfanWarning(status);
}

void FanBackend::onCommandSucceeded(const QString& /*action*/) {
    setStatusText(QString());
    refresh();
}

void FanBackend::onCommandFailed(const QString& action, const QString& error) {
    setStatusText(QStringLiteral("Error: %1 failed: %2").arg(action, error));
}

void FanBackend::updateUiFromState(const ProcFanState& fanState, const SensorSnapshot& sensors,
                                   const ThinkfanStatus& thinkfanStatus) {
    const QString cpuTemp = formatTemperature(sensors.cpu_temp_c);
    const QString fanRpm = formatRpm(sensors.fan_rpm);
    const QString fanLevel = formatFanLevel(fanState.level);
    const bool fanEnabled = fanState.enabled;
    const bool readable = fanState.readable;

    if (m_cpuTemp != cpuTemp) {
        m_cpuTemp = cpuTemp;
        emit cpuTempChanged();
    }
    if (m_fanRpm != fanRpm) {
        m_fanRpm = fanRpm;
        emit fanRpmChanged();
    }
    if (m_fanLevel != fanLevel) {
        m_fanLevel = fanLevel;
        emit fanLevelChanged();
    }
    if (m_fanEnabled != fanEnabled) {
        m_fanEnabled = fanEnabled;
        emit fanEnabledChanged();
    }
    if (m_readable != readable) {
        m_readable = readable;
        emit readableChanged();
        if (!readable) {
            setStatusText(QStringLiteral("Fan interface unavailable (/proc/acpi/ibm/fan not readable)"));
        } else if (m_statusText.startsWith(QStringLiteral("Fan interface unavailable"))) {
            setStatusText(QString());
        }
    }

    if (readable) {
        bool ok = false;
        const int level = QString::fromStdString(fanState.level).toInt(&ok);
        if (ok) {
            setManualValue(level);
        }
    }

    updateThinkfanWarning(thinkfanStatus);
}

void FanBackend::updateThinkfanWarning(const ThinkfanStatus& status) {
    QString warning;
    if (status.active) {
        warning = QStringLiteral("⚠ %1 — fan control may conflict")
                      .arg(QString::fromStdString(status.detail));
    }
    if (m_thinkfanWarning != warning) {
        m_thinkfanWarning = warning;
        emit thinkfanWarningChanged();
    }
}

void FanBackend::applyTheme(ThemeMode mode) {
    m_themeMode = mode;
    const bool dark = (mode == ThemeMode::Dark);
    if (m_darkMode != dark) {
        m_darkMode = dark;
        emit darkModeChanged();
    }
    emit themeChanged();
}

void FanBackend::setupThemeFollow() {
    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    if (sessionBus.isConnected()) {
        sessionBus.connect(QStringLiteral("org.freedesktop.portal.Desktop"),
                           QStringLiteral("/org/freedesktop/portal/desktop"),
                           QStringLiteral("org.freedesktop.portal.Settings"),
                           QStringLiteral("SettingChanged"), this,
                           SLOT(onPortalSettingChanged(QString,QString,QDBusVariant)));
    }

    if (QGuiApplication* app = qobject_cast<QGuiApplication*>(QCoreApplication::instance())) {
        if (QStyleHints* hints = app->styleHints()) {
            connect(hints, &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme) {
                applySystemThemeFallback();
            });
        }
    }

    if (sessionBus.isConnected()) {
        QDBusInterface portal(QStringLiteral("org.freedesktop.portal.Desktop"),
                              QStringLiteral("/org/freedesktop/portal/desktop"),
                              QStringLiteral("org.freedesktop.portal.Settings"), sessionBus);
        if (portal.isValid()) {
            const QDBusReply<QVariant> reply = portal.call(
                QStringLiteral("Read"), QStringLiteral("org.freedesktop.appearance"),
                QStringLiteral("color-scheme"));
            if (reply.isValid()) {
                applyPortalThemeValue(reply.value());
                return;
            }
        }
    }

    applySystemThemeFallback();
}

void FanBackend::applyPortalThemeValue(const QVariant& value) {
    const auto scheme = parsePortalColorScheme(value);
    if (scheme.has_value()) {
        applyTheme(themeModeFromPortalValue(*scheme));
        return;
    }
    applySystemThemeFallback();
}

void FanBackend::applySystemThemeFallback() {
    Qt::ColorScheme scheme = Qt::ColorScheme::Unknown;
    if (const QGuiApplication* app = qobject_cast<QGuiApplication*>(QCoreApplication::instance())) {
        if (const QStyleHints* hints = app->styleHints()) {
            scheme = hints->colorScheme();
        }
    }
    applyTheme(themeModeFromColorScheme(scheme));
}

void FanBackend::onPortalSettingChanged(const QString& namespaceName, const QString& key,
                                       const QDBusVariant& value) {
    if (namespaceName == QStringLiteral("org.freedesktop.appearance")
        && key == QStringLiteral("color-scheme")) {
        applyPortalThemeValue(value.variant());
    }
}

QString FanBackend::formatFanLevel(const std::string& level) const {
    if (level.empty() || level == "unknown") {
        return QStringLiteral("—");
    }
    return QString::fromStdString(level);
}

QString FanBackend::formatTemperature(std::optional<double> temp_c) const {
    if (!temp_c.has_value()) {
        return QStringLiteral("unavailable");
    }
    return QString::number(*temp_c, 'f', 1) + QStringLiteral(" °C");
}

QString FanBackend::formatRpm(std::optional<int> rpm) const {
    if (!rpm.has_value()) {
        return QStringLiteral("unavailable");
    }
    return QString::number(*rpm) + QStringLiteral(" RPM");
}

void FanBackend::setStatusText(const QString& text) {
    if (m_statusText == text) {
        return;
    }
    m_statusText = text;
    emit statusTextChanged();
}

} // namespace thinkfan

#include "FanBackend.moc"
