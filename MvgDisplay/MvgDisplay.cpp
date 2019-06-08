#include "MvgDisplay.h"

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <LiquidCrystal_I2C.h>


const char* ssid = "muenchen.freifunk.net/muc_cty"; // replace with your wifi information
const char* password = "";
const String station = "barthstra%DFe"; //replace with your station, go to www.mvg-live.de/ims/dfiStaticAuswahl.svc, enter your station and check the following url

const int updateTimer = 5000;
const int blinkTimer = 1370;

unsigned long lastUpdate = 0;
unsigned long lastBlink = 0;

// 20x4 lcd display. for 16x2 all lcd related values have to be adapted //TODO
// wiring esp -> lcd
//     5v vin -> vcc
//        gnd -> gnd
//        D1  -> scl
//        D2  -> sda
LiquidCrystal_I2C lcd(0x27, 20, 4); //

void setup() {
	//Serial.begin(115200);

	lcd.init();
	lcd.backlight();
	lcd.noAutoscroll();
	lcd.setDelay(100,100);
	lcd.setCursor(0, 0);
	lcd.print("Connecting to Wifi:");
	lcd.setCursor(0, 1);
	lcd.print(String(ssid).substring(0,20));

	lcd.setCursor(0, 3);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		lcd.print(".");
	}

	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("WiFi connected");
	lcd.setCursor(0, 1);
	lcd.print("IP address:");
	lcd.setCursor(0, 2);
	lcd.print(WiFi.localIP());
	lcd.setCursor(0, 3);
	lcd.print("waiting...");
}

void loop() {
	if((millis() - lastUpdate) > updateTimer){
		lastUpdate = millis();
		update();
	}

	// used to see if esp is still alive
	if((millis() - lastBlink) > blinkTimer){
		lastBlink = millis();
		blink();
	}

	delay(10) ;
}

void blink(){
	lcd.setCursor(19, 0);
    lcd.blink();
    delay(blinkTimer/4);
    lcd.noBlink();
}

bool update() {

	lcd.setCursor(0, 3);
	lcd.print(" ... refreshing ... ");

	if(WiFi.status() != WL_CONNECTED){
		// TODO: not the best solution?
		ESP.restart();
		delay(1);
	}


	String rawList = "";
	if(!getDeparturesRaw(station, rawList)) {
		return false;
	}

	std::vector<Departure> departures;
	if(!getDepartureList(rawList, departures)) {
		return false;
	}

	writeToDisplay(departures);

	return true;
}

bool getDeparturesRaw(const String& station, String& result) {

	WiFiClient wifiClient;
	HTTPClient httpClient;

	String url = "http://www.mvg-live.de/ims/dfiStaticAuswahl.svc?haltestelle="
			+ station
			+ "&ubahn=checked&bus=checked&tram=checked&sbahn=checked";

	if (!httpClient.begin(wifiClient, url)) {
		return false;
	} else {
		int httpReturnCode = httpClient.GET();

		// httpCode will be negative on error
		if (httpReturnCode <= 0) {
			return false;
		} else {
			// HTTP header has been send and Server response header has been handled
			// file found at server
			if (httpReturnCode == HTTP_CODE_OK || httpReturnCode == HTTP_CODE_MOVED_PERMANENTLY) {
				result = httpClient.getString();
			}
		}

		httpClient.end();
		return true;
	}
}

bool getDepartureList(String& rawList, std::vector<Departure>& departureList){
	if (rawList.indexOf("Es wurde kein Bahnhof mit diesem Namen gefunden") >= 0){
		return false;
	}

	/*
	 * Example Departure:
	 * <tr class="rowOdd">
	 *     <td class="lineColumn">18</td>
	 *     <td class="stationColumn">
	 *			Schwanseestraﬂe
	 *			<span class="spacer">&nbsp;</span>
	 *     </td>
	 *	   <td class="inMinColumn">1</td>
     *
	 * </tr>
	 */

	String workingRawList = rawList;

	// remove everything but station name, time and departures
	workingRawList = workingRawList.substring(workingRawList.indexOf("departureView\">")+19, workingRawList.indexOf("=\"reloadLink\">")-49);

	// TODO: handle station and time

	// remove everything but departure list
	workingRawList = workingRawList.substring(workingRawList.indexOf("<tr class="));
	// remove clutter
	workingRawList.replace("<span class=\"spacer\">&nbsp;</span>", "");

	int endIndex;
	String current;
	for(size_t i = 0; i < 4; i++) {

		// get current departure from working list and remove it from working list
		endIndex = workingRawList.indexOf("</tr>");
		current = workingRawList.substring(0, endIndex);
		workingRawList = workingRawList.substring(endIndex+4);

		Departure newDeparture;

		// parse the three items
		endIndex = current.indexOf("</td>");
		newDeparture.line = current.substring(current.indexOf("lineColumn")+12, endIndex);
		endIndex = current.indexOf("</td>", endIndex+4);
		newDeparture.destination = current.substring(current.indexOf("stationColumn")+15, endIndex);
		endIndex = current.indexOf("</td>", endIndex+4);
		newDeparture.minutes = current.substring(current.indexOf("inMinColumn")+13, endIndex);

		newDeparture.line.trim();
		newDeparture.destination.trim();
		newDeparture.minutes.trim();

		if(newDeparture.line.length()==0 || newDeparture.destination.length()==0 || newDeparture.minutes.length()==0){
			// ignoring
			continue;
		}

		departureList.push_back(std::move(newDeparture));
	}

	return true;
}

bool writeToDisplay(std::vector<Departure>& departures) {

	if(lcd.getWriteError() !=0){
		lcd.init();
	}

	size_t linesToWrite = 4;

	if(departures.size() < linesToWrite){
		// TODO: print warning
		linesToWrite = departures.size();
	}

	Departure current;
	String toWrite;
	for(size_t i = 0; i<linesToWrite; i++){
		current = departures.at(i);
		toWrite = current.line+"   ";
		toWrite = toWrite.substring(0, 4);

		toWrite += current.destination+"                ";
		toWrite = toWrite.substring(0, 17);

		toWrite += " "+current.minutes+"    ";
		toWrite = toWrite.substring(0, 20);

		lcd.setCursor(0, i);
		lcd.print(toWrite);
	}

	return true;
}

