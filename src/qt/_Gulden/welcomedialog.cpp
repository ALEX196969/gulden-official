// Copyright (c) 2016-2018 The Gulden developers
// Authored by: Malcolm MacLeod (mmacleod@webmail.co.za)
// Distributed under the GULDEN software license, see the accompanying
// file COPYING

#include "welcomedialog.h"
#include <qt/_Gulden/forms/ui_welcomedialog.h>
#include <unity/appmanager.h>
#include "gui.h"
#include <wallet/wallet.h>
#include "ui_interface.h"
#include <QMovie>
#include <Gulden/mnemonic.h>
#include <vector>
#include "random.h"

static void InitMessage(WelcomeDialog* welcomeDialog, const std::string &message)
{
    QMetaObject::invokeMethod(welcomeDialog, "showMessage",
        Qt::QueuedConnection,
        Q_ARG(QString, QString::fromStdString(message)),
        Q_ARG(int, Qt::AlignBottom|Qt::AlignHCenter),
        Q_ARG(QColor, QColor(255,255,255)));
}

static void ShowProgress(WelcomeDialog* welcomeDialog, const std::string &message, int nProgress)
{
    QMetaObject::invokeMethod(welcomeDialog, "showProgress",
        Qt::QueuedConnection,
        Q_ARG(QString, QString::fromStdString(message)),
        Q_ARG(int, nProgress)
        );
}

WelcomeDialog::WelcomeDialog(const QStyle* _platformStyle, QWidget* parent)
: QFrame( parent )
, ui( new Ui::WelcomeDialog )
, platformStyle( _platformStyle )
, loadingAnimation( NULL )
{
    ui->setupUi(this);

    ui->labelWelcomeDialogLogo->setContentsMargins( 0, 0, 0, 0 );
    ui->labelWelcomeDialogLogoFrame->setContentsMargins( 0, 0, 0, 0 );

    ui->buttonNewWallet->setCursor( Qt::PointingHandCursor );
    ui->buttonRestoreWallet->setCursor( Qt::PointingHandCursor );
    ui->buttonRecoverWallet->setCursor( Qt::PointingHandCursor );
    ui->checkboxConfirmRecoveryPhraseWrittenDown->setCursor( Qt::PointingHandCursor );

    connect(ui->buttonNewWallet, SIGNAL( clicked() ), this, SLOT( newWallet() ) );
    connect(ui->buttonRestoreWallet, SIGNAL( clicked() ), this, SLOT( recoverWallet() ) );
    connect(ui->buttonRecoverWallet, SIGNAL( clicked() ), this, SLOT( processRecoveryPhrase() ) );

    uiInterface.InitMessage.connect(boost::bind(InitMessage, this, _1));
    uiInterface.ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));

    //fixme: (2.1) Remove from future build - place in unit tests instead; we have had no reports of this assert so it seems to be pretty solid.
    if (!testMnemonics())
    {
        assert(0);
    }

    loadingAnimation = new QMovie(":/Gulden/loading_animation");
    if ( loadingAnimation->isValid() )
    {
        loadingAnimation->setScaledSize(QSize(30, 30));
        ui->labelWelcomeScreenAnimation->setMovie(loadingAnimation);
        loadingAnimation->start();
    }

    setAttribute(Qt::WA_DeleteOnClose);
}

void WelcomeDialog::showEvent(QShowEvent *ev)
{
    std::string walletFile = GetArg("-wallet", DEFAULT_WALLET_DAT);
    if (fs::exists(GetDataDir() / walletFile))
    {
        ui->buttonFrame->setVisible(false);
        ui->labelDescribeRecoveryPhrase->setVisible(false);
        ui->labelEnterRecoveryPhrase->setVisible(false);
        ui->welcomeDialogActionStack->setCurrentWidget(ui->welcomeDialogLoadProgressPage);
        Q_EMIT loadWallet();
    }
    else
    {
        ui->buttonFrame->setVisible(false);
        ui->labelDescribeRecoveryPhrase->setVisible(false);
        ui->labelEnterRecoveryPhrase->setVisible(false);
        ui->welcomeDialogActionStack->setCurrentWidget(ui->welcomeDialogCreateWalletPage);
    }

    QFrame::showEvent(ev);

}

void WelcomeDialog::showMessage(const QString& message, [[maybe_unused]] int alignment, [[maybe_unused]] const QColor &color)
{
    ui->labelWelcomeScreenProgressDisplay->setText(message);
}

void WelcomeDialog::showProgress(const QString& message, int nProgress)
{

    ui->labelWelcomeScreenProgressDisplay->setText( QString(message + "%1%").arg(nProgress) );
}



void WelcomeDialog::newWallet()
{
    std::vector<unsigned char> entropy(16);
    GetStrongRandBytes(&entropy[0], 16);
    GuldenAppManager::gApp->setRecoveryPhrase(mnemonicFromEntropy(entropy, entropy.size()*8));

    ui->edittextEnterRecoveryPhrase->setText(QString::fromStdString(GuldenAppManager::gApp->getRecoveryPhrase().c_str()));
    ui->edittextEnterRecoveryPhrase->setReadOnly(true);
    ui->checkboxConfirmRecoveryPhraseWrittenDown->setVisible(true);
    ui->checkboxConfirmRecoveryPhraseWrittenDown->setChecked(false);
    ui->buttonRecoverWallet->setText(tr("Create wallet"));
    ui->buttonFrame->setVisible(true);
    ui->labelDescribeRecoveryPhrase->setVisible(true);
    ui->labelEnterRecoveryPhrase->setVisible(false);
    ui->welcomeDialogActionStack->setCurrentWidget(ui->welcomeDialogRecoverWalletPage);

    GuldenAppManager::gApp->isRecovery = false;
}

void WelcomeDialog::recoverWallet()
{
    ui->checkboxConfirmRecoveryPhraseWrittenDown->setVisible(false);
    ui->checkboxConfirmRecoveryPhraseWrittenDown->setChecked(true);
    ui->buttonRecoverWallet->setText(tr("Recover wallet"));
    ui->buttonFrame->setVisible(true);
    ui->labelDescribeRecoveryPhrase->setVisible(false);
    ui->labelEnterRecoveryPhrase->setVisible(true);
    ui->welcomeDialogActionStack->setCurrentWidget(ui->welcomeDialogRecoverWalletPage);

    GuldenAppManager::gApp->isRecovery = true;
}

void WelcomeDialog::processRecoveryPhrase()
{
    QString recoveryPhrase = ui->edittextEnterRecoveryPhrase->toPlainText();

    //Strip double whitespace and leading/trailing whitespace, newlines, capitals etc.
    recoveryPhrase = recoveryPhrase.simplified().toLower();
    if(ui->checkboxConfirmRecoveryPhraseWrittenDown->isChecked())
    {
        // Refuse to acknowledge an empty recovery phrase, or one that doesn't pass even the most obvious requirement
        if (recoveryPhrase.isEmpty() || recoveryPhrase.length() < 16)
        {
            QString message = tr("Please enter a recovery phrase");
            QDialog* d = GUI::createDialog(this, message, "Okay", "", 400, 180);
            d->exec();
            return;
        }
        else
        {
            // Make sure the recovery phrase is a valid one, if it isn't then we allow the user the option to proceed anyway - but we strongly advise against it.
            if (!checkMnemonic(recoveryPhrase.toStdString().c_str()))
            {
                QString message = tr("The recovery phrase you have entered is not a valid Gulden recovery phrase, if you are sure that this is your phrase then the program can attempt to use it, note that it will be used exactly as is so no double spacing or any other correction will be performed. Making up your own phrase can greatly reduce security, no support can be offered for invalid phrases.");
                QDialog* d = GUI::createDialog(this, message, tr("Cancel"), tr("Proceed with invalid phrase"), 400, 180);

                int result = d->exec();
                if(result == QDialog::Accepted)
                {
                    return;
                }
                //Use unmodified recovery phrase.
                recoveryPhrase = ui->edittextEnterRecoveryPhrase->toPlainText();
            }

            GuldenAppManager::gApp->setRecoveryPhrase(recoveryPhrase.toStdString().c_str());

            //Try burn memory - just in case - not guaranteed to work everywhere but better than doing nothing.
            burnTextEditMemory(ui->edittextEnterRecoveryPhrase);

            ui->labelWelcomeScreenProgressDisplay->setText(tr("Generating wallet"));

            ui->buttonFrame->setVisible(false);
            ui->labelDescribeRecoveryPhrase->setVisible(false);
            ui->labelEnterRecoveryPhrase->setVisible(false);
            ui->welcomeDialogActionStack->setCurrentWidget(ui->welcomeDialogLoadProgressPage);

            Q_EMIT loadWallet();
        }
    }
    else
    {
        QString message = tr("Without your recovery phrase you will lose your Guldens when something goes wrong with your computer.");
        QDialog* d = GUI::createDialog(this, message, tr("I understand"), "", 640, 180);
        d->exec();
    }
}

WelcomeDialog::~WelcomeDialog()
{
    delete ui;
    delete loadingAnimation;
}
