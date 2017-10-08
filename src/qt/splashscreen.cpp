// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// File contains modifications by: The Gulden developers
// All modifications:
// Copyright (c) 2016 The Gulden developers
// Authored by: Malcolm MacLeod (mmacleod@webmail.co.za)
// Distributed under the GULDEN software license, see the accompanying
// file COPYING

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "splashscreen.h"

#include "networkstyle.h"

#include "clientversion.h"
#include "init.h"
#include "util.h"
#include "ui_interface.h"
#include "version.h"

#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QPainter>
#include <QRadialGradient>

SplashScreen::SplashScreen(Qt::WindowFlags f, const NetworkStyle* networkStyle)
    : QWidget(0, f)
    , curAlignment(0)
{

    float fontFactor = 1.0;
    float devicePixelRatio = 1.0;
#if QT_VERSION > 0x050100
    devicePixelRatio = ((QGuiApplication*)QCoreApplication::instance())->devicePixelRatio();
#endif

    QString titleText = tr(PACKAGE_NAME);
    QString versionText = QString("Version %1").arg(QString::fromStdString(FormatFullVersion()));
    QString copyrightText = QString::fromUtf8(CopyrightHolders(strprintf("\xc2\xA9 %u-%u ", 2014, COPYRIGHT_YEAR)).c_str());
    QString titleAddText = networkStyle->getTitleAddText();

    QString font = QApplication::font().toString();

    QSize splashSize(480 * devicePixelRatio, 440 * devicePixelRatio);
    pixmap = QPixmap(splashSize);

#if QT_VERSION > 0x050100

    pixmap.setDevicePixelRatio(devicePixelRatio);
#endif

    QPainter pixPaint(&pixmap);
    pixPaint.setPen(QColor(100, 100, 100));

    QRect rectIcon(QPoint(0, 0), QSize(splashSize.width() / devicePixelRatio, splashSize.height() / devicePixelRatio));

    const QSize requiredSize(splashSize);
    QPixmap icon((devicePixelRatio <= 1.1) ? (IsArgSet("-testnet") ? QPixmap(":/icons/splash_testnet") : QPixmap(":/icons/splash")) : (IsArgSet("-testnet") ? QPixmap(":/icons/splash_testnet_x2") : QPixmap(":/icons/splash_x2")));

    pixPaint.drawPixmap(rectIcon, icon);

    pixPaint.setFont(QFont(font, 33 * fontFactor));
    QFontMetrics fm = pixPaint.fontMetrics();
    int titleTextWidth = fm.width(titleText);
    if (titleTextWidth > 176) {
        fontFactor = fontFactor * 176 / titleTextWidth;
    }

    pixPaint.setFont(QFont(font, 33 * fontFactor));
    /*fm = pixPaint.fontMetrics();
    titleTextWidth  = fm.width(titleText);
    pixPaint.drawText(((pixmap.width()/devicePixelRatio)/2)-(titleTextWidth/2),paddingTop,titleText);

    pixPaint.setFont(QFont(font, 15*fontFactor));


    fm = pixPaint.fontMetrics();
    int versionTextWidth  = fm.width(versionText);
    if(versionTextWidth > titleTextWidth+paddingRight-10) {
        pixPaint.setFont(QFont(font, 10*fontFactor));
        titleVersionVSpace -= 5;
    }
    pixPaint.drawText(((pixmap.width()/devicePixelRatio)/2) - (versionTextWidth/2),paddingTop+titleVersionVSpace,versionText);


    {
        pixPaint.setFont(QFont(font, 10*fontFactor));
    fm = pixPaint.fontMetrics();
    pixPaint.drawText(((pixmap.width()/devicePixelRatio)/2) - (fm.width(copyrightText)/2),paddingTop+titleCopyrightVSpace,copyrightText);
        QRect copyrightRect(x, y, pixmap.width() - x - paddingRight, pixmap.height() - y);
        pixPaint.drawText(copyrightRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, copyrightText);
    }


    if(!titleAddText.isEmpty()) {
        QFont boldFont = QFont(font, 10*fontFactor);
        boldFont.setWeight(QFont::Bold);
        pixPaint.setFont(boldFont);
        fm = pixPaint.fontMetrics();
        int titleAddTextWidth  = fm.width(titleAddText);
        pixPaint.drawText(((pixmap.width()/devicePixelRatio)/2)-titleAddTextWidth/2,15,titleAddText);
    }*/

    pixPaint.end();

    setWindowTitle(titleText + " " + titleAddText);

    QRect r(QPoint(), QSize(pixmap.size().width() / devicePixelRatio, pixmap.size().height() / devicePixelRatio));
    resize(r.size());
    setFixedSize(r.size());
    move(QApplication::desktop()->screenGeometry().center() - r.center());

    subscribeToCoreSignals();
}

SplashScreen::~SplashScreen()
{
    unsubscribeFromCoreSignals();
}

void SplashScreen::slotFinish(QWidget* mainWin)
{
    Q_UNUSED(mainWin);

    /* If the window is minimized, hide() will be ignored. */
    /* Make sure we de-minimize the splashscreen window before hiding */
    if (isMinimized())
        showNormal();
    hide();
}

static void InitMessage(SplashScreen* splash, const std::string& message)
{
    QMetaObject::invokeMethod(splash, "showMessage",
                              Qt::QueuedConnection,
                              Q_ARG(QString, QString::fromStdString(message)),
                              Q_ARG(int, Qt::AlignBottom | Qt::AlignHCenter),
                              Q_ARG(QColor, QColor(255, 255, 255)));
}

static void ShowProgress(SplashScreen* splash, const std::string& title, int nProgress)
{
    InitMessage(splash, title + strprintf("%d", nProgress) + "%");
}

#ifdef ENABLE_WALLET
static void ConnectWallet(SplashScreen* splash, CWallet* wallet)
{
    wallet->ShowProgress.connect(boost::bind(ShowProgress, splash, _1, _2));
}
#endif

void SplashScreen::subscribeToCoreSignals()
{

    uiInterface.InitMessage.connect(boost::bind(InitMessage, this, _1));
    uiInterface.ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));
#ifdef ENABLE_WALLET
    uiInterface.LoadWallet.connect(boost::bind(ConnectWallet, this, _1));
#endif
}

void SplashScreen::unsubscribeFromCoreSignals()
{

    uiInterface.InitMessage.disconnect(boost::bind(InitMessage, this, _1));
    uiInterface.ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
#ifdef ENABLE_WALLET
    if (pwalletMain)
        pwalletMain->ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
#endif
}

void SplashScreen::showMessage(const QString& message, int alignment, const QColor& color)
{
    curMessage = message;
    curAlignment = alignment;
    curColor = color;
    update();
}

void SplashScreen::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.drawPixmap(0, 0, pixmap);
    QRect r = rect().adjusted(5, 5, -5, -5);
    painter.setPen(curColor);
    painter.drawText(r, curAlignment, curMessage);
}

void SplashScreen::closeEvent(QCloseEvent* event)
{
    StartShutdown(); // allows an "emergency" shutdown during startup
    event->ignore();
}
