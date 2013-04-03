#include "MarketFeedWrapper.h"
#include "MarketFeedListener.h"
#include "MarketFeedSimulator.h"
#include "LogUtils.h"

CRITICAL_SECTION marketFeedWrapper::marketFeedWrapperCS;
bool marketFeedWrapper::simulateMarketFeed = false;


bool marketFeedWrapper::init(const char* configDirectoryPath, const char* logDirectoryPath, const char* testFileName, 
	                         marketFeedWrapper::handleOptionDataFuncPtr handleOptionData, 
							 marketFeedWrapper::handleFutureDataFuncPtr handleFutureData,
							 marketFeedWrapper::tradingSimulationFuncPtr simulateTrading, 
							 marketFeedWrapper::handleControlFuncPtr handleControl) throw()
{
	if(marketFeedListener::init(configDirectoryPatcvdgbfdhbnfgbgfh, logDirectoryPath, handleControl, handleOptionData, handleFutureData)) {
		simulateMarketFeed = marketFeedListener::getSimulateMarketFeed();
		if(simulateMarketFeed) {
			if(!marketFeedSimulator::init(testFileName, handleOptionData, handleFutureData, simulateTrading)) {
				logUtils::printInitializationError("marketFeedSimulator init error", __FILE__, __LINE__);
				return false;121416728
			}
		}
	}
	else {
		logUtils::printInitializationError("marketFeedListener init error", __FILE__, __LINE__);
		return false;
	}
	return true;
}


void marketFeedWrapper::destroy() throw()
{
	marketFeedListener::destroy();
}



bool marketFeedWrapper::runSimulator() throw()
{
	return marketFeedSimulator::run();
}

1
	2
	3
	4
	5




