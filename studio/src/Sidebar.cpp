#include "Sidebar.h"

#include <QAbstractItemView>
#include <QDir>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTreeView>
#include <QVBoxLayout>

namespace {

constexpr int kRowHeight = 26;
constexpr int kMaxOpenRows = 6;

// The explorer shows names only; theme icons are not guaranteed to be
// readable on the dark background.
class NoIconProvider : public QFileIconProvider
{
public:
    QIcon icon(IconType) const override { return QIcon(); }
    QIcon icon(const QFileInfo &) const override { return QIcon(); }
};

QFileIconProvider *noIcons()
{
    // Outlives the model; the model's worker thread may still call it.
    static NoIconProvider provider;
    return &provider;
}

} // namespace

Sidebar::Sidebar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("sidebar"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *openHeader = new QLabel(tr("OPEN FILES"), this);
    openHeader->setObjectName(QStringLiteral("sectionHeader"));

    m_openList = new QListWidget(this);
    m_openList->setFrameShape(QFrame::NoFrame);
    m_openList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_openList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(m_openList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0)
            emit openFileSelected(row);
    });

    m_explorerHeader = new QLabel(tr("EXPLORER"), this);
    m_explorerHeader->setObjectName(QStringLiteral("sectionHeader"));

    m_explorerStack = new QStackedWidget(this);

    // Page 0: no folder chosen yet.
    auto *emptyPage = new QWidget(this);
    auto *emptyLayout = new QVBoxLayout(emptyPage);
    emptyLayout->setContentsMargins(12, 8, 12, 12);
    emptyLayout->setSpacing(8);
    auto *hint = new QLabel(tr("No folder open."), emptyPage);
    hint->setObjectName(QStringLiteral("sidebarHint"));
    auto *openButton = new QPushButton(tr("Open Folder..."), emptyPage);
    connect(openButton, &QPushButton::clicked, this, &Sidebar::openFolderRequested);
    emptyLayout->addWidget(hint);
    emptyLayout->addWidget(openButton);
    emptyLayout->addStretch(1);
    m_explorerStack->addWidget(emptyPage);

    // Page 1: the folder tree.
    m_model = new QFileSystemModel(this);
    m_model->setIconProvider(noIcons());
    m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    m_model->setNameFilters({QStringLiteral("*.nex")});
    m_model->setNameFilterDisables(false); // hide other files instead of greying them

    m_tree = new QTreeView(this);
    m_tree->setModel(m_model);
    m_tree->setFrameShape(QFrame::NoFrame);
    m_tree->setHeaderHidden(true);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    for (int column = 1; column < m_model->columnCount(); ++column)
        m_tree->hideColumn(column);
    connect(m_tree, &QTreeView::activated, this, [this](const QModelIndex &index) {
        if (!m_model->isDir(index))
            emit fileActivated(m_model->filePath(index));
    });
    m_explorerStack->addWidget(m_tree);

    layout->addWidget(openHeader);
    layout->addWidget(m_openList);
    layout->addWidget(m_explorerHeader);
    layout->addWidget(m_explorerStack, 1);
}

void Sidebar::setOpenFiles(const QList<OpenFile> &files, int current)
{
    // Edit the list in place: this can run while the list itself is emitting
    // currentRowChanged, so it must not rebuild all items.
    const QSignalBlocker blocker(m_openList);

    while (m_openList->count() > files.size())
        delete m_openList->takeItem(m_openList->count() - 1);
    while (m_openList->count() < files.size())
        m_openList->addItem(new QListWidgetItem);

    for (int i = 0; i < files.size(); ++i) {
        const OpenFile &file = files.at(i);
        QListWidgetItem *item = m_openList->item(i);
        item->setText(file.modified ? tr("%1  \u25cf").arg(file.name) : file.name);
        item->setToolTip(file.path.isEmpty() ? tr("Not saved yet")
                                             : QDir::toNativeSeparators(file.path));
        item->setSizeHint(QSize(0, kRowHeight));
    }

    m_openList->setCurrentRow(current);
    m_openList->setFixedHeight(qBound(1, files.size(), kMaxOpenRows) * kRowHeight + 4);
}

void Sidebar::setRootFolder(const QString &path)
{
    if (!QFileInfo(path).isDir())
        return;

    m_root = QDir(path).absolutePath();
    m_model->setRootPath(m_root);
    m_tree->setRootIndex(m_model->index(m_root));

    m_explorerHeader->setText(tr("EXPLORER  \u00b7  %1").arg(QFileInfo(m_root).fileName().toUpper()));
    m_explorerHeader->setToolTip(QDir::toNativeSeparators(m_root));
    m_explorerStack->setCurrentIndex(1);
}

QString Sidebar::rootFolder() const
{
    return m_root;
}

void Sidebar::revealFile(const QString &path)
{
    const QString prefix = m_root.endsWith(QLatin1Char('/')) ? m_root : m_root + QLatin1Char('/');
    if (m_root.isEmpty() || path.isEmpty() || !path.startsWith(prefix)) {
        m_tree->clearSelection();
        m_tree->setCurrentIndex(QModelIndex());
        return;
    }

    const QModelIndex index = m_model->index(path);
    if (!index.isValid())
        return;

    for (QModelIndex parent = index.parent();
         parent.isValid() && parent != m_tree->rootIndex(); parent = parent.parent()) {
        m_tree->expand(parent);
    }
    m_tree->setCurrentIndex(index);
    m_tree->scrollTo(index);
}