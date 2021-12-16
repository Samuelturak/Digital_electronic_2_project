# Kapacitný senzor vlhkosti pôdy v1.2

## Základný prehľad:

Tento senzor je analógový a meria vlhkosť pôdy pomocou tzv. kapacitného snímania. Veľkosť kapacity sa mení s meniacou sa saturáciou vody v pôde. Kapacita je následne premenená na výstupnú úroveň napätia, ktorá so pohybuje od 1,2 V do 3 V. Výhoda takéhoto senzoru je v tom, že je vyrobený z materiálu odolnému voči korózii, čo zabezpečuje jeho pomerne dlhú životnosť. Celý senzor je možno vidieť na obr. 1. Doska obsahuje aj regulátor napätia pre rozšírenie možností riadenia. V prípade potreby by sa senzor dal pripojiť aj na digitálne PINy Arduina cez AD prevodník.

![obrázek1](https://www.agc.sk/thumbs-4/135821_imgs-Nov%C3%A9-kapacitn%C3%BD-%C4%8Didlo-p%C3%B4dnej-vlhkosti-nie-je-%C4%BEahk%C3%A9.jpeg)
![obrázek2](https://techfun.sk/wp-content/uploads/2020/06/c.png)  
obr. Senzor a odporúčaná úroveň vnorenia do pôdy.

## Parametre a špecifikácia:

- Podpora 3-pinového rozhrania pre ovládanie
- Analógový výstup
- Riadiace napätie 3,3-5,5 VDC
- Výstupné napätie 0-3,3 VDC
- Pracovný prúd 5mA
- Rozhranie typu PH2.0-3P
- Veľkosť: 99x16 mm
- Váha: 15 g

## Popis schémy zapojenia:

Schému zapojenia senzora je možno vidieť na obr. 3. Senzor obsahuje frekvenčný oscilátor vstavaný do časovača 555. Generovaný obdĺžnikový signál je posielaný do kapacitného senzora, ktorého reaktancia tvorí delič napätia s pasívnym 10kΩ rezistorom. S narastajúcou vlhkosťou pôdy narastá aj kapacita senzoru, čím sa znižuje napätie na výstupnom PINe.

![obrázek](https://how2electronics.com/wp-content/uploads/2019/11/Capacitive-Soil-Moisture-Sensor-Schematic-1.png)  
obr. Zjednodušená schéma zapojenia senzoru.

## Zapojenie senzoru pre Arduino UNO:

![obrázek](https://how2electronics.com/wp-content/uploads/2019/11/Capacitive-Soil-Moisture-Sensor-Arduino-Circuit.jpg)

## Použité zdroje:

(https://how2electronics.com/interface-capacitive-soil-moisture-sensor-arduino/)  
(https://www.sigmaelectronica.net/wp-content/uploads/2018/04/sen0193-humedad-de-suelos.pdf)
















