
#pragma once

#include "MatchingEngineWrapper.h"

class matchingEngineParser
{
private:
	matchingEngineParser() throw();
	matchingEngineParser(const matchingEngineParser& ref) throw();
	~matchingEngineParser() throw();

public:
	static const unsigned int idLength = 32;
	static const unsigned int messageLength = 512;
	static const unsigned int codeLength = 16;

public:
	static const bool init(const char* logDirectoryPath) throw();
	static void destroy() throw();
	static void parseInteger(char*& iter, unsigned int& target, unsigned int prefixLength) throw();
	static void parseChar(char*& iter, char& target, unsigned int prefixLength) throw();
	static void parseString(char*& iter, char* target, unsigned int prefixLength) throw();
	static void parseOrderType(char*& iter, matchingEngineWrapper::orderTypes& target, unsigned int prefixLength) throw();
	static void parseOperationType(char*& iter, matchingEngineWrapper::operationTypes& target, unsigned int prefixLength) throw();
	static void parseOrderState(char*& iter, matchingEngineWrapper::orderStates& target, unsigned int prefixLength) throw();
	static void passOver(char*& iter) throw();
};


