import lcddriver
import requests
import time
from bs4 import BeautifulSoup

def set_line(line_num, content):
    lcd.lcd_display_string(content, line_num)

def clear():
    lcd.lcd_clear()

def get_departures(station):
    url = "http://www.mvg-live.de/ims/dfiStaticAuswahl.svc?haltestelle="+station+"&ubahn=checked&bus=checked&tram=checked&sbahn=checked"
    page = requests.get(url)
    soup = BeautifulSoup(page.content, 'html.parser')
    station = soup.find('td', class_='headerStationColumn').get_text()
    time = soup.find('td', class_='serverTimeColumn').get_text()

    departuresOdd = soup.find_all('tr', class_='rowOdd')
    departuresEven = soup.find_all('tr', class_='rowEven')

    departures = [None]*(len(departuresOdd)+len(departuresEven))
    departures[::2] = departuresOdd
    departures[1::2] = departuresEven

    departures_trimmed = [None]*len(departures)

    for x in range(0, len(departures)):
        values = [None]*3
        values[0] = departures[x].find('td', class_='lineColumn').get_text().strip()
        values[1] = departures[x].find('td', class_='stationColumn').get_text().strip()
        values[2] = departures[x].find('td', class_='inMinColumn').get_text().strip()
        departures_trimmed[x] = values

    return departures_trimmed

## -- main -- ##

lcd = lcddriver.lcd()
stations = ['barthstra%DFe'] #['barthstra%DFe']
encoding = 'UTF-8'
stuffing = '                    '

while True:
    try:
        # stations
        for station in stations:
            print("getting station "+station)           
            try:
                set_line(4, '...  refreshing  ...')
                departures = get_departures(station)
                clear()                
                for x in range(0, 4):
                    current_departure = departures[x]
                    line = current_departure[0]+stuffing
                    destination = current_departure[1]+stuffing
                    minutes = current_departure[2]+stuffing
                    set_line(x+1, line[:3] + " "+destination[:13]+" "+str(minutes)[:2])
                    print(line[:3] + " "+destination[:13]+" "+str(minutes))
                print('')
            except Exception as e:
                print('In outer try: '+str(e))
                continue

        # sleep before new round
        time.sleep(15)

    except Exception as e:
        print('In outer try: '+str(e))
        clear()
        lcd = lcddriver.lcd()
        continue
