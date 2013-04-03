
#pragma once

class matchingEngineTickerParser
{
private:
	matchingEngineTickerParser() throw();
	matchingEngineTickerParser(const matchingEngineTickerParser& ref) throw();
	~matchingEngineTickerParser() throw();

public:
	static bool parse(char* buffer, unsigned int& price, unsigned int& qty) throw();
};

