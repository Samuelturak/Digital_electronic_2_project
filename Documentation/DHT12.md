# DHT12

Senzor na meranie vlhkosti a teploty prostredia. Komunikácia prebieha cez protokol I2C alebo cez Single bus zapojenie.

## Piny:

**VCC:** 2,7V až 5,5V  
**GND:** uzemnenie  
**SCL:** hodinový vstup pre I2C rozhranie  
**SDA:** data vstup/výstup pre I2C rozhranie  

## I2C komunikácia:

- je zapnutá keď SCL pin je v stave HIGH  
- V prvej časti komunikácie master odosiela adresný frame, ktorý predáva informáciu o typu dát, ktorú mu slave má poskytnúť, tento byte sa skladá z 8 bitov, z toho je posledný R/W bit obsahujúci informáciu či bude informácia od mastera čítaná alebo zapisovaná (v tomto prípade bude zapisovaná, takže hodnota tohto bitu je 0) a následne ACK/NAK bit. Tento adresný frame má mať dekadickú hodnotu 184. Ďalší byte ktorý bude odoslaný obsahuje informáciu o adrese registru, táto informácia hovorí akú informáciu má slave preposlať masterovi v ďalšej časti komunikácie (vlhkosť alebo teplota).  
- V ďalšej časti komunikácie slave odošle ten istý adresný byte, až na R/W bit, ktorý je tentoraz 1, čo znamená že dáta budú čítané. Ďalej slave odošle byte, podľa adresy registru, ktorú master poslal a posiela až dokým nejaký byte nebude mať hodnotu ACK/NAK 1.

![obrázek](https://github.com/tomas-fryza/Digital-electronics-2/blob/master/Labs/08-i2c/Images/i2c_protocol.jpg)

   | **Adresa registru** | **Popis adresy**                     |
   |         :-:         |        :-:                           |
   | 0x00                | vlhkosť                              |
   | 0x01                | vlhkosť (zlomková čásť )             |
   | 0x02                | teplota                              |
   | 0x03                | teplota (zlomková část)              |
   | 0x04                | kontrola (súčet všetkých informácií) |
   
   ## Zapojenie:
   
   ![zapojení](https://content.instructables.com/ORIG/FH0/4TQU/JB2S55SA/FH04TQUJB2S55SA.png?auto=webp&fit=bounds&frame=1&width=1024)
   
   ## Zdroje:
   (https://github.com/tomas-fryza/Digital-electronics-2/tree/master/Labs/08-i2c)  
   (https://github.com/tomas-fryza/Digital-electronics-2/blob/master/Docs/dht12_manual.pdf)  
   (https://www.google.cz/?hl=cs)
   
   



   
