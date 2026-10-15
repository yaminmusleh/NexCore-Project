#include <QApplication>
#include <QColor>
#include <QFont>
#include <QIcon>
#include <QLinearGradient>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QPixmap>
#include <QRectF>

#include "MainWindow.h"

namespace
{

    // Placeholder application icon drawn in code, so the project needs no
    // image files yet. It can be replaced by a real icon later.
    QIcon makeAppIcon()
    {
        const QColor accent(0x35, 0xc5, 0xb2);
        QIcon icon;

        for (int size : {16, 24, 32, 48, 64, 128, 256})
        {
            QPixmap pixmap(size, size);
            pixmap.fill(Qt::transparent);

            QPainter painter(&pixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::TextAntialiasing);

            QLinearGradient gradient(0, 0, size, size);
            gradient.setColorAt(0.0, QColor(0x2b, 0x2f, 0x38));
            gradient.setColorAt(1.0, QColor(0x14, 0x15, 0x18));

            const qreal inset = size * 0.06;
            painter.setBrush(gradient);
            painter.setPen(QPen(accent, qMax<qreal>(1.0, size / 32.0)));
            painter.drawRoundedRect(
                QRectF(inset, inset, size - 2 * inset, size - 2 * inset),
                size * 0.22, size * 0.22);

            QFont font;
            font.setBold(true);
            font.setPixelSize(static_cast<int>(size * 0.58));
            painter.setFont(font);
            painter.setPen(accent);
            painter.drawText(QRectF(0, 0, size, size), Qt::AlignCenter,
                             QStringLiteral("N"));
            painter.end();

            icon.addPixmap(pixmap);
        }
        return icon;
    }

    void applyDarkTheme(QApplication &app)
    {
        QApplication::setStyle(QStringLiteral("Fusion"));

        const QColor window(0x1e, 0x20, 0x24);
        const QColor base(0x17, 0x18, 0x1c);
        const QColor alternateBase(0x23, 0x26, 0x2c);
        const QColor button(0x2a, 0x2d, 0x34);
        const QColor text(0xd7, 0xda, 0xe0);
        const QColor muted(0x6b, 0x70, 0x80);
        const QColor accent(0x35, 0xc5, 0xb2);
        const QColor onAccent(0x0f, 0x11, 0x14);

        QPalette palette;
        palette.setColor(QPalette::Window, window);
        palette.setColor(QPalette::WindowText, text);
        palette.setColor(QPalette::Base, base);
        palette.setColor(QPalette::AlternateBase, alternateBase);
        palette.setColor(QPalette::ToolTipBase, button);
        palette.setColor(QPalette::ToolTipText, text);
        palette.setColor(QPalette::Text, text);
        palette.setColor(QPalette::PlaceholderText, muted);
        palette.setColor(QPalette::Button, button);
        palette.setColor(QPalette::ButtonText, text);
        palette.setColor(QPalette::BrightText, Qt::white);
        palette.setColor(QPalette::Highlight, accent);
        palette.setColor(QPalette::HighlightedText, onAccent);
        palette.setColor(QPalette::Link, accent);

        palette.setColor(QPalette::Disabled, QPalette::WindowText, muted);
        palette.setColor(QPalette::Disabled, QPalette::Text, muted);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, muted);

        app.setPalette(palette);

        app.setStyleSheet(QStringLiteral(
            "QToolBar { border: none; border-bottom: 1px solid #343842;"
            " padding: 4px; spacing: 4px; }"
            "QStatusBar { border-top: 1px solid #343842; }"
            "QStatusBar::item { border: none; }"
            "QToolTip { color: #d7dae0; background-color: #2a2d34;"
            " border: 1px solid #343842; padding: 4px; }"));
    }

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName(QStringLiteral("NexCore Studio"));
    QApplication::setOrganizationName(QStringLiteral("NexCore"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QApplication::setWindowIcon(makeAppIcon());

    applyDarkTheme(app);

    MainWindow window;
    window.show();

    return app.exec();
}