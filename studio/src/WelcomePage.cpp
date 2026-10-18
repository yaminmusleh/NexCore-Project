#include "WelcomePage.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

#include "WordmarkData.h"

namespace {

constexpr double kTracking = 40.0;     // extra space between the letters (font units)
constexpr double kArcSpan = 0.95;      // angle the baseline covers (radians)
constexpr double kMaxEdge = 8.0;       // longest straight piece before bending (font units)
constexpr int kCurveSteps = 12;        // pieces per curve segment
constexpr int kMaxRecentShown = 5;

// Moves a point from "flat text" (x along the baseline, y up) onto an arc:
// the baseline becomes a circle of the given radius, and every point keeps its
// height above it. Letters therefore stand radially and fan out towards the top.
QPointF bend(const QPointF &p, double radius)
{
    const double angle = p.x() / radius;
    const double distance = radius + p.y();
    return QPointF(distance * std::sin(angle), -distance * std::cos(angle));
}

// Adds points between two corners so that straight edges also follow the arc.
void addEdge(QPolygonF &polygon, const QPointF &from, const QPointF &to)
{
    const double length = std::hypot(to.x() - from.x(), to.y() - from.y());
    const int steps = std::max(1, static_cast<int>(std::ceil(length / kMaxEdge)));
    for (int step = 1; step <= steps; ++step)
        polygon << from + (to - from) * (static_cast<double>(step) / steps);
}

// The wordmark outlines, laid out along a baseline and bent onto an arc.
QPainterPath buildWordmark()
{
    double total = 0;
    for (const wordmark::Glyph &glyph : wordmark::kGlyphs)
        total += glyph.advance + kTracking;
    total -= kTracking;

    const double radius = total / kArcSpan;

    QPainterPath result;
    result.setFillRule(Qt::OddEvenFill); // counters (the holes in e and o) stay open

    auto flush = [&](QPolygonF &flat) {
        if (flat.size() > 2) {
            QPolygonF bent;
            for (const QPointF &point : std::as_const(flat))
                bent << bend(point, radius);
            result.addPolygon(bent);
            result.closeSubpath();
        }
        flat.clear();
    };

    double pen = -total / 2.0; // centred on the top of the arc
    for (const wordmark::Glyph &glyph : wordmark::kGlyphs) {
        const QStringList tokens = QString::fromLatin1(glyph.path).split(QLatin1Char(' '), Qt::SkipEmptyParts);

        QPolygonF flat;
        QPointF last;
        int i = 0;
        auto next = [&]() { return tokens.at(i++).toDouble(); };
        auto point = [&]() {
            const double x = next();
            const double y = next();
            return QPointF(pen + x, y);
        };

        while (i < tokens.size()) {
            const QString command = tokens.at(i++);
            if (command == QLatin1String("M")) {
                flush(flat);
                last = point();
                flat << last;
            } else if (command == QLatin1String("L")) {
                const QPointF to = point();
                addEdge(flat, last, to);
                last = to;
            } else if (command == QLatin1String("Q")) {
                const QPointF control = point();
                const QPointF to = point();
                for (int step = 1; step <= kCurveSteps; ++step) {
                    const double t = static_cast<double>(step) / kCurveSteps;
                    const double u = 1.0 - t;
                    flat << u * u * last + 2 * u * t * control + t * t * to;
                }
                last = to;
            } else if (command == QLatin1String("C")) {
                const QPointF control1 = point();
                const QPointF control2 = point();
                const QPointF to = point();
                for (int step = 1; step <= kCurveSteps; ++step) {
                    const double t = static_cast<double>(step) / kCurveSteps;
                    const double u = 1.0 - t;
                    flat << u * u * u * last + 3 * u * u * t * control1
                                + 3 * u * t * t * control2 + t * t * t * to;
                }
                last = to;
            } else if (command == QLatin1String("Z")) {
                flush(flat);
            }
        }
        flush(flat);

        pen += glyph.advance + kTracking;
    }
    return result;
}

class WordmarkWidget : public QWidget
{
public:
    explicit WordmarkWidget(QWidget *parent = nullptr)
        : QWidget(parent), m_path(buildWordmark())
    {
        setMinimumHeight(110);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    QSize sizeHint() const override { return QSize(640, 260); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const QRectF bounds = m_path.boundingRect();
        const QRectF area = QRectF(rect()).adjusted(16, 12, -16, -12);
        // Never larger than a comfortable size, however big the window is.
        const double scale = std::min({area.width() / bounds.width(),
                                       area.height() / bounds.height(),
                                       760.0 / bounds.width()});

        painter.translate(area.center());
        painter.scale(scale, scale);
        painter.translate(-bounds.center());

        // A soft glow behind the letters, then the letters.
        painter.setPen(QPen(QColor(0x35, 0xc5, 0xb2, 36), 64, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(m_path);

        QLinearGradient gradient(bounds.topLeft(), bounds.bottomLeft());
        gradient.setColorAt(0.0, QColor(0x7f, 0xf0, 0xde));
        gradient.setColorAt(1.0, QColor(0x35, 0xc5, 0xb2));
        painter.setPen(Qt::NoPen);
        painter.setBrush(gradient);
        painter.drawPath(m_path);
    }

private:
    QPainterPath m_path;
};

QPushButton *makeButton(const QString &text, const char *objectName, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(QLatin1String(objectName));
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

} // namespace

QUrl WelcomePage::websiteUrl()
{
    return QUrl(QStringLiteral("https://nexcore-website-landing-page.onrender.com/"));
}

WelcomePage::WelcomePage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("welcomePage"));
    setAttribute(Qt::WA_StyledBackground, true);

    // Everything sits in a scroll area: in a short window the page scrolls
    // instead of squeezing its parts on top of each other.
    auto *content = new QWidget;
    content->setObjectName(QStringLiteral("welcomeContent"));

    auto *wordmark = new WordmarkWidget(content);

    auto *tagline = new QLabel(tr("A native editor for the NexCore language"), content);
    tagline->setObjectName(QStringLiteral("welcomeTagline"));
    tagline->setAlignment(Qt::AlignHCenter);

    // Start actions: a button with its shortcut at the right.
    auto *column = new QWidget(content);
    column->setFixedWidth(360);
    auto *columnLayout = new QVBoxLayout(column);
    columnLayout->setContentsMargins(0, 0, 0, 0);
    columnLayout->setSpacing(2);

    auto addAction = [&](const QString &text, QLabel *&hint) {
        auto *row = new QWidget(column);
        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 8, 0);
        QPushButton *button = makeButton(text, "welcomeButton", row);
        hint = new QLabel(row);
        hint->setObjectName(QStringLiteral("welcomeHint"));
        layout->addWidget(button, 1);
        layout->addWidget(hint);
        columnLayout->addWidget(row);
        return button;
    };

    QPushButton *newButton = addAction(tr("New File"), m_newHint);
    QPushButton *openButton = addAction(tr("Open File..."), m_openHint);
    QPushButton *folderButton = addAction(tr("Open Folder..."), m_folderHint);
    connect(newButton, &QPushButton::clicked, this, &WelcomePage::newFileRequested);
    connect(openButton, &QPushButton::clicked, this, &WelcomePage::openFileRequested);
    connect(folderButton, &QPushButton::clicked, this, &WelcomePage::openFolderRequested);

    // Recent files.
    m_recentSection = new QWidget(column);
    auto *recentOuter = new QVBoxLayout(m_recentSection);
    recentOuter->setContentsMargins(0, 14, 0, 0);
    recentOuter->setSpacing(0);
    auto *recentHeader = new QLabel(tr("RECENT"), m_recentSection);
    recentHeader->setObjectName(QStringLiteral("sectionHeader"));
    recentOuter->addWidget(recentHeader);
    m_recentLayout = new QVBoxLayout;
    m_recentLayout->setContentsMargins(0, 0, 0, 0);
    m_recentLayout->setSpacing(0);
    recentOuter->addLayout(m_recentLayout);
    columnLayout->addWidget(m_recentSection);
    m_recentSection->hide();

    auto *link = new QLabel(
        QStringLiteral("<a href=\"%1\" style=\"text-decoration:none\">%2</a>")
            .arg(websiteUrl().toString(), tr("Visit the NexCore website")),
        content);
    link->setObjectName(QStringLiteral("welcomeLink"));
    link->setTextFormat(Qt::RichText);
    link->setAlignment(Qt::AlignHCenter);
    link->setToolTip(websiteUrl().toString());
    // Always opens the fixed address, whatever the label reports.
    connect(link, &QLabel::linkActivated, this, [] { QDesktopServices::openUrl(websiteUrl()); });

    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(24, 20, 24, 16);
    layout->setSpacing(0);
    layout->addStretch(1);
    layout->addWidget(wordmark, 5);
    layout->addWidget(tagline);
    layout->addSpacing(26);
    layout->addWidget(column, 0, Qt::AlignHCenter);
    layout->addStretch(2);
    layout->addWidget(link);

    auto *scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("welcomeScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->viewport()->setAutoFillBackground(false);
    scroll->setWidget(content);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll);
}

void WelcomePage::setShortcutHints(const QString &newFile, const QString &openFile,
                                   const QString &openFolder)
{
    m_newHint->setText(newFile);
    m_openHint->setText(openFile);
    m_folderHint->setText(openFolder);
}

void WelcomePage::setRecentFiles(const QStringList &paths)
{
    // This can be called from a recent-file button's own click, so the old
    // buttons are only scheduled for deletion.
    while (QLayoutItem *item = m_recentLayout->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->hide();
            widget->deleteLater();
        }
        delete item;
    }

    const int count = std::min<int>(paths.size(), kMaxRecentShown);
    for (int i = 0; i < count; ++i) {
        const QString path = paths.at(i);
        QString name = QFileInfo(path).fileName();
        name.replace(QLatin1Char('&'), QStringLiteral("&&")); // not a mnemonic

        QPushButton *button = makeButton(name, "welcomeRecent", m_recentSection);
        button->setToolTip(QDir::toNativeSeparators(path));
        connect(button, &QPushButton::clicked, this, [this, path] { emit recentFileRequested(path); });
        m_recentLayout->addWidget(button);
    }
    m_recentSection->setVisible(count > 0);
}