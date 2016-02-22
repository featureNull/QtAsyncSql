#include "AsynqQueryModel.h"

#include "AsyncQuery.h"


namespace Database {


AsyncQueryModel::AsyncQueryModel(QObject* parent)
	: QAbstractTableModel(parent)
	, logger("Database.AsyncQuerModel")
{
	_aQuery = new AsyncQuery(this);
	connect (_aQuery, SIGNAL(execDone(Database::AsyncQueryResult)),
			 this, SLOT(onExecDone(Database::AsyncQueryResult)));
}

AsyncQueryModel::~AsyncQueryModel()
{

}

AsyncQuery *AsyncQueryModel::asyncQuery() const
{
	return _aQuery;
}

void AsyncQueryModel::startExec(const QString &query)
{
	_aQuery->startExec(query);
}

int AsyncQueryModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return _res.count();

}

int AsyncQueryModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return _res.headRecord().count();
}

QVariant AsyncQueryModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::DisplayRole)
	{
		return _res.value(index.row(), index.column());
	}
	return QVariant();

}

QVariant AsyncQueryModel::headerData(int section,
										   Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole) {
		if (orientation == Qt::Horizontal) {
			return _res.headRecord().fieldName(section);
		}
	}
	return QVariant();
}

void AsyncQueryModel::onExecDone(const Database::AsyncQueryResult &result)
{
	if (!result.isValid()) {
		qCDebug(logger) << "SqlError" << result.error().text();
	}

	beginResetModel();
	_res = result;
	endResetModel();
}

}
