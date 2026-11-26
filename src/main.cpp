#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

#include "gui/MainWindow.h"

int main(int argc, char* argv[]) {
    Q_INIT_RESOURCE(thinkfancpp);
    QApplication app(argc, argv);
    app.setApplicationName("thinkfancpp");
    app.setApplicationVersion("0.1.0");

    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette dark;
    dark.setColor(QPalette::Window, QColor(45, 45, 45));
    dark.setColor(QPalette::WindowText, Qt::white);
    dark.setColor(QPalette::Base, QColor(30, 30, 30));
    dark.setColor(QPalette::AlternateBase, QColor(45, 45, 45));
    dark.setColor(QPalette::ToolTipBase, Qt::white);
    dark.setColor(QPalette::ToolTipText, Qt::white);
    dark.setColor(QPalette::Text, Qt::white);
    dark.setColor(QPalette::Button, QColor(55, 55, 55));
    dark.setColor(QPalette::ButtonText, Qt::white);
    dark.setColor(QPalette::BrightText, Qt::red);
    dark.setColor(QPalette::Link, QColor(42, 130, 218));
    dark.setColor(QPalette::Highlight, QColor(42, 130, 218));
    dark.setColor(QPalette::HighlightedText, Qt::black);
    dark.setColor(QPalette::PlaceholderText, QColor(160, 160, 160));

    dark.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 127, 127));
    dark.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
    dark.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));
    dark.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
    dark.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(127, 127, 127));

    app.setPalette(dark);

    thinkfan::MainWindow window;
    window.show();

    return app.exec();
}