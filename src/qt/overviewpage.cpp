// Copyright (c) 2011-2014 The Bitcoin Developers
// Copyright (c) 2014-2015 The Dash Developers
// Copyright (c) 2015-2018 The PIVX Developers
// Copyright (c) 2020-2022 The MasterStake Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "bitcoinunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "init.h"
#include "obfuscation.h"
#include "obfuscationconfig.h"
#include "optionsmodel.h"
#include "transactionfilterproxy.h"
#include "transactionrecord.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QSettings>
#include <QTimer>

#define DECORATION_SIZE 48
#define ICON_OFFSET 16
#define NUM_ITEMS 9

extern CWallet* pwalletMain;

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate() : QAbstractItemDelegate(), unit(BitcoinUnits::MASTER)
    {
    }

    inline void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        mainRect.moveLeft(ICON_OFFSET);
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2 * ypad) / 2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top() + ypad, mainRect.width() - xspace - ICON_OFFSET, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top() + ypad + halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();

        // Check transaction status
        int nStatus = index.data(TransactionTableModel::StatusRole).toInt();
        bool fConflicted = false;
        if (nStatus == TransactionStatus::Conflicted || nStatus == TransactionStatus::NotAccepted) {
            fConflicted = true; // Most probably orphaned, but could have other reasons as well
        }
        bool fImmature = false;
        if (nStatus == TransactionStatus::Immature) {
            fImmature = true;
        }

        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = COLOR_BLACK;
        if (value.canConvert<QBrush>()) {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }

        painter->setPen(foreground);
        QRect boundingRect;
        painter->drawText(addressRect, Qt::AlignLeft | Qt::AlignVCenter, address, &boundingRect);

        if (index.data(TransactionTableModel::WatchonlyRole).toBool()) {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            QRect watchonlyRect(boundingRect.right() + 5, mainRect.top() + ypad + halfheight, 16, halfheight);
            iconWatchonly.paint(painter, watchonlyRect);
        }

        if(fConflicted) { // No need to check anything else for conflicted transactions
            foreground = COLOR_CONFLICTED;
        } else if (!confirmed || fImmature) {
            foreground = COLOR_UNCONFIRMED;
        } else if (amount < 0) {
            foreground = COLOR_NEGATIVE;
        } else {
            foreground = COLOR_BLACK;
        }
        painter->setPen(foreground);
        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true, BitcoinUnits::separatorAlways);
        if (!confirmed) {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight | Qt::AlignVCenter, amountText);

        painter->setPen(COLOR_BLACK);
        painter->drawText(amountRect, Qt::AlignLeft | Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;
};
#include "overviewpage.moc"

OverviewPage::OverviewPage(QWidget* parent) : QWidget(parent),
                                              ui(new Ui::OverviewPage),
                                              clientModel(0),
                                              walletModel(0),
                                              currentBalance(-1),
                                              currentUnconfirmedBalance(-1),
                                              currentImmatureBalance(-1),
                                              currentZerocoinBalance(-1),
                                              currentUnconfirmedZerocoinBalance(-1),
                                              currentimmatureZerocoinBalance(-1),
                                              currentWatchOnlyBalance(-1),
                                              currentWatchUnconfBalance(-1),
                                              currentWatchImmatureBalance(-1),
                                              txdelegate(new TxViewDelegate()),
                                              filter(0)
{
    nDisplayUnit = 0; // just make sure it's not unitialized
    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
}

void OverviewPage::handleTransactionClicked(const QModelIndex& index)
{
    if (filter)
        emit transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::getPercentage(CAmount nUnlockedBalance, CAmount nZerocoinBalance, QString& sMASTERPercentage, QString& szMASTERPercentage)
{
    int nPrecision = 2;
    double dzPercentage = 0.0;

    if (nZerocoinBalance <= 0){
        dzPercentage = 0.0;
    }
    else{
        if (nUnlockedBalance <= 0){
            dzPercentage = 100.0;
        }
        else{
            dzPercentage = 100.0 * (double)(nZerocoinBalance / (double)(nZerocoinBalance + nUnlockedBalance));
        }
    }

    double dPercentage = 100.0 - dzPercentage;
    
    szMASTERPercentage = "(" + QLocale(QLocale::system()).toString(dzPercentage, 'f', nPrecision) + " %)";
    sMASTERPercentage = "(" + QLocale(QLocale::system()).toString(dPercentage, 'f', nPrecision) + " %)";
    
}

void OverviewPage::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, 
                              const CAmount& zerocoinBalance, const CAmount& unconfirmedZerocoinBalance, const CAmount& immatureZerocoinBalance,
                              const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance)
{
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentZerocoinBalance = zerocoinBalance;
    currentUnconfirmedZerocoinBalance = unconfirmedZerocoinBalance;
    currentimmatureZerocoinBalance = immatureZerocoinBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;

    CAmount nLockedBalance = 0;
    CAmount nWatchOnlyLockedBalance = 0;
    if (pwalletMain) {
        nLockedBalance = pwalletMain->GetLockedCoins();
        nWatchOnlyLockedBalance = pwalletMain->GetLockedWatchOnlyBalance();
    }
    // MASTER Balance
    CAmount nTotalBalance = balance + unconfirmedBalance;
    CAmount masterAvailableBalance = balance - immatureBalance - nLockedBalance > 0 ? balance - immatureBalance - nLockedBalance : 0;
    CAmount nTotalWatchBalance = watchOnlyBalance + watchUnconfBalance + watchImmatureBalance;    
    CAmount nUnlockedBalance = nTotalBalance - nLockedBalance;

    // zMASTER Balance
    CAmount matureZerocoinBalance = zerocoinBalance - unconfirmedZerocoinBalance - immatureZerocoinBalance;
    // Percentages
    QString szPercentage = "";
    QString sPercentage = "";
    getPercentage(nUnlockedBalance, zerocoinBalance, sPercentage, szPercentage);
    // Combined balances
    CAmount availableTotalBalance = masterAvailableBalance + matureZerocoinBalance;
    CAmount sumTotalBalance = nTotalBalance + zerocoinBalance;

    // MASTER labels
    ui->labelBalance->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, masterAvailableBalance, false, BitcoinUnits::separatorAlways));
    ui->labelUnconfirmed->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, unconfirmedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelImmature->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, immatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelLockedBalance->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, nLockedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelTotal->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, nTotalBalance, false, BitcoinUnits::separatorAlways));

    // Watchonly labels
    ui->labelWatchAvailable->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchOnlyBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchPending->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchUnconfBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchImmature->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchImmatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchLocked->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, nWatchOnlyLockedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchTotal->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, nTotalWatchBalance, false, BitcoinUnits::separatorAlways));

    // zMASTER labels
    ui->labelzBalance->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, zerocoinBalance, false, BitcoinUnits::separatorAlways));
    ui->labelzBalanceUnconfirmed->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, unconfirmedZerocoinBalance, false, BitcoinUnits::separatorAlways));
    ui->labelzBalanceMature->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, matureZerocoinBalance, false, BitcoinUnits::separatorAlways));
    ui->labelzBalanceImmature->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, immatureZerocoinBalance, false, BitcoinUnits::separatorAlways));

    // Combined labels
    ui->labelBalancez->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, availableTotalBalance, false, BitcoinUnits::separatorAlways));
    ui->labelTotalz->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, sumTotalBalance, false, BitcoinUnits::separatorAlways));

    // Percentage labels
    ui->labelMASTERPercent->setText(sPercentage);
    ui->labelzMASTERPercent->setText(szPercentage);

    // Adjust bubble-help according to AutoMint settings
    QString automintHelp = tr("Current percentage of zMASTER.\nIf AutoMint is enabled this percentage will settle around the configured AutoMint percentage (default = 10%).\n");
    bool fEnableZeromint = GetBoolArg("-enablezeromint", false);
    int nZeromintPercentage = GetArg("-zeromintpercentage", 10);
    if (fEnableZeromint) {
        automintHelp += tr("AutoMint is currently enabled and set to ") + QString::number(nZeromintPercentage) + "%.\n";
        automintHelp += tr("To disable AutoMint add 'enablezeromint=0' in masterstake.conf.");
    }
    else {
        automintHelp += tr("AutoMint is currently disabled.\nTo enable AutoMint change 'enablezeromint=0' to 'enablezeromint=1' in masterstake.conf");
    }

    // Only show most balances if they are non-zero for the sake of simplicity
    QSettings settings;
    bool settingShowAllBalances = !settings.value("fHideZeroBalances").toBool();
    bool showSumAvailable = settingShowAllBalances || sumTotalBalance != availableTotalBalance;
    ui->labelBalanceTextz->setVisible(showSumAvailable);
    ui->labelBalancez->setVisible(showSumAvailable);
    bool showMASTERAvailable = settingShowAllBalances || masterAvailableBalance != nTotalBalance;
    bool showWatchOnlyMASTERAvailable = watchOnlyBalance != nTotalWatchBalance;
    bool showMASTERPending = settingShowAllBalances || unconfirmedBalance != 0;
    bool showWatchOnlyMASTERPending = watchUnconfBalance != 0;
    bool showMASTERLocked = settingShowAllBalances || nLockedBalance != 0;
    bool showWatchOnlyMASTERLocked = nWatchOnlyLockedBalance != 0;
    bool showImmature = settingShowAllBalances || immatureBalance != 0;
    bool showWatchOnlyImmature = watchImmatureBalance != 0;
    bool showWatchOnly = nTotalWatchBalance != 0;
    ui->labelBalance->setVisible(showMASTERAvailable || showWatchOnlyMASTERAvailable);
    ui->labelBalanceText->setVisible(showMASTERAvailable || showWatchOnlyMASTERAvailable);
    ui->labelWatchAvailable->setVisible(showMASTERAvailable && showWatchOnly);
    ui->labelUnconfirmed->setVisible(showMASTERPending || showWatchOnlyMASTERPending);
    ui->labelPendingText->setVisible(showMASTERPending || showWatchOnlyMASTERPending);
    ui->labelWatchPending->setVisible(showMASTERPending && showWatchOnly);
    ui->labelLockedBalance->setVisible(showMASTERLocked || showWatchOnlyMASTERLocked);
    ui->labelLockedBalanceText->setVisible(showMASTERLocked || showWatchOnlyMASTERLocked);
    ui->labelWatchLocked->setVisible(showMASTERLocked && showWatchOnly);
    ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature); // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelWatchImmature->setVisible(showImmature && showWatchOnly); // show watch-only immature balance
    bool showzMASTERAvailable = settingShowAllBalances || zerocoinBalance != matureZerocoinBalance;
    bool showzMASTERUnconfirmed = settingShowAllBalances || unconfirmedZerocoinBalance != 0;
    bool showzMASTERImmature = settingShowAllBalances || immatureZerocoinBalance != 0;
    ui->labelzBalanceMature->setVisible(showzMASTERAvailable);
    ui->labelzBalanceMatureText->setVisible(showzMASTERAvailable);
    ui->labelzBalanceUnconfirmed->setVisible(showzMASTERUnconfirmed);
    ui->labelzBalanceUnconfirmedText->setVisible(showzMASTERUnconfirmed);
    ui->labelzBalanceImmature->setVisible(showzMASTERImmature);
    ui->labelzBalanceImmatureText->setVisible(showzMASTERImmature);
    bool showPercentages = ! (zerocoinBalance == 0 && nTotalBalance == 0);
    ui->labelMASTERPercent->setVisible(showPercentages);
    ui->labelzMASTERPercent->setVisible(showPercentages);

    //REMOVE ZEROCOIN
    ui->frame_ZerocoinBalances->setVisible(false);
    //ui->frame_ZerocoinBalances->hide();

    //REMOVE TOP
    ui->frame_Header->setVisible(false);

    static int cachedTxLocks = 0;

    if (cachedTxLocks != nCompleteTXLocks) {
        cachedTxLocks = nCompleteTXLocks;
        ui->listTransactions->update();
    }
}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{
    ui->labelSpendable->setVisible(showWatchOnly);      // show spendable label (only when watch-only is active)
    ui->labelWatchonly->setVisible(showWatchOnly);      // show watch-only label
    ui->labelWatchAvailable->setVisible(showWatchOnly); // show watch-only available balance
    ui->labelWatchPending->setVisible(showWatchOnly);   // show watch-only pending balance
    ui->labelWatchLocked->setVisible(showWatchOnly);     // show watch-only total balance
    ui->labelWatchTotal->setVisible(showWatchOnly);     // show watch-only total balance

    if (!showWatchOnly) {
        ui->labelWatchImmature->hide();
    } else {
        ui->labelBalance->setIndent(20);
        ui->labelUnconfirmed->setIndent(20);
        ui->labelLockedBalance->setIndent(20);
        ui->labelImmature->setIndent(20);
        ui->labelTotal->setIndent(20);
    }
}

void OverviewPage::setClientModel(ClientModel* model)
{
    this->clientModel = model;
    if (model) {
        // Show warning if this is a prerelease version
        connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
        updateAlerts(model->getStatusBarWarnings());
    }
}

void OverviewPage::setWalletModel(WalletModel* model)
{
    this->walletModel = model;
    if (model && model->getOptionsModel()) {
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(),
                   model->getZerocoinBalance(), model->getUnconfirmedZerocoinBalance(), model->getImmatureZerocoinBalance(), 
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance());
        connect(model, SIGNAL(balanceChanged(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)), this, 
                         SLOT(setBalance(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        connect(model->getOptionsModel(), SIGNAL(hideZeroBalancesChanged(bool)), this, SLOT(updateDisplayUnit()));

        updateWatchOnlyLabels(model->haveWatchOnly());
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));
    }

    // update the display unit, to not use the default ("MASTER")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if (walletModel && walletModel->getOptionsModel()) {
        nDisplayUnit = walletModel->getOptionsModel()->getDisplayUnit();
        if (currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance, currentZerocoinBalance, currentUnconfirmedZerocoinBalance, currentimmatureZerocoinBalance,
                currentWatchOnlyBalance, currentWatchUnconfBalance, currentWatchImmatureBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = nDisplayUnit;

        ui->listTransactions->update();
    }
}

void OverviewPage::updateAlerts(const QString& warnings)
{
    this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}
