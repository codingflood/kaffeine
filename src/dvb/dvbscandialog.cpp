/*
 * dvbscandialog.cpp
 *
 * Copyright (C) 2008 Christoph Pfister <christophpfister@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "dvbscandialog.h"
#include "dvbscan.h"
#include "dvbsi.h"
#include "dvbtab.h"
#include "ui_dvbscandialog.h"
#include <QPainter>
#include <KDebug>

DvbGradProgress::DvbGradProgress(QWidget *parent) : QLabel(parent), value(0)
{
	setAlignment(Qt::AlignCenter);
	setFrameShape(Box);
	setText(i18n("0%"));
}

DvbGradProgress::~DvbGradProgress()
{
}

void DvbGradProgress::setValue(int value_)
{
	value = value_;
	Q_ASSERT((value >= 0) && (value <= 100));
	setText(i18n("%1%").arg(value));
	update();
}

void DvbGradProgress::paintEvent(QPaintEvent *event)
{
	{
		QPainter painter(this);
		int border = frameWidth();
		QRect rect(border, border, width() - 2 * border, height() - 2 * border);
		QLinearGradient gradient(rect.topLeft(), rect.topRight());
		gradient.setColorAt(0, Qt::red);
		gradient.setColorAt(1, Qt::green);
		rect.setWidth((rect.width() * value) / 100);
		painter.fillRect(rect, gradient);
	}

	QLabel::paintEvent(event);
}

DvbScanDialog::DvbScanDialog(DvbTab *dvbTab_) : KDialog(dvbTab_), dvbTab(dvbTab_), internal(NULL)
{
	setCaption(i18n("Configure channels"));

	QWidget *widget = new QWidget(this);
	ui = new Ui_DvbScanDialog();
	ui->setupUi(widget);

	QString date = KGlobal::locale()->formatDate(dvbTab->getScanFilesDate(), KLocale::ShortDate);
	ui->scanFilesLabel->setText(i18n("Scan files last updated<br>on %1").arg(date));
	ui->scanButton->setText(i18n("Start scan"));

	channelModel = new DvbChannelModel(this);
	channelModel->setList(dvbTab->getChannelModel()->getList());
	ui->channelView->setModel(channelModel->getProxyModel());
	ui->channelView->enableDeleteAction();

	previewModel = new DvbPreviewChannelModel(this);
	ui->scanResultsView->setModel(previewModel->getProxyModel());
	ui->scanResultsView->setSelectionMode(QAbstractItemView::ExtendedSelection);

	DvbDevice *liveDevice = dvbTab->getLiveDevice();

	if (liveDevice != NULL) {
		ui->sourceList->addItem(i18n("Current transponder"));
		ui->sourceList->setEnabled(false);
		device = liveDevice;
		updateStatus();
		statusTimer.start(1000);
		isLive = true;
	} else {
		QStringList list = dvbTab->getSourceList();

		if (!list.isEmpty()) {
			ui->sourceList->addItems(list);
		} else {
			ui->sourceList->setEnabled(false);
			ui->scanButton->setEnabled(false);
		}

		device = NULL;
		isLive = false;
	}

	connect(ui->deleteAllButton, SIGNAL(clicked(bool)), this, SLOT(deleteAllChannels()));
	connect(ui->scanButton, SIGNAL(clicked(bool)), this, SLOT(scanButtonClicked(bool)));
	connect(ui->providerCBox, SIGNAL(clicked(bool)), ui->providerList, SLOT(setEnabled(bool)));
	connect(ui->filteredButton, SIGNAL(clicked(bool)), this, SLOT(addFilteredChannels()));
	connect(ui->selectedButton, SIGNAL(clicked(bool)), this, SLOT(addSelectedChannels()));
	connect(&statusTimer, SIGNAL(timeout()), this, SLOT(updateStatus()));

	setMainWidget(widget);
}

DvbScanDialog::~DvbScanDialog()
{
	delete internal;
	delete ui;
}

QList<DvbChannel> DvbScanDialog::getChannelList() const
{
	return channelModel->getList();
}

void DvbScanDialog::scanButtonClicked(bool checked)
{
	if (!checked) {
		// stop scan
		ui->scanButton->setText(i18n("Start scan"));
		Q_ASSERT(internal != NULL);
		delete internal;
		internal = NULL;
		return;
	}

	// start scan
	ui->scanButton->setText(i18n("Stop scan"));
	previewModel->setList(QList<DvbPreviewChannel>());

	Q_ASSERT(internal == NULL);

	if (isLive) {
		const DvbChannel *channel = dvbTab->getLiveChannel();
		QList<DvbSharedTransponder> transponderList;
		transponderList.append(channel->getTransponder());
		internal = new DvbScan(channel->source, device, true, transponderList);
	} else {
		// FIXME
	}

	connect(internal, SIGNAL(foundChannels(QList<DvbPreviewChannel>)),
		this, SLOT(foundChannels(QList<DvbPreviewChannel>)));
	connect(internal, SIGNAL(scanFinished()), this, SLOT(scanFinished()));
}

void DvbScanDialog::foundChannels(const QList<DvbPreviewChannel> &channels)
{
	previewModel->appendList(channels);
}

void DvbScanDialog::scanFinished()
{
	ui->scanButton->setChecked(false);
	scanButtonClicked(false);
}

void DvbScanDialog::updateStatus()
{
	ui->signalWidget->setValue(device->getSignal());
	ui->snrWidget->setValue(device->getSnr());
	ui->tuningLed->setState(device->isTuned() ? KLed::On : KLed::Off);
}

void DvbScanDialog::addSelectedChannels()
{
	QList<const DvbPreviewChannel *> channels;

	foreach (QModelIndex index, ui->scanResultsView->selectionModel()->selectedRows()) {
		const DvbPreviewChannel *selectedChannel = previewModel->getChannel(index);

		if (selectedChannel != NULL) {
			channels.append(selectedChannel);
		}
	}

	addUpdateChannels(channels);
}

void DvbScanDialog::addFilteredChannels()
{
	QList<const DvbPreviewChannel *> channels;

	foreach (const DvbPreviewChannel &channel, previewModel->getList()) {
		if (ui->ftaCBox->isChecked()) {
			// only fta channels
			if (channel.scrambled) {
				continue;
			}
		}

		if (ui->radioCBox->isChecked()) {
			if (!ui->tvCBox->isChecked()) {
				// only radio channels
				if (channel.videoPid != -1) {
					continue;
				}
			}
		} else {
			if (ui->tvCBox->isChecked()) {
				// only tv channels
				if (channel.videoPid == -1) {
					continue;
				}
			}
		}

		if (ui->providerCBox->isChecked()) {
			// only channels from a certain provider
			if (channel.provider != ui->providerList->currentText()) {
				continue;
			}
		}

		channels.append(&channel);
	}

	addUpdateChannels(channels);
}

void DvbScanDialog::deleteAllChannels()
{
	channelModel->setList(QList<DvbChannel>());
}

class DvbChannelNumberLess
{
public:
	bool operator()(const DvbChannel &x, const DvbChannel &y) const
	{
		return (x.number < y.number);
	}
};

void DvbScanDialog::addUpdateChannels(const QList<const DvbPreviewChannel *> &channelList)
{
	QList<DvbChannel> channels = channelModel->getList();
	QList<DvbChannel> newChannels;

	foreach (const DvbPreviewChannel *currentChannel, channelList) {
		QList<DvbChannel>::const_iterator it;

		for (it = channels.begin(); it != channels.end(); ++it) {
			// FIXME - algorithmic complexity is quite high
			if ((currentChannel->source == it->source) &&
			    (currentChannel->networkId == it->networkId) &&
			    (currentChannel->transportStreamId == it->transportStreamId) &&
			    (currentChannel->serviceId == it->serviceId)) {
				break;
			}
		}

		DvbChannel channel = *currentChannel;

		if (it != channels.end()) {
			// update channel
			channel.number = it->number;
			channel.audioPid = it->audioPid;
			if (!currentChannel->audioPids.contains(channel.audioPid)) {
				if (!currentChannel->audioPids.isEmpty()) {
					channel.audioPid = currentChannel->audioPids.at(0);
				} else {
					channel.audioPid = -1;
				}
			}

			channelModel->updateChannel(it - channels.begin(), channel);
		} else {
			// add channel
			// number is assigned later
			if (!currentChannel->audioPids.isEmpty()) {
				channel.audioPid = currentChannel->audioPids.at(0);
			}

			newChannels.append(channel);
		}
	}

	if (newChannels.isEmpty()) {
		return;
	}

	qSort(channels.begin(), channels.end(), DvbChannelNumberLess());

	int currentNumber = 1;
	QList<DvbChannel>::const_iterator channelIt = channels.begin();

	for (QList<DvbChannel>::iterator it = newChannels.begin(); it != newChannels.end(); ++it) {
		while ((channelIt != channels.end()) && (currentNumber == channelIt->number)) {
			++channelIt;
			++currentNumber;
		}

		it->number = currentNumber;
		++currentNumber;
	}

	channelModel->appendList(newChannels);
}
