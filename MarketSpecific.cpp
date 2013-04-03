
#include <math.h>
#include "MarketSpecific.h"
#include "LinkDoorman.h"
#include <ParsingUtils.h>
#include <LogUtils.h>
#include <ConfigParser.h>
#include <limits.h>
#include <fstream>

using namespace std;

const double marketSpecific::doubleStrikesStep = 10.0;
unsigned int marketSpecific::uintStrikeStep = 10;

unsigned int marketSpecific::optionIdLength = 8;
unsigned int marketSpecific::optionBookDepth = 3;
unsigned int marketSpecific::maxOptionId = 100000000;
unsigned int marketSpecific::fakeMaxOptionId = 100000000;
unsigned int marketSpecific::strikeNumber = 0;
unsigned int marketSpecific::minStrike = UINT_MAX;
unsigned int marketSpecific::maxStrike = 0;
unsigned int* marketSpecific::idToIndexMap = 0;
unsigned int* marketSpecific::idToPrefixId = 0;
double marketSpecific::fixedIR = 0.0;
unsigned int marketSpecific::multiple = 0;
double marketSpecific::commission = 0.0;
unsigned int marketSpecific::coolClosingTheoreticalLoss = 400;
unsigned int marketSpecific::intermediateClosingTheoreticalLoss = 450;
unsigned int marketSpecific::stressClosingTheoreticalLoss = 500;
unsigned int marketSpecific::coolClosingReactionalLoss = 1000;
unsigned int marketSpecific::intermediateClosingReactionalLoss = 1050;
unsigned int marketSpecific::stressClosingReactionalLoss = 1100;
unsigned int marketSpecific::stressClosingStepLoss = 75;
unsigned int marketSpecific::disableTradingTheoreticalLoss = 800;
unsigned int marketSpecific::disableTradingReactionalLoss = 1400;


void marketSpecific::destroy() throw()
{
	if(idToIndexMap) {
		delete[] idToIndexMap;
		idToIndexMap = 0;
	}
	if(idToPrefixId) {
		delete[] idToPrefixId;
		idToPrefixId = 0;
	}
}


const bool marketSpecific::init(const char* configDirectoryPath, const char* optionFileName) throw()
{
	if(!processOptionFile(optionFileName)) {
		return false;
	}
	int optionNumber = 2 * (((maxStrike - minStrike) / uintStrikeStep) + 1);
	if (!calculateMaxOptionId(optionFileName,optionNumber)) {
		return false;
	}
	idToIndexMap = new unsigned int[fakeMaxOptionId];
	idToPrefixId = new unsigned int[fakeMaxOptionId];
	for(unsigned int i = 0; i < fakeMaxOptionId; ++i) {
		idToIndexMap[i] = UINT_MAX;
		idToPrefixId[i] = UINT_MAX;
	}
	if(!fillIndexMap(optionFileName)) {
		return false;
	}

	configParser config;
	ostringstream configFile;
	configFile << configDirectoryPath << "\\" << linkDoorman::configFileName;
	if(!config.parseConfig(configFile.str().c_str())) {
		ostringstream oss;
		oss << "Failed to load config file " << configFile.str().c_str();
		logUtils::printInitializationError(oss.str().c_str(), __FILE__, __LINE__);
		return false;
	}
	if(!config.getDoubleParam(string("fixedIR"), fixedIR)) {
		logUtils::printInitializationError("Missing fixedIR configuration", __FILE__, __LINE__);
		return false;
	}
	if(!config.getDoubleParam(string("commission"), commission)) {
		logUtils::printInitializationError("Missing commission configuration", __FILE__, __LINE__);
		return false;
	}
	int tmpMultiple;
	if(!config.getIntParam(string("multiple"), tmpMultiple) || (tmpMultiple < 1)) {
		logUtils::printInitializationError("Missing / Invalid multiple configuration", __FILE__, __LINE__);
		return false;
	}
	multiple = static_cast<unsigned int>(tmpMultiple);
	int tmp;
	if(!config.getIntParam(string("coolClosingTheoreticalLoss"), tmp) || (tmp < 0)) {
		logUtils::printInitializationError("Missing / Invalid coolClosingTheoreticalLoss configuration", __FILE__, __LINE__);
		return false;
	}
	coolClosingTheoreticalLoss = static_cast<unsigned int>(tmp);
	if(!config.getIntParam(string("intermediateClosingTheoreticalLoss"), tmp) || (tmp < static_cast<int>(coolClosingTheoreticalLoss))) {
		logUtils::printInitializationError("Missing / Invalid intermediateClosingTheoreticalLoss configuration", __FILE__, __LINE__);
		return false;
	}
	intermediateClosingTheoreticalLoss = static_cast<unsigned int>(tmp);
	if(!config.getIntParam(string("stressClosingTheoreticalLoss"), tmp) || (tmp < static_cast<int>(intermediateClosingTheoreticalLoss))) {
		logUtils::printInitializationError("Missing / Invalid stressClosingTheoreticalLoss configuration", __FILE__, __LINE__);
		return false;
	}
	stressClosingTheoreticalLoss = static_cast<unsigned int>(tmp);
	if(!config.getIntParam(string("coolClosingReactionalLoss"), tmp) || (tmp < static_cast<int>(coolClosingTheoreticalLoss))) {
		logUtils::printInitializationError("Missing / Invalid coolClosingReactionalLoss configuration", __FILE__, __LINE__);
		return false;
	}
	coolClosingReactionalLoss = static_cast<unsigned int>(tmp);
	if(!config.getIntParam(string("intermediateClosingReactionalLoss"), tmp) || (tmp < static_cast<int>(coolClosingReactionalLoss))) {
		logUtils::printInitializationError("Missing / Invalid intermediateClosingReactionalLoss configuration", __FILE__, __LINE__);
		return false;
	}
	intermediateClosingReactionalLoss = static_cast<unsigned int>(tmp);
	if(!config.getIntParam(string("stressClosingReactionalLoss"), tmp) || (tmp < static_cast<int>(intermediateClosingReactionalLoss))) {
		logUtils::printInitializationError("Missing / Invalid stressClosingReactionalLoss configuration", __FILE__, __LINE__);
		return false;
	}
	stressClosingReactionalLoss = static_cast<unsigned int>(tmp);
	if(!config.getIntParam(string("stressClosingStepLoss"), tmp) || (tmp < 0)) {
		logUtils::printInitializationError("Missing / Invalid stressClosingStepLoss configuration", __FILE__, __LINE__);
		return false;
	}
	stressClosingStepLoss = static_cast<unsigned int>(tmp);
	if(!config.getIntParam(string("disableTradingTheoreticalLoss"), tmp) || (tmp < 0)) {
		logUtils::printInitializationError("Missing / Invalid disableTradingTheoreticalLoss configuration", __FILE__, __LINE__);
		return false;
	}
	disableTradingTheoreticalLoss = static_cast<unsigned int>(tmp);
	if(!config.getIntParam(string("disableTradingReactionalLoss"), tmp) || (tmp < static_cast<int>(disableTradingTheoreticalLoss))) {
		logUtils::printInitializationError("Missing / Invalid disableTradingReactionalLoss configuration", __FILE__, __LINE__);
		return false;
	}
	disableTradingReactionalLoss = static_cast<unsigned int>(tmp);
	strikeNumber = 1 + ((maxStrike - minStrike) / uintStrikeStep);
	return true;
}


const bool marketSpecific::processOptionFile(const char* optionFileName) throw()
{
	ifstream* ifs = parsingUtils::openInputFile(optionFileName);
	if(!ifs) {
		return false;
	}
	unsigned int optionNumber = 0;
	bool emptyFile = true;
	char* optionId = new char[marketSpecific::optionIdLength + 1];
	unsigned int expirationMonth = 0, isCall = 0, strike = 0;
	while(true) {
		string lineStr = "";
		if(!parsingUtils::readLine(*ifs, lineStr)) {
			logUtils::printInitializationError("Parsing error", __FILE__, __LINE__);
			parsingUtils::closeInputFile(ifs);
			delete []optionId;
			return false;
		}
		if(lineStr == "") {
			if(emptyFile || (optionNumber == 0) || (optionNumber % 2)) {
				logUtils::printInitializationError("Empty file / Invalid option number", __FILE__, __LINE__);
				parsingUtils::closeInputFile(ifs);
				delete []optionId;
				return false;
			}
			if(optionNumber != 2 * (((maxStrike - minStrike) / uintStrikeStep) + 1)) {
				logUtils::printInitializationError("Missing inner strikes definition", __FILE__, __LINE__);
				parsingUtils::closeInputFile(ifs);
				delete []optionId;
				return false;
			}
			parsingUtils::closeInputFile(ifs);
			delete []optionId;
			return true;
		}
		emptyFile = false;
		istringstream lineStream(lineStr);
		lineStream >> optionId >> expirationMonth >> isCall >> strike;
		if(lineStream.fail() || lineStream.bad()) {
			ostringstream oss;
			oss << "Bad format: " << lineStr;
			logUtils::printInitializationError(oss.str().c_str(), __FILE__, __LINE__);
			parsingUtils::closeInputFile(ifs);
			delete []optionId;
			return false;
		}
		if((expirationMonth <= 0) || (expirationMonth > 12) || (isCall < 0) || (isCall > 1) || (strike <= 0)) {
			ostringstream oss;
			oss << "Incorrect data: " << lineStr;
			logUtils::printInitializationError(oss.str().c_str(), __FILE__, __LINE__);
			parsingUtils::closeInputFile(ifs);
			delete []optionId;
			return false;
		}
		++optionNumber;
		if(strike < minStrike) {
			minStrike = strike;
		}
		if(strike > maxStrike) {
			maxStrike = strike;
		}
	}
	delete []optionId;
}


const bool marketSpecific::fillIndexMap(const char* optionFileName) throw()
{
	ifstream* ifs = parsingUtils::openInputFile(optionFileName);
	if(!ifs) {
		return false;
	}
	char* optionId = new char[marketSpecific::optionIdLength + 1];
	unsigned int expirationMonth = 0, isCall = 0, strike = 0;
	while(true) {
		string lineStr = "";
		if(!parsingUtils::readLine(*ifs, lineStr)) {
			logUtils::printInitializationError("Parsing error", __FILE__, __LINE__);
			parsingUtils::closeInputFile(ifs);
			delete []optionId;
			return false;
		}
		if(lineStr == "") {
			parsingUtils::closeInputFile(ifs);
			delete []optionId;
			return true;
		}
		istringstream lineStream(lineStr);
		lineStream >> optionId >> expirationMonth >> isCall >> strike;
		unsigned int intId = atoi(optionId);
		if(intId >= maxOptionId) {
			ostringstream oss;
			oss << "Incorrect data: " << lineStr;
			logUtils::printInitializationError(oss.str().c_str(), __FILE__, __LINE__);
			parsingUtils::closeInputFile(ifs);
			delete []optionId;
			return false;
		}
		unsigned int index = UINT_MAX;
		if(!getOptionDBIndex(strike, (isCall != 0), index)) {
			ostringstream oss;
			oss << "getOptionDBIndex error for: " << ((isCall != 0) ? "C" : "P") << "-" << strike;
			logUtils::printInitializationError(oss.str().c_str(), __FILE__, __LINE__);
			parsingUtils::closeInputFile(ifs);
			delete []optionId;
			return false;
		} 
		idToIndexMap[intId % fakeMaxOptionId] = index;
		idToPrefixId[intId % fakeMaxOptionId] = intId / fakeMaxOptionId;
	}
}


const double marketSpecific::getCommission(const double price) throw()
{
	return commission;
}


const double marketSpecific::getRealStrike(const unsigned int strike) throw()
{
	return strike;
}


const unsigned int marketSpecific::getOptionNumber() throw()
{
	return (2 * strikeNumber);
}


const unsigned int marketSpecific::getStrikeNumber() throw()
{
	return strikeNumber;
}


const unsigned int marketSpecific::getStrikeDBIndex(const unsigned int optionIndex) throw()
{
	return (optionIndex / 2);
}


const unsigned int marketSpecific::getRelatedOptionIndex(const unsigned int optionIndex) throw()
{
	if(optionIndex % 2) {
		return (optionIndex - 1);
	}
	return (optionIndex + 1);
}


const bool marketSpecific::getStrikeDBIndex(const unsigned int strike, unsigned int& index) throw()
{
	index = 0;
	if((strike < minStrike) || (strike > maxStrike)) {
		ostringstream oss;
		oss << "Invalid strike: " << strike;
		logUtils::printMsg(oss.str().c_str(), logUtils::ERR, "marketSpecific::getFutureDBIndex");
		return false;
	}
	index = ((strike - minStrike) / uintStrikeStep);
	return true;
}


const bool marketSpecific::getStrikeFromStrikeDBIndex(const unsigned int index, unsigned int& strike) throw()
{
	strike = index * uintStrikeStep + minStrike;
	if(strike > maxStrike) {
		ostringstream oss;
		oss << "Invalid index: " << index;
		logUtils::printMsg(oss.str().c_str(), logUtils::ERR, "marketSpecific::getStrikeFromStrikeDBIndex");
		return false;
	}
	return true;
}


const bool marketSpecific::validateOptionDBIndex(const unsigned int index) throw()
{
	return (minStrike + uintStrikeStep * (index / 2)) <= maxStrike;
}


const bool marketSpecific::validateStrikeDBIndex(const unsigned int index) throw()
{
	return (minStrike + index * uintStrikeStep) <= maxStrike;
}


const double marketSpecific::calculatePenalty(const unsigned int ordered, const unsigned int executed) throw()
{
	double penalty = ordered;
	penalty -= (static_cast<double>(executed) * 7.0);
	penalty *= 0.07;
	return penalty;
}


const bool marketSpecific::getInfoFromOptionDBIndex(const unsigned int index, bool& call, unsigned int& strike) throw()
{
	call = false;
	if(index % 2) {
		call = true;
	}
	strike = minStrike + uintStrikeStep * (index / 2);
	if(strike > maxStrike) {
		ostringstream oss;
		oss << "Invalid index: " << index;
		logUtils::printMsg(oss.str().c_str(), logUtils::ERR, "marketSpecific::getInfoFromOptionDBIndex");
		return false;
	}
	return true;
}


const bool marketSpecific::getOptionDBLowIndex(const unsigned int strike, unsigned int& index) throw()
{
	index = 0;
	if((strike < minStrike) || (strike > maxStrike)) {
		ostringstream oss;
		oss << "Invalid strike: " << strike;
		logUtils::printMsg(oss.str().c_str(), logUtils::ERR, "marketSpecific::getOptionDBLowIndex");
		return false;
	}
	index = (2 * (strike - minStrike) / uintStrikeStep);
	return true;
}


const bool marketSpecific::getOptionDBHighIndex(const unsigned int strike, unsigned int& index) throw()
{
	index = 0;
	if((strike < minStrike) || (strike > maxStrike)) {
		ostringstream oss;
		oss << "Invalid strike: " << strike;
		logUtils::printMsg(oss.str().c_str(), logUtils::ERR, "marketSpecific::getOptionDBHighIndex");
		return false;
	}
	index = ((2 * (strike - minStrike) / uintStrikeStep) + 1);
	return true;
}


const bool marketSpecific::getOptionDBIndex(const unsigned int strike, const bool call, unsigned int& index) throw()
{
	index = 0;
	if((strike < minStrike) || (strike > maxStrike)) {
		ostringstream oss;
		oss << "Invalid strike: " << strike;
		logUtils::printMsg(oss.str().c_str(), logUtils::ERR, "marketSpecific::getOptionDBIndex");
		return false;
	}
	index = ((2 * (strike - minStrike) / uintStrikeStep) + (call ? 1 : 0));
	return true;
}
 

void marketSpecific::appendOptionName(ostringstream& oss, const unsigned int optionId) throw()
{
	if((optionId >= maxOptionId) || (idToIndexMap[optionId % fakeMaxOptionId] == UINT_MAX) || (idToPrefixId[optionId % fakeMaxOptionId] != optionId / fakeMaxOptionId)) {
		oss << "N/A";
		return;
	}
	unsigned int index = idToIndexMap[optionId % fakeMaxOptionId];
	oss << ((index % 2) ? "C-" : "P-") << (minStrike + uintStrikeStep * (index / 2));
}


const bool marketSpecific::getOptionDBIndex(const unsigned int id, unsigned int& index) throw()
{
	if(id >= maxOptionId) {
		ostringstream oss;
		oss << "Invalid id: " << id;
		logUtils::printMsg(oss.str().c_str(), logUtils::ERR, "marketSpecific::getOptionDBIndex");
		return false;
	}
	index = idToIndexMap[id % fakeMaxOptionId];
	if(index == UINT_MAX || (idToPrefixId[id % fakeMaxOptionId] != id / fakeMaxOptionId)) {
		// Don't print error message since all the options that are not in the option file will get here
		return false;
	}
	return true;
}


unsigned int marketSpecific::getTradingPrice(const double price, const bool buy) throw()
{
	unsigned int uintPrice = static_cast<unsigned int>(price);
	if(buy) {
		return (uintPrice - uintPrice % getPriceStep(uintPrice, false));
	}
	else {
		return (uintPrice - uintPrice % getPriceStep(uintPrice, false) + getPriceStep(uintPrice, true));
	}
}


void marketSpecific::addPricingSteps(const unsigned int stepsNumber, const bool buy, unsigned int& price) throw()
{
	for(unsigned int i = 0; i < stepsNumber; ++i) {
		if(buy) {
			price += getPriceStep(price, true);
		}
		else {
			price -= getPriceStep(price, false);
		}
	}
}


const unsigned int marketSpecific::getPriceStep(const unsigned int price, const bool up) throw()
{
	if(price > 2000) {
		return 20;
	}
	if(price == 2000) {
		if(up) {
			return 20;
		}
		return 10;
	}
	if(price > 200) {
		return 10;
	}
	if(price == 200) {
		if(up) {
			return 10;
		}
		return 5;
	}
	if(price > 20) {
		return 5;
	}
	if(price == 20) {
		if(up) {
			return 5;
		}
		return 1;
	}
	return 1;
}


const bool marketSpecific::getBidAskStepsSpread(const unsigned int bid, const unsigned int ask, unsigned int& spread) throw()
{
	spread = 0;
	if(bid >= ask) {
		ostringstream oss;
		oss << "Invalid (bid, ask) = (" << bid << ", " << ask << ")";
		logUtils::printMsg(oss.str().c_str(), logUtils::ERR, "marketSpecific::getBidAskStepsSpread");
		return false;
	}
	if(bid % getPriceStep(bid, true)) {
		ostringstream oss;
		oss << "Invalid bid price:" << bid;
		logUtils::printMsg(oss.str().c_str(), logUtils::ERR, "marketSpecific::getBidAskStepsSpread");
		return false;
	}
	if(ask % getPriceStep(ask, true)) {
		ostringstream oss;
		oss << "Invalid ask price:" << ask;
		logUtils::printMsg(oss.str().c_str(), logUtils::ERR, "marketSpecific::getBidAskStepsSpread");
		return false;
	}
	unsigned int tmp = bid;
	while(tmp < ask) {
		tmp += getPriceStep(tmp, true);
		++spread;
	}
	if(tmp > ask) {
		logUtils::printMsg("getPriceStep error", logUtils::ERR, "marketSpecific::getBidAskStepsSpread");
		return false;
	}
	return true;
}


void marketSpecific::getStrikesAround(const double market, const unsigned int strikesNumber, unsigned int& down, unsigned int& up) throw()
{
	unsigned int uintMarket = static_cast<unsigned int>(market);
	unsigned int lowBaseStrike = uintMarket - (uintMarket % uintStrikeStep);
	down = lowBaseStrike - uintStrikeStep * ((strikesNumber / 2) - 1);
	up = down + (strikesNumber - 1) * uintStrikeStep;
}


unsigned int marketSpecific::getAboveStrike(const unsigned int strike) throw()
{
	return strike + uintStrikeStep;
}


unsigned int marketSpecific::getBelowStrike(const unsigned int strike) throw()
{
	return strike - uintStrikeStep;
}




const bool marketSpecific::calculateMaxOptionId(const char* optionFileName,const unsigned  int numberOfOptions) throw()
{
	int index = 0;
	ifstream* ifs = parsingUtils::openInputFile(optionFileName);
	if(!ifs) {
		return false;
	}
	unsigned int* options = new unsigned int[numberOfOptions];
	char* optionId = new char[marketSpecific::optionIdLength + 1];
	while(true) {
		string lineStr = "";
		if(!parsingUtils::readLine(*ifs, lineStr)) {
			logUtils::printInitializationError("Parsing error", __FILE__, __LINE__);
			parsingUtils::closeInputFile(ifs);
			delete []options;
			delete []optionId;
			return false;
		}
		if(lineStr == "") {
			parsingUtils::closeInputFile(ifs);
			break;
		}
		istringstream lineStream(lineStr);
		lineStream >> optionId;
		unsigned int intId = atoi(optionId);
		options[index] = intId;
		index++;
	}
	fakeMaxOptionId = recommendedArraySize(options,numberOfOptions);
	ostringstream oss;
	oss<<"fakeMaxOptionId = "<<fakeMaxOptionId;
	logUtils::printMsg(oss.str().c_str(), logUtils::INF, "marketSpecific::calculateMaxOptionId");
	delete []options;
	delete []optionId;
	return true;
}

const unsigned int marketSpecific::recommendedArraySize(const unsigned int* options,const unsigned int numberOfOptions) throw()
{
	unsigned int arraySize = 1;
	//calculating 10 ^ optionIdLength
	for (unsigned int i = 0; i < optionIdLength; i++) {
		arraySize *= 10;
	}
	bool alreadyExist;
	int num;
	unsigned int *temp = 0;
	for(unsigned int i = 10; i < arraySize; i *= 10) {
		alreadyExist = false;
		if(numberOfOptions <= i) {
			temp = new unsigned int[i];
			//initialization
			for (unsigned int j = 0; j < i; j++) {
				temp[j] = 0;
			}
			//running over the options' id
			for (unsigned int j = 0; j < numberOfOptions; j++) {
				num = options[j]%i;
				//the case the suffix of the option id appear twice
				if (temp[num] > 0) {
					alreadyExist = true;
					ostringstream oss;
					oss<<"the suffix which exist twice is "<<num;
					logUtils::printMsg(oss.str().c_str(), logUtils::INF, "marketSpecific::recommendedArraySize");
					delete[] temp;
					temp = 0;
					break;
				}
				temp[num]++;
			}
			if (!alreadyExist) {
				delete[] temp;
				return i;
			}
			if(temp) {
				delete[] temp;
				temp = 0;
			}
		}
	}
	return arraySize;
}

unsigned int marketSpecific::getOptionId(const char* optionIdStr) throw()
{
	return atoi(optionIdStr);
}

