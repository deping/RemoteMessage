#include <QObject>

// This class is used just to communicate between message thread and UI thread
class UpdateUI : public QObject
{
	Q_OBJECT
signals:
	void addLine(const QString& line);
	void enableButtons(bool val);
};