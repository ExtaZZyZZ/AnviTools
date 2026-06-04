#include "MainWindow.h"
#include "ConnectionTab.h"
#include "MonitorTab.h"
#include "ModuleHubTab.h"
#include <QEvent>
#include <QFrame>
#include <QGuiApplication>
#include <QIcon>
#include <QResizeEvent>
#include <QScreen>
#include <QScrollArea>
#include <QStatusBar>
#include <QTabBar>
#include <QWindow>

namespace {

void configureTabWidget(QTabWidget* tabWidget)
{
    tabWidget->setDocumentMode(true);
    tabWidget->setUsesScrollButtons(true);

    QTabBar* bar = tabWidget->tabBar();
    bar->setExpanding(false);
    bar->setElideMode(Qt::ElideNone);
    bar->setUsesScrollButtons(true);
}

QScrollArea* makeScrollablePage(QWidget* page, QWidget* parent)
{
    auto* scrollArea = new QScrollArea(parent);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setWidget(page);
    return scrollArea;
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("AnviTools"));
    setWindowIcon(QIcon(":/resources/app.ico"));
    resize(1380, 860);
    setMinimumSize(900, 620);

    canDriver_ = new CanDriver(this);
    sdoClient_ = new SdoClient(canDriver_, this);
    serialMon_ = new SerialMonitor();

    // CanDriver::frameReceived is emitted from the main thread (QSerialPort::readyRead)
    // so a direct connection to SdoClient is safe.
    connect(canDriver_, &CanDriver::frameReceived,
            sdoClient_,  &SdoClient::onFrameReceived);
    connect(canDriver_, &CanDriver::errorOccurred,
            this,        &MainWindow::onCanError);

    serialThread_ = new QThread(this);
    serialMon_->moveToThread(serialThread_);
    connect(serialThread_, &QThread::finished, serialMon_, &QObject::deleteLater);
    serialThread_->start();

    applyModernStyle();
    setupTabs();
    statusBar()->showMessage(QString::fromUtf8("Отключено"));

    // Apply initial adaptive tab sizing after the window is laid out
    QMetaObject::invokeMethod(this, &MainWindow::updateTabStyle, Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
    serialThread_->quit();
    serialThread_->wait(1000);
}

void MainWindow::setupTabs()
{
    tabs_ = new QTabWidget(this);
    tabs_->setObjectName("primaryTabs");
    configureTabWidget(tabs_);
    setCentralWidget(tabs_);

    connTab_      = new ConnectionTab(canDriver_, serialMon_, this);
    monitorTab_   = new MonitorTab(this);
    moduleHubTab_ = new ModuleHubTab(canDriver_, sdoClient_, this);

    tabs_->addTab(makeScrollablePage(connTab_, tabs_),    QString::fromUtf8("Подключение"));
    tabs_->addTab(makeScrollablePage(monitorTab_, tabs_), QString::fromUtf8("Монитор"));
    tabs_->addTab(moduleHubTab_,                          QString::fromUtf8("Модули"));

    moduleHubTab_->setCanConnected(false);

    connect(connTab_, &ConnectionTab::canConnected,       this, &MainWindow::onCanConnected);
    connect(connTab_, &ConnectionTab::canDisconnected,    this, &MainWindow::onCanDisconnected);
    connect(connTab_, &ConnectionTab::serialConnected,    this, &MainWindow::onSerialConnected);
    connect(connTab_, &ConnectionTab::serialDisconnected, this, &MainWindow::onSerialDisconnected);

    connect(serialMon_, &SerialMonitor::lineReceived,
            monitorTab_, &MonitorTab::appendLine, Qt::QueuedConnection);
    connect(serialMon_, &SerialMonitor::errorOccurred,
            monitorTab_, &MonitorTab::appendLine, Qt::QueuedConnection);

}

void MainWindow::onCanConnected()
{
    statusBar()->showMessage(QString::fromUtf8("CAN подключён"));
    moduleHubTab_->setCanConnected(true);
}

void MainWindow::onCanDisconnected()
{
    statusBar()->showMessage(QString::fromUtf8("CAN отключён"));
    moduleHubTab_->setCanConnected(false);
}

void MainWindow::onSerialConnected()
{
    QString cur = statusBar()->currentMessage();
    if (!cur.contains("Serial"))
        statusBar()->showMessage(cur + QString::fromUtf8(" | Serial подключён"));
}

void MainWindow::onSerialDisconnected()
{
    statusBar()->showMessage(
        statusBar()->currentMessage().remove(QString::fromUtf8(" | Serial подключён")));
}

void MainWindow::onCanError(QString msg)
{
    statusBar()->showMessage(QString::fromUtf8("Ошибка CAN: ") + msg);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    updateTabStyle();
}

void MainWindow::changeEvent(QEvent* event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange)
        updateTabStyle();
}

void MainWindow::updateTabStyle()
{
    if (!tabs_)
        return;

    // Determine effective DPI scale from current screen
    double dpr = 1.0;
    if (QWindow* win = windowHandle())
        if (QScreen* scr = win->screen())
            dpr = scr->devicePixelRatio();

    const int w = width();
    const bool isMaxOrFull = isMaximized() || isFullScreen();

    // Breakpoints (logical pixels):
    //   compact  < 900
    //   medium   900–1199
    //   wide    >= 1200
    // Additionally scale down slightly on hi-DPI screens (dpr >= 2)
    const bool hiDpi   = dpr >= 2.0;
    const bool compact = w < 900;
    const bool wide    = w >= 1200;

    // --- Primary tab bar (Подключение / Монитор / Модули) ---
    QString primaryTabPad, primaryTabMargin, primaryTabFont;
    if (compact) {
        primaryTabPad    = "6px 8px";
        primaryTabMargin = "3px 2px 0 0";
        primaryTabFont   = hiDpi ? "7pt" : "8pt";
    } else if (wide || isMaxOrFull) {
        primaryTabPad    = "9px 12px";
        primaryTabMargin = "4px 3px 0 0";
        primaryTabFont   = hiDpi ? "8pt" : "9pt";
    } else {
        primaryTabPad    = "8px 10px";
        primaryTabMargin = "4px 2px 0 0";
        primaryTabFont   = hiDpi ? "8pt" : "9pt";
    }

    // --- Module sub-tab bar (Автономный модуль / AnviDrive / AnviEnergy) ---
    QString moduleTabPad, moduleTabMargin, moduleTabFont;
    if (compact) {
        moduleTabPad    = "6px 8px";
        moduleTabMargin = "3px 2px 0 0";
        moduleTabFont   = hiDpi ? "7pt" : "8pt";
    } else if (wide || isMaxOrFull) {
        moduleTabPad    = "9px 14px";
        moduleTabMargin = "4px 3px 0 0";
        moduleTabFont   = hiDpi ? "8pt" : "9pt";
    } else {
        moduleTabPad    = "8px 10px";
        moduleTabMargin = "4px 2px 0 0";
        moduleTabFont   = hiDpi ? "8pt" : "9pt";
    }

    // Build dynamic overrides and merge with existing stylesheet
    // We replace only the adaptive rules via setStyleSheet on the tab bars directly
    // so the global palette is untouched.
    const QString primaryCSS = QString(
        "QTabWidget#primaryTabs > QTabBar::tab {"
        "  padding: %1;"
        "  margin: %2;"
        "  font-size: %3;"
        "}"
    ).arg(primaryTabPad, primaryTabMargin, primaryTabFont);

    const QString moduleCSS = QString(
        "QTabWidget#moduleTabs > QTabBar::tab {"
        "  padding: %1;"
        "  margin: %2;"
        "  font-size: %3;"
        "}"
        "QTabWidget#moduleToolTabs > QTabBar::tab {"
        "  padding: %1;"
        "  margin: %2;"
        "  font-size: %3;"
        "}"
    ).arg(moduleTabPad, moduleTabMargin, moduleTabFont);

    tabs_->setStyleSheet(primaryCSS + moduleCSS);
}

void MainWindow::applyModernStyle()
{
    setStyleSheet(
        // ── Base ────────────────────────────────────────────────────────────
        "QWidget {"
        "  background: #FFFFFF;"
        "  color: #9964BB;"
        "  font-family: 'Segoe UI';"
        "  font-size: 10pt;"
        "}"

        // ── Status bar ───────────────────────────────────────────────────────
        "QStatusBar {"
        "  background: #FFFFFF;"
        "  color: #6A2BB0;"
        "  border-top: 1px solid #9964BB;"
        "}"
        "QStatusBar::item { border: none; }"

        // ── Tab widget ───────────────────────────────────────────────────────
        "QTabWidget::pane {"
        "  border: 1px solid #9964BB;"
        "  background: #FFFFFF;"
        "  border-radius: 12px;"
        "  top: -1px;"
        "}"
        "QTabWidget#primaryTabs::pane { background: #FFFFFF; }"
        "QTabBar::tab {"
        "  background: #FFFFFF;"
        "  color: #9964BB;"
        "  padding: 10px 12px;"
        "  margin: 0 2px 0 0;"
        "  border: none;"
        "  border-bottom: 2px solid #9964BB;"
        "  font-weight: 600;"
        "  min-width: 0px;"
        "  white-space: nowrap;"
        "}"
        "QTabBar::tab:selected {"
        "  background: #FFFFFF;"
        "  color: #440486;"
        "  border-bottom: 2px solid #440486;"
        "}"
        "QTabBar::scroller { width: 28px; }"
        "QTabBar QToolButton {"
        "  background: #FFFFFF;"
        "  color: #6A2BB0;"
        "  border: 1px solid #9964BB;"
        "  border-radius: 10px;"
        "  padding: 3px;"
        "  margin-top: 4px;"
        "}"
        "QTabBar QToolButton:hover  { background: #F0E0FB; }"
        "QTabBar QToolButton:disabled { background: #FFFFFF; color: #B0A0C0; }"

        "QTabWidget#moduleTabs QTabBar::tab {"
        "  padding: 8px 10px; margin: 0 2px 0 0; font-size: 9pt; min-width: 100px;"
        "  border-bottom: 2px solid #9964BB;"
        "}"
        "QTabWidget#moduleTabs QTabBar::tab:selected {"
        "  border-bottom: 2px solid #440486;"
        "}"
        "QTabWidget#moduleToolTabs QTabBar::tab {"
        "  padding: 7px 9px; margin: 0 2px 0 0; font-size: 9pt;"
        "  border-bottom: 2px solid #9964BB;"
        "}"
        "QTabWidget#moduleToolTabs QTabBar::tab:selected {"
        "  border-bottom: 2px solid #440486;"
        "}"

        // ── Modules tab: all panes same flat 1px border ───────────────────────
        "QTabWidget#moduleTabs::pane {"
        "  border: 1px solid #9964BB;"
        "  border-radius: 12px;"
        "  background: #FFFFFF;"
        "}"
        "QTabWidget#moduleToolTabs::pane {"
        "  border: 1px solid #9964BB;"
        "  border-radius: 12px;"
        "  background: #FFFFFF;"
        "}"

        // ── GroupBox ─────────────────────────────────────────────────────────
        "QGroupBox {"
        "  background: #F0E0FB;"
        "  border: 1px solid #9964BB;"
        "  border-radius: 12px;"
        "  margin-top: 18px;"
        "  padding: 14px 16px 16px 16px;"
        "  font-size: 10pt; font-weight: 600;"
        "  color: #000000;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; left: 16px; padding: 0 6px;"
        "  color: #9964BB;"
        "}"
        "QGroupBox QWidget { background: transparent; }"
        "QGroupBox QLabel { color: #000000; background: transparent; }"
        "QGroupBox QLineEdit, QGroupBox QComboBox,"
        "QGroupBox QSpinBox, QGroupBox QDoubleSpinBox {"
        "  color: #000000; background: #FFFFFF;"
        "}"
        "QGroupBox QPushButton {"
        "  background: #440486; color: #FFFFFF; border: none;"
        "}"
        "QGroupBox QPushButton:hover  { background: #5a0aaa; }"
        "QGroupBox QPushButton:pressed { background: #F0E0FB; color: #000000; border: 1px solid #000000; }"
        "QGroupBox QPushButton:disabled { background: #D5C8E8; color: #8B7A9E; border: none; }"

        // ── Hero cards ───────────────────────────────────────────────────────
        "QFrame#heroCard, QFrame#placeholderCard {"
        "  background: #F0E0FB;"
        "  border: 1px solid #9964BB;"
        "  border-radius: 12px;"
        "}"
        "QFrame#heroCard QWidget, QFrame#placeholderCard QWidget { background: transparent; }"
        "QFrame#heroCard QLabel, QFrame#placeholderCard QLabel {"
        "  color: #000000; background: transparent;"
        "}"
        "QFrame#heroCard QLabel#onlineBadge, QFrame#placeholderCard QLabel#onlineBadge {"
        "  background: rgba(80,200,140,0.25); color: #1A6640;"
        "}"
        "QFrame#heroCard QLabel#offlineBadge, QFrame#placeholderCard QLabel#offlineBadge {"
        "  background: rgba(220,80,60,0.20); color: #8B2010;"
        "}"
        "QFrame#heroCard QLabel#comingSoonBadge, QFrame#placeholderCard QLabel#comingSoonBadge {"
        "  background: rgba(153,100,187,0.18); color: #440486;"
        "}"
        "QFrame#heroCard QPushButton, QFrame#placeholderCard QPushButton {"
        "  background: #440486; color: #FFFFFF; border: none;"
        "}"
        "QFrame#heroCard QPushButton:hover, QFrame#placeholderCard QPushButton:hover { background: #5a0aaa; }"
        "QFrame#heroCard QPushButton:pressed, QFrame#placeholderCard QPushButton:pressed { background: #F0E0FB; color: #000000; border: 1px solid #000000; }"
        "QFrame#heroCard QPushButton:disabled, QFrame#placeholderCard QPushButton:disabled { background: #D5C8E8; color: #8B7A9E; border: none; }"

        // ── Labels ───────────────────────────────────────────────────────────
        "QLabel#pageTitle {"
        "  font-size: 18pt; font-weight: 700; color: #9964BB;"
        "}"
        "QFrame#heroCard QLabel#pageTitle, QFrame#placeholderCard QLabel#pageTitle {"
        "  color: #9964BB;"
        "}"
        "QLabel#pageSubtitle {"
        "  font-size: 11pt; color: #6A2BB0;"
        "}"
        "QLabel#hintText, QLabel#placeholderNote {"
        "  color: #6A2BB0; font-size: 10pt;"
        "}"

        // ── Module state badges ───────────────────────────────────────────────
        "QLabel#onlineBadge, QLabel#offlineBadge, QLabel#comingSoonBadge {"
        "  padding: 6px 12px; border-radius: 999px; font-weight: 700;"
        "}"
        "QLabel#onlineBadge {"
        "  background: rgba(80, 200, 140, 0.25);"
        "  color: #1A6640;"
        "}"
        "QLabel#offlineBadge {"
        "  background: rgba(220, 80, 60, 0.20);"
        "  color: #8B2010;"
        "}"
        "QLabel#comingSoonBadge {"
        "  background: rgba(153, 100, 187, 0.18);"
        "  color: #440486;"
        "}"

        // ── Buttons ──────────────────────────────────────────────────────────
        "QPushButton {"
        "  background: #440486;"
        "  color: #FFFFFF;"
        "  border: none;"
        "  border-radius: 12px;"
        "  padding: 9px 16px;"
        "  font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "  background: #5a0aaa;"
        "}"
        "QPushButton:pressed {"
        "  background: #F0E0FB;"
        "  color: #000000;"
        "  border: 1px solid #000000;"
        "}"
        "QPushButton:disabled {"
        "  background: #D5C8E8;"
        "  color: #8B7A9E;"
        "  border: none;"
        "}"

        // ── Input fields ─────────────────────────────────────────────────────
        "QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QTextEdit, QTableWidget {"
        "  background: #FFFFFF;"
        "  border: 1px solid #9964BB;"
        "  border-radius: 12px;"
        "  padding: 8px 10px;"
        "  color: #000000;"
        "  selection-background-color: #440486;"
        "  selection-color: #FFFFFF;"
        "}"
        "QLineEdit:focus, QComboBox:focus, QSpinBox:focus,"
        "QDoubleSpinBox:focus, QTextEdit:focus {"
        "  border: 1px solid #440486;"
        "}"

        // ── Table header ─────────────────────────────────────────────────────
        "QHeaderView::section {"
        "  background: rgba(153, 100, 187, 0.15);"
        "  color: #440486;"
        "  border: none; padding: 8px; font-weight: 600;"
        "}"

        // ── Scrollbar ────────────────────────────────────────────────────────
        "QScrollBar:vertical {"
        "  background: transparent; width: 12px; margin: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #9964BB; border-radius: 6px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0px;"
        "}"
    );
}
