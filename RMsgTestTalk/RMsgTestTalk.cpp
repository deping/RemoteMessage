#include <string>
#include <thread>

#include <QApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <codeanalysis\warnings.h>
#pragma warning( push )
#pragma warning ( disable : ALL_CODE_ANALYSIS_WARNINGS )
#include "Session.h"
#include "Server.h"
#include "utility.h"
#pragma warning( pop )

#include "UpdateUI.h"

using namespace RMsg;

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	const int port = 9092;
	QWidget mn;
	auto* pVL = new QVBoxLayout;
	mn.setLayout(pVL);

	auto* pInfo = new QTextEdit;
	pVL->addWidget(pInfo);

	auto* pHL = new QHBoxLayout;
	pVL->addLayout(pHL);
	auto* pListen = new QPushButton;
	pListen->setText("Listen");
	pHL->addWidget(pListen);
	auto* pCon = new QPushButton;
	pCon->setText("Connect to");
	pHL->addWidget(pCon);
	auto* pIP = new QLineEdit;
	pIP->setText("192.168.1.9");
	pHL->addWidget(pIP);

	auto* pHL2 = new QHBoxLayout;
	pVL->addLayout(pHL2);
	auto* pLine = new QLineEdit;
	pHL2->addWidget(pLine);
	auto* pSend = new QPushButton;
	pSend->setText("Send");
	pHL2->addWidget(pSend);

	// Disable to improve performance, Enable to debug communication error.
	EnableDebugInfo(true);
	Session s;
	Server ser;
	UpdateUI* pUui = nullptr; // belong to message thread.
	std::thread lth;
	std::thread cth;

	QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
		// Wait message thread or app.exec() will throw exception at exit.
		s.Stop();
		if (lth.joinable())
			lth.join();
		if (cth.joinable())
			cth.join();
	});

	QObject::connect(pListen, &QPushButton::clicked, [&]() {
		pInfo->append("Listen ...");
		lth = std::thread([&]() {
			ser.Listen(port, s);
			pUui = new UpdateUI;
			QObject::connect(pUui, &UpdateUI::addLine, pInfo, &QTextEdit::append);
			pUui->addLine("Connected.");
			s.RunForever();
			delete pUui;
			pUui = nullptr;
		});
		pListen->setEnabled(false);
		pCon->setEnabled(false);
	});

	QObject::connect(pCon, &QPushButton::clicked, [&]() {
		std::string ipstring = pIP->text().toStdString();
		if (ipstring.empty())
			return;
		pInfo->append("Connect to IP ...");
		pListen->setEnabled(false);
		pCon->setEnabled(false);
		// Use copy capture for ipstring or it will be used out of its lifespan.
		cth = std::thread([ipstring, pInfo, port, &pUui, &s]() {
			pUui = new UpdateUI;
			QObject::connect(pUui, &UpdateUI::addLine, pInfo, &QTextEdit::append);
			s.Connect(ipstring.c_str(), port);
			pUui->addLine("Connected.");
			s.RunForever();
			delete pUui;
			pUui = nullptr;
		});
	});

	s.RegisterMessageHandler(MsgCategory(0), MsgClass(0), [&](std::unique_ptr<const Message> pMsg) {
		std::string payload(pMsg->m_Payload.begin(), pMsg->m_Payload.end());
		QString msg = QString::fromStdString(payload);
		pUui->addLine(msg);
	});

	QObject::connect(pSend, &QPushButton::clicked, [&]() {
		Message* pMsg = s.NewRequestMessage(0, 0);
		std::string str = pLine->text().toStdString();
		std::vector<char> data(str.begin(), str.end());
		pMsg->m_Payload.swap(data);
		s.EnqueueNotice(pMsg);
	});

	s.RegisterDisconnect([&]() {
		pUui->addLine("Disconnected.\n");
		//pListen->setEnabled(true);
		//pCon->setEnabled(true);
	});

	mn.show();
	return app.exec();
}
