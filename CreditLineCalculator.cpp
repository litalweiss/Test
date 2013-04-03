
#include "CreditLineCalculator.h"
#include "LinkDoorman.h"
#include "MarketSpecific.h"
#include <ConfigParser.h>
#include <LogUtils.h>
#include <TimeUtils.h>
#include <Timer.h>
#include <sstream>
#include <iomanip>

using namespace std;


double creditLineCalculator::writtingCredit = 0.0;
double creditLineCalculator::purchaseCredit = 0.0;
double creditLineCalculator::initialWrittingCredit = 0.0;
double creditLineCalculator::initialPurchaseCredit = 0.0;
CRITICAL_SECTION creditLineCalculator::creditLineCalculatorCS;
ofstream* creditLineCalculator::creditLineCalculatorLog = 0;


const bool creditLineCalculator::init(const char* configDirectoryPath, const char* logDirectoryPath) throw()
{

#ifdef __LOGS__
	char month[3], day[3], year[3];
	timeUtils::getCurrentDate(month, day, year);
	ostringstream oss;
	oss << logDirectoryPath << "\\" << year << "-" << month << "-" << day << "-creditLineCalculatorLog.txt";
	creditLineCalculatorLog = new ofstream(oss.str().c_str(), std::ios_base::app);
	if(!creditLineCalculatorLog || creditLineCalculatorLog->fail() || creditLineCalculatorLog->bad()) {
		logUtils::printInitializationError("creditLineCalculator init error", __FILE__, __LINE__);
		return false;
	}
	(*creditLineCalculatorLog) << fixed << setprecision(2) << timer::getTimerFrequency() << endl;
#endif

	configParser config;
	ostringstream configFile;
	configFile << configDirectoryPath << "\\" << linkDoorman::configFileName;
	if(!config.parseConfig(configFile.str().c_str())) {
		ostringstream oss;
		oss << "Failed to load config file " << configFile.str().c_str();
		logUtils::printInitializationError(oss.str().c_str(), __FILE__, __LINE__);
		return false;
	}
	if(!config.getDoubleParam(string("writtingCredit"), writtingCredit)) {
		logUtils::printInitializationError("Missing writtingCredit configuration", __FILE__, __LINE__);
		return false;
	}
	if(!config.getDoubleParam(string("purchaseCredit"), purchaseCredit)) {
		logUtils::printInitializationError("Missing purchaseCredit configuration", __FILE__, __LINE__);
		return false;
	}
	initialWrittingCredit = writtingCredit;
	initialPurchaseCredit = purchaseCredit;
	InitializeCriticalSection(&creditLineCalculatorCS);


	return true;
}


void creditLineCalculator::destroy() throw()
{
	DeleteCriticalSection(&creditLineCalculatorCS);

#ifdef __LOGS__
	if(creditLineCalculatorLog) {
		creditLineCalculatorLog->flush();
		creditLineCalculatorLog->close();
		delete creditLineCalculatorLog;
		creditLineCalculatorLog = 0;
	}
#endif

}


void creditLineCalculator::cancelOrder(const bool buy, const unsigned int qty, const unsigned int price) throw()
{
	if(buy) {
		EnterCriticalSection(&creditLineCalculatorCS);

#ifdef __LOGS__
		LONGLONG ts = 0;
		if(!timer::getTimestamp(ts)) {
			ts = 0;
		}
		(*creditLineCalculatorLog) << ts << " C B " << purchaseCredit << " " << writtingCredit << " ";
#endif

		double cost = static_cast<double>(qty) * (price + marketSpecific::getCommission(price));
		purchaseCredit += cost;
		writtingCredit += cost;

#ifdef __LOGS__
		(*creditLineCalculatorLog) << qty << " " << price << " " << purchaseCredit << " " << writtingCredit << endl;
#endif

		LeaveCriticalSection(&creditLineCalculatorCS);
	}
}


const bool creditLineCalculator::allowNewOrder(const bool buy, const unsigned int qty, const unsigned int price) throw()
{
	bool ret = true;
	if(buy) {
		EnterCriticalSection(&creditLineCalculatorCS);

#ifdef __LOGS__
		LONGLONG ts = 0;
		if(!timer::getTimestamp(ts)) {
			ts = 0;
		}
		(*creditLineCalculatorLog) << ts << " N B " << purchaseCredit << " " << writtingCredit << " ";
#endif

		double cost = static_cast<double>(qty) * (price + marketSpecific::getCommission(price));
		if(purchaseCredit >= cost) {
			purchaseCredit -= cost;
			writtingCredit -= cost;
		}
		else {
			ret = false;
		}

#ifdef __LOGS__
		(*creditLineCalculatorLog) << qty << " " << price << " " << purchaseCredit << " " << writtingCredit << endl;
#endif

		LeaveCriticalSection(&creditLineCalculatorCS);
	}
	return ret;
}


void creditLineCalculator::imposeOrder(const bool buy, const unsigned int qty, const unsigned int price) throw()
{
	if(buy) {
		EnterCriticalSection(&creditLineCalculatorCS);

#ifdef __LOGS__
		LONGLONG ts = 0;
		if(!timer::getTimestamp(ts)) {
			ts = 0;
		}
		(*creditLineCalculatorLog) << ts << " I B " << purchaseCredit << " " << writtingCredit << " ";
#endif

		double cost = static_cast<double>(qty) * (price + marketSpecific::getCommission(price));
		purchaseCredit -= cost;
		writtingCredit -= cost;

#ifdef __LOGS__
		(*creditLineCalculatorLog) << qty << " " << price << " " << purchaseCredit << " " << writtingCredit << endl;
#endif

		LeaveCriticalSection(&creditLineCalculatorCS);
	}
}


const bool creditLineCalculator::allowChangeOrder(const bool buy, const unsigned int newQty, const unsigned int newPrice, const unsigned int oldQty, const unsigned int oldPrice) throw()
{
	bool ret = true;
	if(buy) {
		EnterCriticalSection(&creditLineCalculatorCS);

#ifdef __LOGS__
		LONGLONG ts = 0;
		if(!timer::getTimestamp(ts)) {
			ts = 0;
		}
		(*creditLineCalculatorLog) << ts << " U B " << purchaseCredit << " " << writtingCredit << " ";
#endif

		double cost = static_cast<double>(newQty) * (newPrice + marketSpecific::getCommission(newPrice));
		double profit = static_cast<double>(oldQty) * (oldPrice + marketSpecific::getCommission(oldPrice));
		if(purchaseCredit >= (cost - profit)) {
			purchaseCredit -= (cost - profit);
			writtingCredit -= (cost - profit);
		}
		else {
			ret = false;
		}

#ifdef __LOGS__
		(*creditLineCalculatorLog) << oldQty << " " << oldPrice << " " << newQty << " " << newPrice << " ";
		(*creditLineCalculatorLog) << purchaseCredit << " " << writtingCredit << endl;
#endif

		LeaveCriticalSection(&creditLineCalculatorCS);
	}
	return ret;
}


void creditLineCalculator::execution(const bool buy, const unsigned int qty, const unsigned int price) throw()
{
	if(!buy) {
		EnterCriticalSection(&creditLineCalculatorCS);

#ifdef __LOGS__
		LONGLONG ts = 0;
		if(!timer::getTimestamp(ts)) {
			ts = 0;
		}
		(*creditLineCalculatorLog) << ts << " E S " << purchaseCredit << " " << writtingCredit << " ";
#endif

		double profit = static_cast<double>(qty) * (price - marketSpecific::getCommission(price));
		purchaseCredit += profit;
		writtingCredit += profit;

#ifdef __LOGS__
		(*creditLineCalculatorLog) << qty << " " << price << " " << purchaseCredit << " " << writtingCredit << endl;
#endif

		LeaveCriticalSection(&creditLineCalculatorCS);
	}
}


void creditLineCalculator::cancelExecution(const bool buy, const unsigned int qty, const unsigned int price) throw()
{
	EnterCriticalSection(&creditLineCalculatorCS);

#ifdef __LOGS__
	LONGLONG ts = 0;
	if(!timer::getTimestamp(ts)) {
		ts = 0;
	}
	(*creditLineCalculatorLog) << ts << " CE " << (buy ? "B " : "S ") << purchaseCredit << " " << writtingCredit << " ";
#endif

	double profit = static_cast<double>(qty);
	if(buy) {
		profit *= (price + marketSpecific::getCommission(price));
	}
	else {
		profit *= (-1.0 * (price - marketSpecific::getCommission(price)));
	}
	purchaseCredit += profit;
	writtingCredit += profit;

#ifdef __LOGS__
	(*creditLineCalculatorLog) << qty << " " << price << " " << purchaseCredit << " " << writtingCredit << endl;
#endif

	LeaveCriticalSection(&creditLineCalculatorCS);
}


void creditLineCalculator::resetCredits()
{
	EnterCriticalSection(&creditLineCalculatorCS);
	purchaseCredit = initialPurchaseCredit;
	writtingCredit = initialWrittingCredit;
	LeaveCriticalSection(&creditLineCalculatorCS);
}


