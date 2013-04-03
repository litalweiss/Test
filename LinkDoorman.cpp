
#include "LinkDoorman.h"
#include "MarketSpecific.h"
#include "MatchingEngineOrderParser.h"
#include "MatchingEngineDealParser.h"
#include "MatchingEngineFailedOrderParser.h"
#include "CreditLineCalculator.h"
#include "MatchingEngineAnswerParser.h"
#include "MatchingEngineParser.h"
#include "MatchingEngineMessageComposer.h"
#include "MatchingEngineWrapper.h"
#include <LogUtils.h>
#include "MarketFeedListener.h"


const char* const linkDoorman::configFileName = "linkConfig.txt";


bool linkDoorman::initialize(const char* configDirectoryPath, const char* optionFileName, const char* logDirectoryPath, 
							 const char* testFileName, 
							 marketFeedWrapper::handleControlFuncPtr handleControl, 
							 marketFeedWrapper::handleOptionDataFuncPtr handleOptionData, 
							 marketFeedWrapper::handleFutureDataFuncPtr handleFutureData, 
		                     marketFeedWrapper::tradingSimulationFuncPtr simulateTrading, 
							 matchingEngineWrapper::handleMatchingEngineUpdateFuncPtr handleOrderUpdate, 
		                     matchingEngineWrapper::handleMatchingEngineUpdateFuncPtr handleFailedOrderUpdate, 
							 matchingEngineWrapper::handleMatchingEngineUpdateFuncPtr handleExecutionUpdate, 
		                     matchingEngineWrapper::handleMatchingEngineReconnectionFuncPtr recoverFromDisconnection, 
							 matchingEngineWrapper::closePositionFuncPtr closePositionFunction) throw()
{
	if(!marketSpecific::init(configDirectoryPath, optionFileName)) {
		logUtils::printInitializationError("marketSpecific init error", __FILE__, __LINE__);
		return false;
	}
	if(!matchingEngineWrapper::init(configDirectoryPath, logDirectoryPath, handleExecutionUpdate, handleOrderUpdate, handleFailedOrderUpdate, recoverFromDisconnection, closePositionFunction)) {
		logUtils::printInitializationError("matchingEngineWrapper init error", __FILE__, __LINE__);
		return false;
	}
	if(!marketFeedWrapper::init(configDirectoryPath, logDirectoryPath, testFileName, handleOptionData, handleFutureData, simulateTrading, handleControl)) {
		logUtils::printInitializationError("marketFeedWrapper init error", __FILE__, __LINE__);
		return false;
	}
	if(!matchingEngineOrderParser::init()) {
		logUtils::printInitializationError("marketEngineOrderParser init error", __FILE__, __LINE__);
		return false;
	}
	if(!matchingEngineDealParser::init()) {
		logUtils::printInitializationError("matchingEngineDealParser init error", __FILE__, __LINE__);
		return false;
	}
	if(!matchingEngineFailedOrderParser::init()) {
		logUtils::printInitializationError("matchingEngineFailedOrderParser init error", __FILE__, __LINE__);
		return false;
	}
	if(!creditLineCalculator::init(configDirectoryPath, logDirectoryPath)) {
		logUtils::printInitializationError("creditLineCalculator init error", __FILE__, __LINE__);
		return false;
	}
	if(!matchingEngineMessageComposer::init()) {
		logUtils::printInitializationError("matchingEngineMessageComposer init error", __FILE__, __LINE__);
		return false;
	}
	if(!matchingEngineAnswerParser::init()) {
		logUtils::printInitializationError("matchingEngineAnswerParser init error", __FILE__, __LINE__);
		return false;
	}
	if(!matchingEngineParser::init(logDirectoryPath)) {
		logUtils::printInitializationError("matchingEngineParser init error", __FILE__, __LINE__);
		return false;
	}
	return true;
}


bool linkDoorman::run() throw()
{
	if(!marketFeedListener::run()) {
		logUtils::printInitializationError("marketFeedListener run error", __FILE__, __LINE__);
		return false;
	}
	return true;
}


void linkDoorman::destroy() throw()
{
	matchingEngineParser::destroy();
	matchingEngineAnswerParser::destroy();
	matchingEngineMessageComposer::destroy();
	creditLineCalculator::destroy();
	matchingEngineFailedOrderParser::destroy();
	matchingEngineDealParser::destroy();
	matchingEngineOrderParser::destroy();
	marketFeedWrapper::destroy();
	matchingEngineWrapper::destroy();
	marketSpecific::destroy();
}




