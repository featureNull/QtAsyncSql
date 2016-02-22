#pragma once

#include <QLoggingCategory>
#include <QAbstractTableModel>

#include "AsyncQueryResult.h"

namespace Database {

class AsyncQuery;

/**
 * @brief The AsyncQueryModel class implementents a QtAbstractTableModel for asynchronous
 * queries.
 * @details The model can used with a QTableView to show the query results.
 */
class AsyncQueryModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	explicit AsyncQueryModel(QObject* parent = 0);
	virtual ~AsyncQueryModel();

	/**
	 * @brief The internal AsynQuery object. Any startExec() function of this
	 * object will update the model content.
	 */
	AsyncQuery *asyncQuery() const;

	/**
	 * @brief Convinience function to start the an query. The model will be updated
	 * when finsihed.
	 */
	void startExec(const QString &query);

	/** @name QAbstractItemModel interface */
	///@{
	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	///@}

protected slots:
	void onExecDone(const Database::AsyncQueryResult &result);

private:
	QLoggingCategory logger;
	AsyncQueryResult _res;
	AsyncQuery *_aQuery;
};

}
