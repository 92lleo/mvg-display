#ifndef _MvgDisplay_H_
#define _MvgDisplay_H_

#include "Arduino.h"

struct Departure {
	String line;
	String destination;
	String minutes;
};

void blink();
bool update();
bool getDeparturesRaw(const String& station, String& result);
bool getDepartureList(String& rawList, std::vector<Departure>& departureList);
bool writeToDisplay(std::vector<Departure>& departureList);

#endif /* _MvgDisplay_H_ */
