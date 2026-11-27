#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QMenu>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSystemTrayIcon>
#include <QUrl>

#include "gui/FanBackend.h"

namespace {

class AppShell : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool quitting READ quitting NOTIFY quittingChanged)

public:
    explicit AppShell(QSystemTrayIcon* tray, QObject* parent = nullptr) : QObject(parent), m_tray(tray) {}

    bool quitting() const { return m_quitting; }

    Q_INVOKABLE bool hideToTrayOnClose() {
        if (m_quitting || m_tray == nullptr || !m_tray->isVisible()) {
            return false;
        }
        auto* window = qobject_cast<QQuickWindow*>(parent());
        if (window == nullptr) {
            return false;
        }
        window->hide();
        m_tray->showMessage(QStringLiteral("thinkfancpp"),
                            QStringLiteral("Still running in the system tray."),
                            QSystemTrayIcon::Information, 2000);
        return true;
    }

    Q_INVOKABLE void requestQuit() {
        if (m_quitting) {
            return;
        }
        m_quitting = true;
        emit quittingChanged();
        if (m_tray != nullptr) {
            m_tray->hide();
        }
        QCoreApplication::quit();
    }

    Q_INVOKABLE void showWindow() {
        auto* window = qobject_cast<QQuickWindow*>(parent());
        if (window == nullptr) {
            return;
        }
        window->show();
        window->raise();
        window->requestActivate();
    }

signals:
    void quittingChanged();

private:
    QSystemTrayIcon* m_tray = nullptr;
    bool m_quitting = false;
};

} // namespace

int main(int argc, char* argv[]) {
    Q_INIT_RESOURCE(thinkfancpp);
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("thinkfancpp"));
    app.setApplicationVersion(QStringLiteral("0.1.0"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/logo-sizes/logo_4096x4096.png")));

    qputenv("QT_QUICK_CONTROLS_MATERIAL_VARIANT", "Dense");
    QQuickStyle::setStyle(QStringLiteral("Material"));

    thinkfan::FanBackend backend;

    QSystemTrayIcon tray(QIcon(QStringLiteral(":/icons/logo-sizes/logo_1280x1280.png")));
    tray.setToolTip(QStringLiteral("thinkfancpp"));
    QMenu trayMenu;
    QAction* showAction = trayMenu.addAction(QStringLiteral("Show"));
    QAction* quitAction = trayMenu.addAction(QStringLiteral("Quit"));
    tray.setContextMenu(&trayMenu);
    tray.show();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);

    AppShell appShell(&tray);
    engine.rootContext()->setContextProperty(QStringLiteral("appShell"), &appShell);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));

    const auto roots = engine.rootObjects();
    if (roots.isEmpty()) {
        return -1;
    }
    auto* window = qobject_cast<QQuickWindow*>(roots.first());
    if (window == nullptr) {
        return -1;
    }

    appShell.setParent(window);

    QObject::connect(showAction, &QAction::triggered, &appShell, &AppShell::showWindow);
    QObject::connect(quitAction, &QAction::triggered, &appShell, &AppShell::requestQuit);
    QObject::connect(&tray, &QSystemTrayIcon::activated, &appShell, [&appShell](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            appShell.showWindow();
        }
    });

    return app.exec();
}

#include "main.moc"