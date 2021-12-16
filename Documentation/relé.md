# Relé

## Jednokanálový relé modul

### Typy:

- Opticky oddelené relé
- Diaľkovo ovládané relé
- Relé s časovačom oneskorenia
- Atď.

### Riadenie:

Relé možno ovládať priamo mikro-kontrolérom, prípadne externe(napr. diaľkový ovládač). Má viacero kontaktov ako na vstupe, tak aj na výstupe:

**Vstup:**
- GND
- IN – Signál spínajúci relé(napr. PIN C5)
- VCC – Napájacie napätie modulu(napr. 3V, 5V)

**Výstup:**
- COM – „Common terminal“, relé je k nemu pripojené vždy z minimálne jedného kontaktu
- NO – „Normally open“ kontakt je rozpojený s COM pri nulovom napätí na vstupe
- NC – „Normally closed“ kontakt je zopnutý s COM pri nulovom napätí na vstupe

### Predpokladané parametre:

- Možno spínať až 250VAC a 30 VDC
- Max. prietok prúdu cez relé môže byť až 10 A. Pri osadení do DPS je prietok limitovaný na 3 A
- Prúdový odber až 60 mA pri zopnutí

### Príklad zapojenia:

![obrázek](https://navody.dratek.cz/images/technikuv_blog/navody/spinaci_prvky/one_channel_relay.png)

## Viackanálový relé modul

### Riadenie:

Princíp riedenia je úplne rovnaký. Jediné čo sa nám zmení je, že nám pribudne jeden a viac riadiacich vstupov(napr. IN2, IN3, ...).

### Predpokladané parametre:

Budú opäť z veľkej časti rovnaké. Rozdielny je len prúdový odber pri zopnutí(napr. 100+ mA). Pre Arduino UNO je max. prúdový odber 200 mA.

### Možné komplikácie:

- Prúdový odber pri zopnutí jedného relé môže byť až 60 mA, pri viacerých relé odber narastá. Pre Arduino UNO je povolený prúdový odber(DC) pre VCC a GND piny maximálne do 200 mA. Napájanie z USB limituje spotrebu na 500mA no je zdieľané pre všetky komponenty na doske. Tak, či onak, prúdový odber môže byť jemne prekročený aj do ~400 mA.
- Pre I/O piny je povolený prúdový odber(DC) do 40 mA(Absolútny limit, mal by sa používať len v krajných prípadoch, nie v našom).

### Príklad zapojenia:

![obrázek](https://navody.dratek.cz/images/technikuv_blog/navody/spinaci_prvky/vicekanal.png)

### Použité zdroje:

(https://navody.dratek.cz/zaciname-s-arduinem/spinaci-prvky-rele-tranzistory-a-tranzistorova-pole.html)  
(https://www.mouser.com/pdfdocs/PanasonicPhotomosRelaysWhitePaper.PDF)



















