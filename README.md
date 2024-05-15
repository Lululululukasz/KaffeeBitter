# Kaffee BITter

With the help of an ESP32, a Loadcell and the HX711 ADC Amplifier, we have built a scale that determines and displays the fill level of the coffee pot in the IuE Fachschaft:  

[Coffee](https://fsie-kiel.de/coffee)

## Installation
  
Software:
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- [HX711 Libary](https://esp-idf-lib.readthedocs.io/en/latest/groups/hx711.html)
  
Hardware:  
- ESP32  
- [HX711 Loadcell](https://www.reichelt.de/de/de/entwicklerboards-a-d-wandler-waage-10-kg-debo-hx711-10-p316339.html?PROVID=2788&gclid=EAIaIQobChMI-M7jjLfOgwMVpZ9oCR2JgADwEAQYASABEgIWA_D_BwE&&r=1)

## How does it Work
Hardware:

1. You have to build a [scale](https://randomnerdtutorials.com/esp32-load-cell-hx711/) with the loadcell and the HX711 amplifier 
2. A coffee can or similar 

Software:
1. Change in the menuconfig the weight of your coffee can and the size of your cups
2. Connect your ESP32 to your WI-FI
3. Define your API-Endpoint
4. Build an Aplication to show the value of the scale:  
{"state":"filled","temperature":"hot","cups":5,"stamp":"2024-01-08T14:27:11.000Z"}

## Pin Config:
- GND (BLACK) to GND
- DT (Weight) to 19
- SCK (Yellow) to 18
- VCC (RED) to 3v3

## Contributing

Pull requests are welcome. For major changes, please open an issue first
to discuss what you would like to change.

Please make sure to update tests as appropriate.

## License

MIT License

Copyright (c) [2024] [Lukasz Szupka & Helene Lüning & Veronica Zylla]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
