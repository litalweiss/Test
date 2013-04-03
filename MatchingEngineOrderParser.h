
#pragma once

#include "MatchingEngineParser.h"
#include "MatchingEngineIdentificator.h"
#include "MatchingEngineWrapper.h"


class matchingEngineOrderParser
{
private:
	matchingEngineOrderParser() throw();
	matchingEngineOrderParser(const matchingEngineOrderParser& ref) throw();
	~matchingEngineOrderParser() throw();
	static void reset() throw();
	static bool parse1(char*& iter) throw();
	static bool parse3(char*& iter) throw();
	static bool parse4(char*& iter) throw();
	static bool parse5(char*& iter) throw();
	static bool parseS(char*& iter) throw();
	static bool parseO(char*& iter) throw();
	static bool parseE(char*& iter) throw();

private:
	static char* tempId;
	static char* matchingEngineId;
	static char* matchingEngineInitialId;
	static char* stockExchangeId;
	static char* stockExchangeInitialId;
	static char* stockExchangeRejectReason;
	static char* stockExchangeRejectCode;
	static char* matchingEngineRejectReason;
	static char* matchingEngineRejectCode;
	static matchingEngineWrapper::orderTypes orderType;
	static matchingEngineWrapper::operationTypes operationType;
	static matchingEngineWrapper::orderStates orderState;
	static unsigned int optionId;
	static unsigned int initialQty;
	static unsigned int price;
	static unsigned int executedQty;

public:
	static void parse(char* buffer) throw();
	static bool init() throw();
	static void destroy() throw();
	static const unsigned int getOptionId() throw() {return optionId;}
	static const unsigned int getInitialQty() throw() {return initialQty;}
	static const unsigned int getExecutedQty() throw() {return executedQty;}
	static const unsigned int getPrice() throw() {return price;}
	static const matchingEngineWrapper::orderStates getOrderState() throw() {return orderState;}
	static const matchingEngineWrapper::orderTypes getOrderType() throw() {return orderType;}
	static const matchingEngineWrapper::operationTypes getOperationType() throw() {return operationType;}
	static char* getTempId() throw() {return tempId;}
	static char* getMatchingEngineId() throw() {return matchingEngineId;}
	static char* getMatchingEngineInitialId() throw() {return matchingEngineInitialId;}
	static char* getStockExchangeId() throw() {return stockExchangeId;}
	static char* getStockExchangeInitialId() throw() {return stockExchangeInitialId;}
	static char* getStockExchangeRejectReason() throw() {return stockExchangeRejectReason;}
	static char* getStockExchangeRejectCode() throw() {return stockExchangeRejectCode;}
	static char* getMatchingEngineRejectReason() throw() {return matchingEngineRejectReason;}
	static char* getMatchingEngineRejectCode() throw() {return matchingEngineRejectCode;}
	static const bool updateIdentificator(matchingEngineIdentificator& identificator, bool& change) throw();
};
