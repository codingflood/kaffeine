/*
 * dvbchannelui_p.h
 *
 * Copyright (C) 2007-2010 Christoph Pfister <christophpfister@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef DVBCHANNELUI_P_H
#define DVBCHANNELUI_P_H

#include <QPersistentModelIndex>
#include <KDialog>
#include "../sqltablemodel.h"

class QCheckBox;
class QSpinBox;
class KComboBox;
class KLineEdit;
class DvbChannel;

class DvbChannelSqlInterface : public SqlTableModelInterface
{
public:
	DvbChannelSqlInterface(QAbstractItemModel *model,
		QList<QSharedDataPointer<DvbChannel> > *channels_, QObject *parent);
	~DvbChannelSqlInterface();

private:
	int insertFromSqlQuery(const QSqlQuery &query, int index);
	void bindToSqlQuery(QSqlQuery &query, int index, int row) const;

	QList<QSharedDataPointer<DvbChannel> > *channels;
};

class DvbChannelEditor : public KDialog
{
public:
	DvbChannelEditor(QAbstractItemModel *model_, const QModelIndex &modelIndex_,
		QWidget *parent);
	~DvbChannelEditor();

private:
	void accept();

	QAbstractItemModel *model;
	QPersistentModelIndex persistentIndex;
	QExplicitlySharedDataPointer<const DvbChannel> channel;
	KLineEdit *nameEdit;
	QSpinBox *numberBox;
	QSpinBox *networkIdBox;
	QSpinBox *transportStreamIdBox;
	QSpinBox *serviceIdBox;
	KComboBox *audioChannelBox;
	QList<int> audioPids;
	QCheckBox *scrambledBox;
};

#endif /* DVBCHANNELUI_P_H */
