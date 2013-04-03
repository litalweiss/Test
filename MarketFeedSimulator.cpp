
#include "MarketFeedSimulator.h"
#include "MarketSpecific.h"
#include <LogUtils.h>
#include <TimeUtils.h>
#include <ParsingUtils.h>
#include <Timer.h>
#include <Scheduler.h>
#include <string>
#include <sstream>

using namespace std;


ifstream* marketFeedSimulator::ifs = 0;
marketFeedWrapper::handleOptionDataFuncPtr marketFeedSimulator::handleOptionDataFunc = 0;
marketFeedWrapper::handleFutureDataFuncPtr marketFeedSimulator::handleFutureDataFunc = 0;
marketFeedWrapper::tradingSimulationFuncPtr marketFeedSimulator::tradingSimulation = 0;
LONGLONG marketFeedSimulator::frequency = 0;
LONGLONG marketFeedSimulator::currentTS = 0;
unsigned int marketFeedSimulator::optionIndex = 0;
unsigned int marketFeedSimulator::lastPrice = 0;
unsigned int marketFeedSimulator::lastQty = 0;
unsigned int* marketFeedSimulator::bidPrices = 0;
unsigned int* marketFeedSimulator::bidQties = 0;
unsigned int* marketFeedSimulator::askPrices = 0;
unsigned int* marketFeedSimulator::askQties = 0;
double marketFeedSimulator::secondsFromStart = 0;


bool marketFeedSimulator::init(const char* testFileName,
							   marketFeedWrapper::handleOptionDataFuncPtr handleOptionData,
							   marketFeedWrapper::handleFutureDataFuncPtr handleFutureData,
							   marketFeedWrapper::tradingSimulationFuncPtr tradingSimulationFunction) throw()
{
	if(!testFileName || (testFileName == "")) {
		logUtils::printInitializationError("You should provide a test file", __FILE__, __LINE__);
		return false;
	}
	ifs = parsingUtils::openInputFile(testFileName);
	if(!ifs) {
		logUtils::printInitializationError("Unable to open market feed test file", __FILE__, __LINE__);
		return false;
	}
	handleOptionDataFunc = handleOptionData;
	handleFutureDataFunc = handleFutureData;
	tradingSimulation = tradingSimulationFunction;
	bidPrices = new unsigned int[marketSpecific::optionBookDepth];
	bidQties = new unsigned int[marketSpecific::optionBookDepth];
	askPrices = new unsigned int[marketSpecific::optionBookDepth];
	askQties = new unsigned int[marketSpecific::optionBookDepth];
	if(!bidPrices || !bidQties || !askPrices || !askQties) {
		logUtils::printInitializationError("Memory allocation error", __FILE__, __LINE__);
		return false;
	}
	resetBuffers();
	if(!processHeader(currentTS)) {
		return false;
	}
	return true;
}


void marketFeedSimulator::destroy() throw()
{
	if(ifs) {
		parsingUtils::closeInputFile(ifs);
	}
	ifs = 0;
	if(bidPrices) {
		delete[] bidPrices;
		bidPrices = 0;
	}
	if(bidQties) {
		delete[] bidQties;
		bidQties = 0;
	}
	if(askPrices) {
		delete[] askPrices;
		askPrices = 0;
	}
	if(askQties) {
		delete[] askQties;
		askQties = 0;
	}
}


void marketFeedSimulator::resetBuffers() throw()
{
	for(unsigned int i = 0; i < marketSpecific::optionBookDepth; ++i) {
		bidPrices[i] = bidQties[i] = askPrices[i] = askQties[i] = 0;
	}
	lastPrice = lastQty = 0;
}


bool marketFeedSimulator::processHeader(LONGLONG& ts) throw()
{
	unsigned int day, month, year, hour, minutes, seconds;
	string line;
	parsingUtils::readLine(*ifs, line);
	istringstream frequencyStream(line);
	frequencyStream >> frequency;
	if(frequencyStream.fail() || frequencyStream.bad()) {
		logUtils::printInitializationError("Unable to read frequency data from test file", __FILE__, __LINE__);	
		return false;
	}
	parsingUtils::readLine(*ifs, line);
	istringstream expirationStream(line);
	expirationStream >> ts >> day >> month >> year >> hour >> minutes >> seconds;
	if(expirationStream.fail() || expirationStream.bad()) {
		logUtils::printMsg("Unable to read time data from test file", logUtils::ERR, "marketFeedSimulator::processHeader");	
		return false;
	}
	timer::setRealTime(false, frequency);
	timeUtils::setUseFake(true);
	if(!timeUtils::setFakeDate(year, month, day) || !timeUtils::setFakeTime(hour, minutes, seconds)) {
		logUtils::printMsg("Invalid time data from test file", logUtils::ERR, "marketFeedSimulator::processHeader");	
		return false;
	}
	return true;
}


bool marketFeedSimulator::run() throw()
{
	scheduler::initSchedulerRun();
	LONGLONG previousTS = 0;
	string line;
	bool ret = false;
	previousTS = currentTS;
	unsigned int counter = 0;
	while(true) {
		if(!(counter % 1000)) {
			ostringstream oss;
			oss << "Parsing line " << counter;
			logUtils::printMsg(oss.str().c_str(), logUtils::INF, "marketFeedSimulator::run");
		}
		if(!parsingUtils::readLine(*ifs, line)) {
			break;
		}
		if(line == "") {
			ret = true;
			break;
		}
		if(!processTestLine(line, previousTS, currentTS)) {
			break;
		}
		tradingSimulation();
		++counter;
	}
	parsingUtils::closeInputFile(ifs);
	ifs = 0;
	if(ret) {
		logUtils::printMsg("Simulation succeeded", logUtils::INF, "marketFeedSimulator::run");
	}
	return ret;
}


bool marketFeedSimulator::processOpcodeT(istringstream& stream, unsigned int*& bidPricesPtr,
										 unsigned int*& bidQtiesPtr, unsigned int*& askPricesPtr,
										 unsigned int*& askQtiesPtr) throw()
{
	stream >> lastPrice >> lastQty;
	if(stream.fail() || stream.bad()) {
		return false;
	}
	bidPricesPtr = bidQtiesPtr = askPricesPtr = askQtiesPtr = 0;
	return true;
}


bool marketFeedSimulator::processOpcodeA(istringstream& stream, unsigned int*& bidPricesPtr,
										 unsigned int*& bidQtiesPtr, unsigned int*& askPricesPtr,
										 unsigned int*& askQtiesPtr) throw()
{
	bidPricesPtr = bidQtiesPtr = 0;
	askPricesPtr = askPrices;
	askQtiesPtr = askQties;
	stream >> askPrices[0] >> askPrices[1] >> askPrices[2] >> askQties[0] >> askQties[1] >> askQties[2];
	if(stream.fail() || stream.bad()) {
		return false;
	}
	return true;
}


bool marketFeedSimulator::processOpcodeB(istringstream& stream, unsigned int*& bidPricesPtr,
										 unsigned int*& bidQtiesPtr, unsigned int*& askPricesPtr,
										 unsigned int*& askQtiesPtr) throw()
{
	askPricesPtr = askQtiesPtr = 0;
	bidPricesPtr = bidPrices;
	bidQtiesPtr = bidQties;
	stream >> bidPrices[0] >> bidPrices[1] >> bidPrices[2] >> bidQties[0] >> bidQties[1] >> bidQties[2];
	if(stream.fail() || stream.bad()) {
		return false;
	}
	return true;
}


bool marketFeedSimulator::processOpcodeC(istringstream& stream, unsigned int*& bidPricesPtr,
										 unsigned int*& bidQtiesPtr, unsigned int*& askPricesPtr,
										 unsigned int*& askQtiesPtr) throw()
{
	return false;
}


bool marketFeedSimulator::processOpcode(const char opcode, istringstream& stream, unsigned int*& bidPricesPtr,
										unsigned int*& bidQtiesPtr, unsigned int*& askPricesPtr,
										unsigned int*& askQtiesPtr) throw()
{
	if(opcode == 'T') {
		return processOpcodeT(stream, bidPricesPtr, bidQtiesPtr, askPricesPtr, askQtiesPtr);
	}
	else {
		if(opcode == 'A') {
			return processOpcodeA(stream, bidPricesPtr, bidQtiesPtr, askPricesPtr, askQtiesPtr);
		}
		else {
			if(opcode == 'B') {
				return processOpcodeB(stream, bidPricesPtr, bidQtiesPtr, askPricesPtr, askQtiesPtr);
			}
		}
	}
	return false;
}


bool marketFeedSimulator::processTestLine(string& line, LONGLONG& previousTS, LONGLONG& currentTS) throw()
{
	unsigned int optionId;
	unsigned int *bidPricesPtr, *bidQtiesPtr, *askPricesPtr, *askQtiesPtr;
	long messageId;
	char opcode;
	resetBuffers();
	istringstream stream(line);
	stream >> currentTS >> messageId;
	timer::setFakeTimestamp(currentTS);
	double secondsElapsed = ((static_cast<double>(currentTS - previousTS)) / frequency);
	secondsFromStart += secondsElapsed;
	unsigned int truncatedSecondsFromStart = static_cast<unsigned int>(secondsFromStart);
	timeUtils::addFakeTime(truncatedSecondsFromStart);
	secondsFromStart -= truncatedSecondsFromStart;
	scheduler::scheduleLoop(static_cast<unsigned int>(secondsElapsed * 1000.0));
	previousTS = currentTS;
	stream >> opcode >> optionId;
	if(!processOpcode(opcode, stream, bidPricesPtr, bidQtiesPtr, askPricesPtr, askQtiesPtr)) {
		ostringstream oss;
		oss << "Invalid test line: " << line;
		logUtils::printMsg(oss.str().c_str(), logUtils::ERR, "marketFeedSimulator::processTestLine");
		return false;
	}
	if(!marketSpecific::getOptionDBIndex(optionId, optionIndex)) {
		ostringstream oss;
		oss << "Invalid option id: " << line;
		logUtils::printMsg(oss.str().c_str(), logUtils::ERR, "marketFeedSimulator::processTestLine");
		return false;
	}
	if(!handleOptionDataFunc(optionIndex, bidPricesPtr, bidQtiesPtr, askPricesPtr, askQtiesPtr, lastPrice, lastQty, 0, 0)) {
		ostringstream oss;
		oss << "handleDataFunc error at line: " << line;
		logUtils::printMsg(oss.str().c_str(), logUtils::STK, "marketFeedSimulator::processTestLine");
		return false;
	}
	return true;
}


