#include <QObject>

class UpdateUI : public QObject
{
	Q_OBJECT
signals:
	void addLine(const QString& line);
};