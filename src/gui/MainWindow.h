#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QVBoxLayout>
#include <QLabel>
#include <QButtonGroup>
#include <QRadioButton>
#include <QSlider>
#include <QPushButton>
#include <QFontDatabase>
#include <QFutureWatcher>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QDBusVariant>
#include <memory>

#include "core/ProcFanParser.h"
#include "core/SensorReader.h"
#include "core/ThinkfanDetector.h"
#include "gui/PrivilegedFanController.h"
#include "gui/Theme.h"

#include <QThread>
#include <QProcess>
#include <QCloseEvent>

namespace thinkfan {

class ThinkfanDetectorWorker;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onPollTimer();
    void onThinkfanDetected(const ThinkfanStatus& status);
    void onModeAutoToggled(bool checked);
    void onModeManualToggled(bool checked);
    void onManualSliderReleased();
    void onManualSliderValueChanged(int value);
    void onMaxFullSpeedClicked();
    void onDisengagedClicked();
    void onCommandSucceeded(const QString& action);
    void onCommandFailed(const QString& action, const QString& error);
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onTrayShowRequested();
    void onTrayQuitRequested();
    void onPortalSettingChanged(const QString& namespaceName, const QString& key, const QDBusVariant& value);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUi();
    void setupConnections();
    void updateUiFromState(const ProcFanState& fanState, const SensorSnapshot& sensors, const ThinkfanStatus& thinkfanStatus);
    void updateThinkfanWarning(const ThinkfanStatus& status);
    void setControlsEnabled(bool enabled);
    void showDisengagedConfirmation();
    void showError(const QString& message);
    void clearError();
    void applyTheme(ThemeMode mode);
    void applySemanticStyles();
    void setupThemeFollow();
    void applyPortalThemeValue(const QVariant& value);
    void applySystemThemeFallback();
    QString formatFanLevel(const std::string& level) const;
    QString formatTemperature(std::optional<double> temp_c) const;
    QString formatRpm(std::optional<int> rpm) const;

    QWidget* m_centralWidget = nullptr;
    QVBoxLayout* m_mainLayout = nullptr;

    QLabel* m_tempLabel = nullptr;
    QLabel* m_rpmLabel = nullptr;
    QLabel* m_levelLabel = nullptr;
    QLabel* m_enabledLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_thinkfanWarningLabel = nullptr;

    QButtonGroup* m_modeGroup = nullptr;
    QRadioButton* m_autoRadio = nullptr;
    QRadioButton* m_manualRadio = nullptr;
    QSlider* m_manualSlider = nullptr;
    QLabel* m_manualSliderValueLabel = nullptr;
    QPushButton* m_maxFullSpeedButton = nullptr;
    QPushButton* m_disengagedButton = nullptr;

    QTimer m_pollTimer;
    QTimer m_thinkfanCheckTimer;
    std::unique_ptr<PrivilegedFanController> m_controller;

    bool m_sliderPressed = false;

    ThinkfanDetectorWorker* m_thinkfanWorker = nullptr;
    QThread* m_thinkfanThread = nullptr;
    ThinkfanStatus m_lastThinkfanStatus;

    QString m_currentLevelDisplay = "Auto";
    bool m_pendingClose = false;
    ThemeMode m_themeMode = ThemeMode::Light;

    QSystemTrayIcon* m_trayIcon = nullptr;
    QMenu* m_trayMenu = nullptr;
    QAction* m_trayShowAction = nullptr;
    QAction* m_trayQuitAction = nullptr;

    const QString m_procFanPath = "/proc/acpi/ibm/fan";
    const QString m_hwmonRoot = "/sys/class/hwmon";
};

} // namespace thinkfan