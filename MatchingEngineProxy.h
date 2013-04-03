
#pragma once

#include "MatchingEngineIdentificator.h"
#include "windows.h"
#include "MatchingEngineWrapper.h"
#include <fstream>

using namespace std;


class matchingEngineProxy
{
public:
	static const unsigned int accountLength = 11;
	static const unsigned int messageBufferSize = 4096;
	static const unsigned int answerBufferSize = 512;

private:
	static char* idBuffer;
	static const char* const iniFileName;
	static char* buffer;
	static char* lastErrorDescription;
	static char* messageBuffer;
	static char* answerBuffer;
	static bool simulateMatchingEngine;
	static char* login;
	static char* password;
	static bool connected;
	static bool connectionFailure;
	static bool inDisconnection;
	static char* account;
	static matchingEngineWrapper::handleMatchingEngineUpdateFuncPtr handleOrderUpdate;
	static matchingEngineWrapper::handleMatchingEngineUpdateFuncPtr handleFailedOrderUpdate;
	static matchingEngineWrapper::handleMatchingEngineUpdateFuncPtr handleExecutionUpdate;
	static matchingEngineWrapper::handleMatchingEngineReconnectionFuncPtr recoverFromDisconnection;
	static ofstream* matchingEngineProxyLog;

private:
	matchingEngineProxy() throw();
	matchingEngineProxy(const matchingEngineProxy& ref) throw();
	~matchingEngineProxy() throw();
	static bool initMatchingEngine(const char* configDirectoryPath) throw();
	static bool waitTillConnexion() throw();
	static void handleOngoingConnectionEvent(const int connectionId) throw();
	static void handleFailedConnectionAttemptEvent(const int connectionId) throw();
	static void handleCanceledConnectionAttemptEvent(const int connectionId) throw();
	static void handleSuccessfullConnectionEvent() throw();
	static void handleControlMessage(const int messageId, const int connectionId, const char* message) throw();
	static bool initMatchingEngineData() throw();
	static bool parseOrderTempId(matchingEngineIdentificator& identificator, const bool changeOrder) throw();
	static void log(ostringstream& message) throw();

public:
	static bool init(const char* configDirectoryPath, const char* logDirectoryPath, 
					 matchingEngineWrapper::handleMatchingEngineUpdateFuncPtr handleOrderUpdateFunction, 
					 matchingEngineWrapper::handleMatchingEngineUpdateFuncPtr handleFailedOrderUpdateFunction, 
					 matchingEngineWrapper::handleMatchingEngineUpdateFuncPtr handleExecutionUpdateFunction, 
					 matchingEngineWrapper::handleMatchingEngineReconnectionFuncPtr recoverFromDisconnectionFunction) throw();
	static void destroy() throw();
	static const bool getSimulateMatchingEngine() throw() {return simulateMatchingEngine;}
	static void fetchOrdersUpdates() throw();
	static void fetchFailedOrdersUpdates() throw();
	static void fetchExecutionsUpdates() throw();
	static LRESULT CALLBACK handleMessages(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) throw();
	static void checkMatchingEngineMessages() throw();
	static bool newOrder(char* optionId, const unsigned int qty, const unsigned int limit, const bool buy, matchingEngineIdentificator& identificator, const bool ioc) throw();
	static bool changeOrder(const unsigned int qty, const unsigned int limit, matchingEngineIdentificator& identificator, const bool ioc) throw();
	static bool cancelOrder(matchingEngineIdentificator& identificator) throw();
	static char* getAccount() throw() {return account;}
	static bool queryLastTrade(char* optionId, unsigned int& price, unsigned int& qty) throw();
};


