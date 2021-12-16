# Real time clock DS 1307

Mikrokontrolér udržiava sekundy, minúty, hodiny, dni, mesiace a roky ako informáciu. Automaticky započítava počty dni v mesiacoch a takisto prechodné roky. Hodiny operujú v 24 aj v 12 hodinovom formáte s AM/PM indikátorom.

## Parametre a špecifikácia:

- Dvoj-zbernicové I2C rozhranie
- Riadiace napätie : 3,3V – 5V
- Obsahuje batériovú zálohu
- 1Hz výstupný pin
- 56 bytová of neprelietavá (non-volatile) pamäť dostupná pre použivateľa
- Pracovný prúd: 1,5 mA
- Pracovná teplota: 0°C do +70°C

## Piny:

**VCC:** 3,3V až 5,5V  
**GND:** uzemnenie  
**BAT:** náhradné napájanie pre 3V lítiovú batériu, využíva sa na udržiavanie presného času, pre prípád keď hlavný zdroj energie je prerušený; **keď sa nevyužíva tento pin, musí byť uzemnený**  
**SQW pin:** výstup sú 4 obdĺžnikové signály: 1Hz, 4kHz, 8kHz alebo 32kHz; **ktoré sú programovateľné**  
**SLC:** hodinový vstup pre I2C rozhranie  
**SDA:** data vstup/výstup pre I2C rozhranie, vyžaduje vonkajší pull-up rezistor  
**DS:** pin má výstup teploty, pokiaľ je teplotný senzor pripojený  

![obrázek](https://lastminuteengineers.b-cdn.net/wp-content/uploads/arduino/DS1307-RTC-Module-Pinout.png)

## Popis fungovania:

Zariadenie operuje ako slave-device na I2C komunikácií. Clock signál má štandart mód (100kHz) a rýchly mód (400kHz). Zariadenie operuje na dvoch módoch:

**1. Slave receiver mód (write mode):** prvý byte je adresný 1101000, nasleduje R/W bit (Write = 0), následne ACK bit. Ďaľši byte master odošle slovo, ktoré je naša adresa (viď. Obrázok 2.) a následne master môže odoslať 0 alebo viac bytov (v našej komunikácii nemusíme, v každom musí byť ACK bit).

**2 .Slave transmitter mód (read mode):** prvý byte sa líši jedine v R/W bite (Read = 1), následne slave odosiela byty, ktoré reprezentujú data. Register pointer ktorý reprezentuje adresný byte sa postupne pričítava po každom prečítanom byte, takže každý byte je nová informácia. Zobrazenie komunikácie je na obrázku 3..

![tabulka_1](https://github.com/david3891/Digital-electronic-2/blob/main/dokumentace/obr%C3%A1zky/RTC_DS1307_1.PNG)

![tabulka_2](https://github.com/david3891/Digital-electronic-2/blob/main/dokumentace/obr%C3%A1zky/RTC_DS1307_2.PNG)

## Zapojenie:

![zapojení](http://labpeers.com/php-file-download-script/download.php?filename=../media/catalog/product/f/i/file.jpg)

## Zdroje:

(https://pdf1.alldatasheet.com/datasheet-pdf/view/226599/MAXIM/DS1307.html)  
(https://pdf1.alldatasheet.com/datasheet-pdf/view/58481/DALLAS/DS1307.html)  
(https://wiki.dfrobot.com/Real_Time_Clock_Module__DS1307___SKU_DFR0151_)  
(https://lastminuteengineers.com/ds1307-rtc-arduino-tutorial/)  
(https://www.google.cz/?hl=cs)





















