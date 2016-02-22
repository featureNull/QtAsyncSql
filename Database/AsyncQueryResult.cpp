#include "AsyncQueryResult.h"

#include <QVariant>
#include <QSqlError>

namespace Database {

AsyncQueryResult::AsyncQueryResult()
{
	qRegisterMetaType<AsyncQueryResult>();
}

AsyncQueryResult::~AsyncQueryResult()
{
}

AsyncQueryResult::AsyncQueryResult(const AsyncQueryResult& other)
{
	_data = other._data;
	_record = other._record;
	_error = other._error;
}

AsyncQueryResult& AsyncQueryResult::operator=(const AsyncQueryResult& other)
{
	_data = other._data;
	_record = other._record;
	_error = other._error;
	return *this;
}

QSqlError AsyncQueryResult::error() const
{
	return _error;
}

QSqlRecord AsyncQueryResult::headRecord() const
{
	return _record;
}

int AsyncQueryResult::count() const
{
	return _data.size();
}

QSqlRecord AsyncQueryResult::record(int row) const
{
	QSqlRecord rec = _record;
	if (row >= 0 && row < _data.size()) {
		for (int i = 0; i < _record.count(); i++) {
			rec.setValue(i, _data[row][i]);
		}
	}
	return rec;
}

QVariant AsyncQueryResult::value(int row, int col) const
{
	if (row >= 0 && row < _data.size()) {
		if (col >= 0 && col < _record.count())
			return _data[row][col];
	}
	return QVariant();
}

QVariant AsyncQueryResult::value(int row, const QString &col) const
{
	int colid = _record.indexOf(col);
	return value(row, colid);
}

bool AsyncQueryResult::isValid() const
{
	return !_error.isValid();
}
}	//	namespace
