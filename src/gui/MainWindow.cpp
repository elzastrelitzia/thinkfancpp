#include "gui/MainWindow.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpacerItem>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QVBoxLayout>
#include <QWidget>
#include <QtConcurrent>

#include <chrono>
#include <functional>
#include <string>
#include <vector>

namespace thinkfan {

// detects thinkfan status in worker thread
class ThinkfanDetectorWorker : public QObject {
    Q_OBJECT
public:
    explicit ThinkfanDetectorWorker(QObject* parent = nullptr) : QObject(parent) {}

public slots:
    void detect() {
        ThinkfanStatus status;
        const std::vector<std::string> units = {
            "thinkfan.service",
            "thinkfan",
            "thinkfan-wakeup"
        };

        for (const auto& unit : units) {
            QProcess process;
            process.start("systemctl", QStringList{"is-active", QString::fromStdString(unit)});
            if (!process.waitForStarted(1000) || !process.waitForFinished(5000)) {
                if (!status.active) {
                    status.unknown = true;
                    status.detail = "systemctl is-active " + unit + " failed: " + process.errorString().toStdString();
                }
                continue;
            }

            QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
            std::string outputStd = output.toStdString();

            if (outputStd == "active") {
                status.active = true;
                status.unknown = false;
                status.detail = unit + " is active";
                emit finished(status);
                return;
            } else if (outputStd == "inactive" || outputStd == "failed") {
                status.detail = unit + " is " + outputStd;
                continue;
            } else {
                if (!status.active) {
                    status.unknown = true;
                    status.detail = "Thinkfan status uncertain: " + unit + " returned '" + outputStd + "'";
                }
            }
        }

        if (!status.active && !status.unknown) {
            status.detail = "No thinkfan units active";
        } else if (!status.active && status.unknown) {
            status.detail = "Thinkfan status uncertain: " + status.detail;
        }

        emit finished(status);
    }

signals:
    void finished(const ThinkfanStatus& status);
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_controller(std::make_unique<PrivilegedFanController>(this))
    , m_thinkfanWorker(new ThinkfanDetectorWorker())
    , m_thinkfanThread(new QThread(this)) {
    setWindowTitle("thinkfancpp");
    resize(500, 400);

    // app and tray icons
    const QIcon appIcon(QStringLiteral(":/icons/logo-sizes/logo_4096x4096.png"));

    QIcon trayIcon;
    const QPixmap traySource(QStringLiteral(":/icons/logo-sizes/logo_1280x1280.png"));
    if (!traySource.isNull()) {
        for (const int size : {16, 20, 22, 24, 32, 48, 64, 128, 256}) {
            trayIcon.addPixmap(traySource.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    } else {
        trayIcon = appIcon;
    }
    setWindowIcon(appIcon);

    setupUi();
    setupConnections();

    // system tray
    m_trayIcon = new QSystemTrayIcon(trayIcon, this);
    m_trayIcon->setToolTip("thinkfancpp");

    m_trayMenu = new QMenu(this);
    m_trayShowAction = m_trayMenu->addAction("Show");
    m_trayQuitAction = m_trayMenu->addAction("Quit");
    m_trayIcon->setContextMenu(m_trayMenu);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
    connect(m_trayShowAction, &QAction::triggered, this, &MainWindow::onTrayShowRequested);
    connect(m_trayQuitAction, &QAction::triggered, this, &MainWindow::onTrayQuitRequested);

    m_trayIcon->show();

    // poll every 2000 ms
    m_pollTimer.setInterval(2000);
    connect(&m_pollTimer, &QTimer::timeout, this, &MainWindow::onPollTimer);
    m_pollTimer.start();

    // async thinkfan detection
    m_thinkfanWorker->moveToThread(m_thinkfanThread);
    connect(m_thinkfanWorker, &ThinkfanDetectorWorker::finished,
            this, &MainWindow::onThinkfanDetected);
    m_thinkfanThread->start();

    m_thinkfanCheckTimer.setInterval(10000);
    m_thinkfanCheckTimer.setSingleShot(false);
    connect(&m_thinkfanCheckTimer, &QTimer::timeout, this, [this]() {
        QMetaObject::invokeMethod(m_thinkfanWorker, "detect", Qt::QueuedConnection);
    });
    m_thinkfanCheckTimer.start();

    QMetaObject::invokeMethod(m_thinkfanWorker, "detect", Qt::QueuedConnection);
    onPollTimer();
}

MainWindow::~MainWindow() {
    if (m_thinkfanThread) {
        m_thinkfanThread->quit();
        m_thinkfanThread->wait(1000);
    }
}

void MainWindow::setupUi() {
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    m_mainLayout = new QVBoxLayout(m_centralWidget);
    m_mainLayout->setSpacing(12);
    m_mainLayout->setContentsMargins(16, 16, 16, 16);

    // status section
    auto* statusGroup = new QWidget(m_centralWidget);
    auto* statusLayout = new QVBoxLayout(statusGroup);
    statusLayout->setSpacing(6);
    statusLayout->setContentsMargins(0, 0, 0, 0);

    m_tempLabel = new QLabel("CPU Temperature: —", statusGroup);
    m_rpmLabel = new QLabel("Fan RPM: —", statusGroup);
    m_levelLabel = new QLabel("Fan Level: —", statusGroup);
    m_enabledLabel = new QLabel("Fan Control: —", statusGroup);

    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoFont.setPointSize(10);
    m_tempLabel->setFont(monoFont);
    m_rpmLabel->setFont(monoFont);
    m_levelLabel->setFont(monoFont);
    m_enabledLabel->setFont(monoFont);

    statusLayout->addWidget(m_tempLabel);
    statusLayout->addWidget(m_rpmLabel);
    statusLayout->addWidget(m_levelLabel);
    statusLayout->addWidget(m_enabledLabel);

    m_mainLayout->addWidget(statusGroup);

    // thinkfan warning banner
    m_thinkfanWarningLabel = new QLabel(m_centralWidget);
    m_thinkfanWarningLabel->setText("⚠ thinkfan.service is active — fan control may conflict");
    m_thinkfanWarningLabel->setStyleSheet(
        "QLabel {"
        "  background-color: #3d3418;"
        "  border: 1px solid #a67c00;"
        "  border-radius: 4px;"
        "  padding: 8px;"
        "  color: #ffd966;"
        "  font-weight: bold;"
        "}"
    );
    m_thinkfanWarningLabel->setWordWrap(true);
    m_thinkfanWarningLabel->setVisible(false);
    m_mainLayout->addWidget(m_thinkfanWarningLabel);

    // mode controls
    auto* controlsGroup = new QWidget(m_centralWidget);
    auto* controlsLayout = new QVBoxLayout(controlsGroup);
    controlsLayout->setSpacing(10);
    controlsLayout->setContentsMargins(0, 8, 0, 0);

    QLabel* modeLabel = new QLabel("Fan Mode", controlsGroup);
    modeLabel->setStyleSheet("font-weight: bold; font-size: 13px;");
    controlsLayout->addWidget(modeLabel);

    m_modeGroup = new QButtonGroup(this);
    m_modeGroup->setExclusive(true);

    m_autoRadio = new QRadioButton("Auto", controlsGroup);
    m_autoRadio->setChecked(true);
    m_modeGroup->addButton(m_autoRadio, 0);
    controlsLayout->addWidget(m_autoRadio);

    auto* manualRow = new QWidget(controlsGroup);
    auto* manualLayout = new QHBoxLayout(manualRow);
    manualLayout->setSpacing(8);
    manualLayout->setContentsMargins(16, 0, 0, 0);

    m_manualRadio = new QRadioButton("Manual:", manualRow);
    m_modeGroup->addButton(m_manualRadio, 1);
    manualLayout->addWidget(m_manualRadio);

    m_manualSlider = new QSlider(Qt::Horizontal, manualRow);
    m_manualSlider->setRange(0, 7);
    m_manualSlider->setValue(4);
    m_manualSlider->setTickPosition(QSlider::TicksBelow);
    m_manualSlider->setTickInterval(1);
    m_manualSlider->setEnabled(false);
    m_manualSlider->setMinimumWidth(200);
    manualLayout->addWidget(m_manualSlider, 1);

    m_manualSliderValueLabel = new QLabel("4", manualRow);
    m_manualSliderValueLabel->setMinimumWidth(24);
    m_manualSliderValueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_manualSliderValueLabel->setFont(monoFont);
    manualLayout->addWidget(m_manualSliderValueLabel);

    controlsLayout->addWidget(manualRow);

    m_maxFullSpeedButton = new QPushButton("Max Full-speed", controlsGroup);
    controlsLayout->addWidget(m_maxFullSpeedButton);

    m_disengagedButton = new QPushButton("Disengaged...", controlsGroup);
    m_disengagedButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #5a1d24;"
        "  border: 1px solid #8b2c36;"
        "  color: #ffb3b8;"
        "  padding: 6px 12px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #6e2430;"
        "}"
    );
    controlsLayout->addWidget(m_disengagedButton);

    auto* fanControlRow = new QWidget(controlsGroup);
    auto* fanControlLayout = new QHBoxLayout(fanControlRow);
    fanControlLayout->setSpacing(8);
    fanControlLayout->setContentsMargins(16, 8, 0, 0);

    m_enableButton = new QPushButton("Enable Fan Control", fanControlRow);
    m_enableButton->setToolTip("Enable fan control (write 'enable' to /proc/acpi/ibm/fan)");
    fanControlLayout->addWidget(m_enableButton);

    m_disableButton = new QPushButton("Disable Fan Control", fanControlRow);
    m_disableButton->setToolTip("Disable fan control (write 'disable' to /proc/acpi/ibm/fan)");
    fanControlLayout->addWidget(m_disableButton);

    controlsLayout->addWidget(fanControlRow);

    m_mainLayout->addWidget(controlsGroup);

    // status/error label
    m_statusLabel = new QLabel(m_centralWidget);
    m_statusLabel->setStyleSheet("color: #ff6b6b; font-weight: bold;");
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setVisible(false);
    m_mainLayout->addWidget(m_statusLabel);

    m_mainLayout->addStretch();
}

void MainWindow::setupConnections() {
    connect(m_autoRadio, &QRadioButton::toggled, this, &MainWindow::onModeAutoToggled);
    connect(m_manualRadio, &QRadioButton::toggled, this, &MainWindow::onModeManualToggled);

    connect(m_manualSlider, &QSlider::sliderPressed, this, [this]() {
        m_sliderPressed = true;
    });
    connect(m_manualSlider, &QSlider::sliderReleased, this, &MainWindow::onManualSliderReleased);
    connect(m_manualSlider, &QSlider::valueChanged, this, &MainWindow::onManualSliderValueChanged);

    connect(m_maxFullSpeedButton, &QPushButton::clicked, this, &MainWindow::onMaxFullSpeedClicked);
    connect(m_disengagedButton, &QPushButton::clicked, this, &MainWindow::onDisengagedClicked);
    connect(m_enableButton, &QPushButton::clicked, this, &MainWindow::onEnableFanControlClicked);
    connect(m_disableButton, &QPushButton::clicked, this, &MainWindow::onDisableFanControlClicked);

    connect(m_controller.get(), &PrivilegedFanController::commandSucceeded,
            this, &MainWindow::onCommandSucceeded);
    connect(m_controller.get(), &PrivilegedFanController::commandFailed,
            this, &MainWindow::onCommandFailed);
}

void MainWindow::onPollTimer() {
    QFile fanFile(m_procFanPath);
    ProcFanState fanState;
    if (fanFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QString::fromUtf8(fanFile.readAll());
        fanState = parse_proc_fan(content.toStdString());
    } else {
        fanState.readable = false;
        fanState.error = "Cannot open " + m_procFanPath.toStdString();
    }

    SensorSnapshot sensors = read_sensors(m_procFanPath.toStdString(), m_hwmonRoot.toStdString());
    updateUiFromState(fanState, sensors, m_lastThinkfanStatus);
    setControlsEnabled(fanState.readable);
}

void MainWindow::onThinkfanDetected(const ThinkfanStatus& status) {
    m_lastThinkfanStatus = status;
    updateThinkfanWarning(status);
}

void MainWindow::updateUiFromState(const ProcFanState& fanState, const SensorSnapshot& sensors, const ThinkfanStatus& thinkfanStatus) {
    m_tempLabel->setText(QString("CPU Temperature: %1").arg(formatTemperature(sensors.cpu_temp_c)));
    m_rpmLabel->setText(QString("Fan RPM: %1").arg(formatRpm(sensors.fan_rpm)));

    QString levelDisplay = formatFanLevel(fanState.level);
    m_levelLabel->setText(QString("Fan Level: %1").arg(levelDisplay));
    if (fanState.readable) {
        m_currentLevelDisplay = levelDisplay;
    }

    if (fanState.readable) {
        m_enabledLabel->setText(QString("Fan Control: %1").arg(fanState.enabled ? "Enabled" : "Disabled"));
        m_enabledLabel->setStyleSheet(fanState.enabled ? "color: #5fd38d;" : "color: #ff6b6b;");
    } else {
        m_enabledLabel->setText("Fan Control: Unavailable");
        m_enabledLabel->setStyleSheet("color: #9a9a9a;");
    }

    updateThinkfanWarning(thinkfanStatus);

    // keep slider in sync when manual mode is active
    if (m_manualRadio->isChecked() && fanState.readable) {
        bool ok = false;
        int level = QString::fromStdString(fanState.level).toInt(&ok);
        if (ok && level >= 0 && level <= 7) {
            m_manualSlider->blockSignals(true);
            m_manualSlider->setValue(level);
            m_manualSliderValueLabel->setText(QString::number(level));
            m_manualSlider->blockSignals(false);
        }
    }
}

void MainWindow::updateThinkfanWarning(const ThinkfanStatus& status) {
    if (status.active) {
        QString detail = QString::fromStdString(status.detail);
        m_thinkfanWarningLabel->setText(QString("⚠ %1 — fan control may conflict").arg(detail));
        m_thinkfanWarningLabel->setVisible(true);
    } else {
        m_thinkfanWarningLabel->setVisible(false);
    }
}

void MainWindow::setControlsEnabled(bool enabled) {
    m_autoRadio->setEnabled(enabled);
    m_manualRadio->setEnabled(enabled);
    m_manualSlider->setEnabled(enabled && m_manualRadio->isChecked());
    m_maxFullSpeedButton->setEnabled(enabled);
    m_disengagedButton->setEnabled(enabled);
    m_enableButton->setEnabled(enabled);
    m_disableButton->setEnabled(enabled);

    if (!enabled) {
        m_statusLabel->setText("Fan interface unavailable (/proc/acpi/ibm/fan not readable)");
        m_statusLabel->setVisible(true);
    } else {
        clearError();
    }
}

void MainWindow::showDisengagedConfirmation() {
    QDialog dialog(this);
    dialog.setWindowTitle("Confirm Disengaged Mode");
    dialog.setModal(true);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->setSpacing(12);
    layout->setContentsMargins(16, 16, 16, 16);

    QLabel* iconLabel = new QLabel(&dialog);
    iconLabel->setPixmap(QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(48, 48));
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    QLabel* textLabel = new QLabel(
        "<b>⚠ Disengaged mode disables all fan control.</b><br/>"
        "The fan will stop completely and will not restart automatically.<br/>"
        "<b>This can overheat and permanently damage hardware.</b>",
        &dialog
    );
    textLabel->setWordWrap(true);
    textLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(textLabel);

    QCheckBox* confirmCheckbox = new QCheckBox(
        "☐ I understand this can overheat and permanently damage hardware",
        &dialog
    );
    confirmCheckbox->setStyleSheet("font-weight: bold; color: #ffb3b8;");
    layout->addWidget(confirmCheckbox);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, &dialog);
    QPushButton* disengageButton = buttonBox->addButton("Disengage", QDialogButtonBox::AcceptRole);
    disengageButton->setEnabled(false);
    disengageButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #5a1d24;"
        "  border: 1px solid #8b2c36;"
        "  color: #ffb3b8;"
        "  padding: 6px 12px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:enabled:hover {"
        "  background-color: #6e2430;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #3a2a2c;"
        "  color: #8a6f72;"
        "}"
    );
    QPushButton* cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
    cancelButton->setDefault(true);
    cancelButton->setFocus();

    layout->addWidget(buttonBox);

    connect(confirmCheckbox, &QCheckBox::toggled, disengageButton, &QPushButton::setEnabled);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        m_controller->setLevel("disengaged");
    }
}

void MainWindow::showError(const QString& message) {
    m_statusLabel->setText(message);
    m_statusLabel->setVisible(true);
}

void MainWindow::clearError() {
    m_statusLabel->clear();
    m_statusLabel->setVisible(false);
}

QString MainWindow::formatFanLevel(const std::string& level) const {
    if (level.empty() || level == "unknown") {
        return "—";
    }
    return QString::fromStdString(level);
}

QString MainWindow::formatTemperature(std::optional<double> temp_c) const {
    if (!temp_c.has_value()) {
        return "unavailable";
    }
    return QString::number(*temp_c, 'f', 1) + " °C";
}

QString MainWindow::formatRpm(std::optional<int> rpm) const {
    if (!rpm.has_value()) {
        return "unavailable";
    }
    return QString::number(*rpm);
}

// --- Slots ---

void MainWindow::onModeAutoToggled(bool checked) {
    if (checked) {
        m_manualSlider->setEnabled(false);
        m_controller->setLevel("auto");
    }
}

void MainWindow::onModeManualToggled(bool checked) {
    m_manualSlider->setEnabled(checked);
    if (checked) {
        m_controller->setLevel(QString::number(m_manualSlider->value()));
    }
}

void MainWindow::onManualSliderReleased() {
    m_sliderPressed = false;
    m_controller->setLevel(QString::number(m_manualSlider->value()));
}

void MainWindow::onManualSliderValueChanged(int value) {
    m_manualSliderValueLabel->setText(QString::number(value));
    // write on release only
}

void MainWindow::onMaxFullSpeedClicked() {
    m_controller->setLevel("full-speed");
}

void MainWindow::onDisengagedClicked() {
    showDisengagedConfirmation();
}

void MainWindow::onCommandSucceeded(const QString& action) {
    clearError();
    onPollTimer();
}

void MainWindow::onCommandFailed(const QString& action, const QString& error) {
    showError(QString("Failed to %1: %2").arg(action, error));
}

void MainWindow::onEnableFanControlClicked() {
    m_controller->enable();
}

void MainWindow::onDisableFanControlClicked() {
    m_controller->disable();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_pendingClose) {
        m_pendingClose = false;
        event->accept();
        return;
    }

    // remind if leaving non-auto level
    if (!m_currentLevelDisplay.compare("Auto", Qt::CaseInsensitive) == 0 &&
        !m_currentLevelDisplay.contains("—", Qt::CaseInsensitive)) {
        m_statusLabel->setText(QString("Fan level left at %1 on exit").arg(m_currentLevelDisplay));
        m_statusLabel->setStyleSheet("color: #ffd966; font-weight: bold;");
        m_statusLabel->setVisible(true);
        // guard against dangling event pointer by re-triggering close
        m_pendingClose = true;
        event->ignore();
        QTimer::singleShot(3000, this, [this]() {
            close();
        });
        return;
    }

    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        m_trayIcon->showMessage("thinkfancpp", "Still running in the system tray.",
                                QSystemTrayIcon::Information, 2000);
        event->ignore();
        return;
    }

    event->accept();
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        onTrayShowRequested();
    }
}

void MainWindow::onTrayShowRequested() {
    showNormal();
    raise();
    activateWindow();
}

void MainWindow::onTrayQuitRequested() {
    m_pendingClose = true;
    if (m_trayIcon) {
        m_trayIcon->hide();
    }
    close();
}

} // namespace thinkfan

#include "MainWindow.moc"