
#include "MarketFeedListener.h"
#include "LinkDoorman.h"
#include "MarketSpecific.h"
#include <Timer.h>
#include <TimeUtils.h>
#include <Profiler.h>
#include <ConfigParser.h>
#include <LogUtils.h>
#include <iostream>
#include <sstream>
#include <ctime>
#include "MarketFeedSimulator.h"

using namespace std;

typedef VOID (*CtrlFuncPtr)(int, char*);
typedef VOID (*DataFuncPtr)(char*, long);

BOOL __declspec(dllimport) WINAPI FastMarketInit2(CtrlFuncPtr, DataFuncPtr);
BOOL __declspec(dllimport) WINAPI StartFastMarket(char*, char*);


marketFeedWrapper::handleOptionDataFuncPtr marketFeedListener::handleOptionDataFunc = 0;
marketFeedWrapper::handleFutureDataFuncPtr marketFeedListener::handleFutureDataFunc = 0;
marketFeedWrapper::handleControlFuncPtr marketFeedListener::handleControlFunc = 0;
unsigned int* marketFeedListener::prices = 0;
unsigned int* marketFeedListener::qties = 0;
unsigned int marketFeedListener::lastPrice = 0;
unsigned int marketFeedListener::lastQty = 0;
ofstream* marketFeedListener::marketFeedListenerLog = 0;
char* marketFeedListener::marketFeedLogin = 0;
char* marketFeedListener::marketFeedPassword = 0;
char* marketFeedListener::qtyBuffer = 0;
char* marketFeedListener::priceBuffer = 0;
bool marketFeedListener::simulateMarketFeed = false;
CRITICAL_SECTION marketFeedListener::marketFeedListenerLogCS;


bool marketFeedListener::init(const char* configDirectoryPath, const char* logDirectoryPath,
							  marketFeedWrapper::handleControlFuncPtr handleControl,
							  marketFeedWrapper::handleOptionDataFuncPtr handleOptionData, 
							  marketFeedWrapper::handleFutureDataFuncPtr handleFutureData) throw()
{
	InitializeCriticalSection(&marketFeedListenerLogCS);
	char month[3], day[3], year[3];
	timeUtils::getCurrentDate(month, day, year);
	ostringstream oss;
	oss << logDirectoryPath << "\\" << year << "-" << month << "-" << day << "-marketFeedLog.txt";
	marketFeedListenerLog = new ofstream(oss.str().c_str(), std::ios_base::app);
	if(!marketFeedListenerLog || marketFeedListenerLog->fail() || marketFeedListenerLog->bad()) {
		logUtils::printInitializationError("marketFeedListener init error", __FILE__, __LINE__);
		return false;
	}
	prices = new unsigned int[marketSpecific::optionBookDepth];
	qties = new unsigned int[marketSpecific::optionBookDepth];
	qtyBuffer = new char[10];
	priceBuffer = new char[8];
	if(!prices || !qties || !qtyBuffer || !priceBuffer) {
		logUtils::printInitializationError("Memory allocation error", __FILE__, __LINE__);
		return false;
	}
	memset(qtyBuffer, 0, 10);
	memset(priceBuffer, 0, 8);
	handleControlFunc = handleControl;
	handleOptionDataFunc = handleOptionData;
	handleFutureDataFunc = handleFutureData;
	configParser config;
	ostringstream configFile;
	configFile << configDirectoryPath << "\\" << linkDoorman::configFileName;
	if(!config.parseConfig(configFile.str().c_str())) {
		ostringstream oss;
		oss << "Failed to load config file " << configFile.str().c_str();
		logUtils::printInitializationError(oss.str().c_str(), __FILE__, __LINE__);
		return false;
	}
	marketFeedLogin = config.getAllocStringParam(string("marketFeedLogin"));
	marketFeedPassword = config.getAllocStringParam(string("marketFeedPassword"));
	if(!marketFeedLogin || !marketFeedPassword) {
		logUtils::printInitializationError("Missing marketFeedLogin / marketFeedPassword Configuration", __FILE__, __LINE__);
		return false;
	}
	if(!config.getBoolParam(string("simulateMarketFeed"), simulateMarketFeed)) {
		logUtils::printInitializationError("Missing simulateMarketFeed Configuration", __FILE__, __LINE__);
		return false;
	}
	if(simulateMarketFeed) {

#ifdef __RELEASE__
		logUtils::printInitializationError("The release version should not be run in test mode", __FILE__, __LINE__);
		return false;	
#endif

	}
	else {
		if(!FastMarketInit2(handleControlMessage, handleOptionDataMessage)) {
			logUtils::printInitializationError("Failed to init the MarketFeedListener", __FILE__, __LINE__);
			return false;
		}
	}
	return true;
}


void marketFeedListener::destroy() throw()
{
	if(simulateMarketFeed) {
		marketFeedSimulator::destroy();
	}
	if(marketFeedListenerLog) {
		marketFeedListenerLog->flush();
		marketFeedListenerLog->close();
		delete marketFeedListenerLog;
		marketFeedListenerLog = 0;
	}
	if(prices) {
		delete[] prices;
		prices = 0;
	}
	if(qties) {
		delete[] qties;
		qties = 0;
	}
	if(marketFeedLogin) {
		delete[] marketFeedLogin;
		marketFeedLogin = 0;
	}
	if(marketFeedPassword) {
		delete[] marketFeedPassword;
		marketFeedPassword = 0;
	}
	if(qtyBuffer) {
		delete[] qtyBuffer;
		qtyBuffer = 0;
	}
	if(priceBuffer) {
		delete[] priceBuffer;
		priceBuffer = 0;
	}
	DeleteCriticalSection(&marketFeedListenerLogCS);
}


bool marketFeedListener::run() throw()
{
	if(!simulateMarketFeed) {

#ifdef __LOGS__
		// This should occur after the initial exp dates have been computed
		printMarketFeedLogHeader();
#endif

		if(!StartFastMarket(marketFeedLogin, marketFeedPassword)) {
			logUtils::printInitializationError("StartFastMarket error", __FILE__, __LINE__);
			return false;
		}
	}
	return true;
}


void marketFeedListener::handleOptionDataMessage(char* message, long messageId) throw()
{

#ifdef __DEBUG__
	logUtils::printMsg("Received message", logUtils::LOG, "marketFeedListener::handleOptionDataMessage");
#endif

	if((*message) == 'F') {
		if((*(message + 1)) == 'E') {
			handleBidUpdate(message, messageId);
		}
		else {
			if((*(message + 1)) == 'F') {
				handleAskUpdate(message, messageId);
			}
			else {
				if((*(message + 1)) == 'H') {
					handleTickerUpdate(message, messageId);
				}
			}
		}
	}

#ifdef __DEBUG__
	logUtils::printMsg("Message processed", logUtils::LOG, "marketFeedListener::handleOptionDataMessage");
#endif

}


unsigned int marketFeedListener::getOptionId(const char* message) throw()
{
	unsigned int id = (((*(message + 3)) - '0') * 10000000) +
					  (((*(message + 4)) - '0') * 1000000) + 
					  (((*(message + 5)) - '0') * 100000) +
					  (((*(message + 6)) - '0') * 10000) +
					  (((*(message + 7)) - '0') * 1000) +
					  (((*(message + 8)) - '0') * 100) +
					  (((*(message + 9)) - '0') * 10) +
					  (((*(message + 10)) - '0'));
	return id;
}


void marketFeedListener::fillPricesAndQties(const char* message, unsigned int defaultPrice) throw()
{
	memcpy(qtyBuffer, message + 32, 9);
	qties[0] = atoi(qtyBuffer);
	memcpy(qtyBuffer, message + 41, 9);
	qties[1] = atoi(qtyBuffer);
	memcpy(qtyBuffer, message + 50, 9);
	qties[2] = atoi(qtyBuffer);
	if((qties[0] == 0) || ((*(message + 15)) == '.')) {
		prices[0] = prices[1] = prices[2] = defaultPrice;
		qties[0] = qties[1] = qties[2] = 0;
	}
	else {
		memcpy(priceBuffer, message + 11, 7);
		prices[0] = atoi(priceBuffer);
		if((qties[1] == 0) || ((*(message + 22)) == '.')) {
			prices[1] = prices[2] = defaultPrice;
			qties[1] = qties[2] = 0;
		}
		else {
			memcpy(priceBuffer, message + 18, 7);
			prices[1] = atoi(priceBuffer);
			if((qties[2] == 0) || ((*(message + 29)) == '.')) {
				prices[2] = defaultPrice;
				qties[2] = 0;
			}
			else {
				memcpy(priceBuffer, message + 25, 7);
				prices[2] = atoi(priceBuffer);
			}
		}
	}
}


bool marketFeedListener::fillLastPriceAndQty(const char* message) throw()
{
	memcpy(qtyBuffer, message + 31, 9);
	lastQty = atoi(qtyBuffer);
	if((lastQty == 0) || ((*(message + 15)) == '.')) {
		return false;
	}
	memcpy(priceBuffer, message + 11, 7);
	lastPrice = atoi(priceBuffer);
	return true;
}


void marketFeedListener::registerLoop() throw()
{

#ifdef __LOGS__
	LONGLONG ts = 0;
	if(!timer::getTimestamp(ts)) {
		ts = 0;
	}
	EnterCriticalSection(&marketFeedListenerLogCS);
	(*marketFeedListenerLog) << ts << " L" << endl;
	LeaveCriticalSection(&marketFeedListenerLogCS);
#endif

}


void marketFeedListener::handleBidUpdate(const char* message, long messageId) throw()
{

#ifdef __PROFILE__
	profiler profile("marketFeedListener", "handleBidUpdate");
#endif

	unsigned int index = UINT_MAX, optionId = getOptionId(message);
	if(marketSpecific::getOptionDBIndex(optionId, index)) {
		fillPricesAndQties(message, 0);

#ifdef __LOGS__
		LONGLONG ts = 0;
		if(!timer::getTimestamp(ts)) {
			ts = 0;
		}
		EnterCriticalSection(&marketFeedListenerLogCS);
		(*marketFeedListenerLog) << ts << " " << messageId << " B " << optionId << " ";
		(*marketFeedListenerLog) << prices[0] << " " << prices[1] << " " << prices[2] << " ";
		(*marketFeedListenerLog) << qties[0] << " " << qties[1] << " " << qties[2];
		(*marketFeedListenerLog) << endl;
		LeaveCriticalSection(&marketFeedListenerLogCS);
#endif

#ifdef __DEBUG__
		logUtils::printMsg("Handling bid update", logUtils::LOG, "marketFeedListener::handleBidUpdate");
#endif

		handleOptionDataFunc(index, prices, qties, 0, 0, 0, 0, 0, 0);
	}
}


void marketFeedListener::handleAskUpdate(const char* message, long messageId) throw()
{

#ifdef __PROFILE__
	profiler profile("marketFeedListener", "handleAskUpdate");
#endif

	unsigned int index = UINT_MAX, optionId = getOptionId(message);
	if(marketSpecific::getOptionDBIndex(optionId, index)) {
		fillPricesAndQties(message, UINT_MAX);

#ifdef __LOGS__
		LONGLONG ts = 0;
		if(!timer::getTimestamp(ts)) {
			ts = 0;
		}
		EnterCriticalSection(&marketFeedListenerLogCS);
		(*marketFeedListenerLog) << ts << " " << messageId << " A " << optionId << " ";
		(*marketFeedListenerLog) << prices[0] << " " << prices[1] << " " << prices[2] << " ";
		(*marketFeedListenerLog) << qties[0] << " " << qties[1] << " " << qties[2];
		(*marketFeedListenerLog) << endl;
		LeaveCriticalSection(&marketFeedListenerLogCS);
#endif

#ifdef __DEBUG__
		logUtils::printMsg("Handling ask update", logUtils::LOG, "marketFeedListener::handleAskUpdate");
#endif

		handleOptionDataFunc(index, 0, 0, prices, qties, 0, 0, 0, 0);
	}
}


void marketFeedListener::handleTickerUpdate(const char* message, long messageId) throw()
{

#ifdef __PROFILE__
	profiler profile("marketFeedListener", "handleTickerUpdate");
#endif

	unsigned int index = UINT_MAX, optionId = getOptionId(message);
	if(marketSpecific::getOptionDBIndex(optionId, index)) {
		fillLastPriceAndQty(message);

#ifdef __LOGS__
		LONGLONG ts = 0;
		if(!timer::getTimestamp(ts)) {
			ts = 0;
		}
		EnterCriticalSection(&marketFeedListenerLogCS);
		(*marketFeedListenerLog) << ts << " " << messageId << " T " << optionId << " " << lastPrice << " " << lastQty << endl;
		LeaveCriticalSection(&marketFeedListenerLogCS);
#endif

#ifdef __DEBUG__
		logUtils::printMsg("Handling ticker update", logUtils::LOG, "marketFeedListener::handleTickerUpdate");
#endif	

		handleOptionDataFunc(index, 0, 0, 0, 0, lastPrice, lastQty, 0, 0);
	}
}


void marketFeedListener::handleControlMessage(int messageId, char* message) throw()
{

#ifdef __LOGS__
//	LONGLONG ts = 0;
//	if(!timer::getTimestamp(ts)) {
//		ts = 0;
//	}
//	EnterCriticalSection(&marketFeedListenerLogCS);
//	(*marketFeedListenerLog) << ts << " " << message << "\n";
//	LeaveCriticalSection(&marketFeedListenerLogCS);
#endif

	logUtils::printMsg(message, logUtils::INF, "marketFeedListener::handleControlMessage");
}


void marketFeedListener::printMarketFeedLogHeader() throw()
{
#ifdef __LOGS__
	LONGLONG ts = 0;
	if(!timer::getTimestamp(ts)) {
		ts = 0;
	}
	EnterCriticalSection(&marketFeedListenerLogCS);
	char day[3], month[3], year[3];
	unsigned int hour, minutes, seconds;
	timeUtils::getCurrentDate(month, day, year);
	timeUtils::getCurrentTime(hour, minutes, seconds);
	(*marketFeedListenerLog) << timer::getTimerFrequency() << endl;
	(*marketFeedListenerLog) << ts << " " << day << " " << month << " " << year << " " << hour << " " << minutes << " " << seconds << endl;
	LeaveCriticalSection(&marketFeedListenerLogCS);
#endif
}


