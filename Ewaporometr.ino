//--------------BILIOTEKI--------------
//Silnik
#include <AccelStepper.h>
#include <MultiStepper.h>
//Czas
#include <Wire.h>
#include "RTClib.h"
RTC_DS1307 czas_rtc;
//Wyświetlacz LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
//Klawiatura
#include <Keypad.h>
//Timer
#include <Timers.h>
//Zapis
#include <SPI.h>
#include <SD.h>

//-------------ZMIENNE-GLOBALNE------------
//--------------------Zapis na karte zmienne globalne-------------------
//Podłączenie karty
//MOSI - pin 51
//MISO - pin 50
//SCK - pin 52
//CS - pin 53
File plik;
int obrut = 0; 
short kierunekSilnik = 0; 
int pomocSD = 0; 
int krok = 0; 
short pomiar = 0; 
short stopSilnik = 1; 
short p_kalibracja = 0; 
int p_pracasilnik = 0; 
int stopSilnikPraca = 1; 
int odliczanie = 0; 
int minuty = 0; 
int sek = 0; 

//-----------Zadeklarowanie dwóch silników na odwrotnych pinach w celu kręcenia silnikiem w różne strony bez awarii
AccelStepper PrzeciwnieDoZegara(AccelStepper::FULL2WIRE, 10, 11);
AccelStepper ZgodnieDoZegara(AccelStepper::FULL2WIRE, 11, 10);
//-------------------------Zmienne klawiatura--------------------------
int numer = 0; 
int pos = 0; 
int zmiana = 0; 
const byte ROWS = 4;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {     
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {6, 7, 8, 9}; 
byte colPins[COLS] = {2, 3, 4, 5}; 
//------klawiatura inicjalizacja---------------
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
//------wyświetlacz inicjalizacja---------------
#define BACKLIGHT_PIN 3
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7);

//--------Zmienne-Czas------------------------------
int rok = 0;
int miesiac = 0;
int dzien = 0;
int godzina = 0;
int minuta = 0;
char w_data[10]; 
char w_godzina[5]; 
//--------Zmienne-Czestotliwosc-Badan-----------------------------
int cz_badania = 6; 
int cz_badania_t = 0; 
char w_badania[2]; 
//-------Zmienna-Pomocnicza-do-zabezpieczenia-Menu
short pomoc_menu = 0; 
//-------Zmienna-Do-Timera--------------------------------
Timer pierwszyTimer; 
Timer kaliSilnika; 
//-------Zmienna-Odczyt-Napiecia--------------------------------
int odczytanaWartosc = 0;
float napiecie = 0;
//-------Obsługa-błędów--------------------------------
short wyllcd = 0; 
//---------------------------------------------------

void setup()
{
  Serial.begin(9600);
  //-----------------Wyświetlacz-musi-byc-pierwszy-----------------
  lcd.begin(16, 2);
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd.setBacklight(HIGH);
  pinMode(22, OUTPUT);
  //-----------------Tekst-uruchamiany-przy-starcie-urzadznenia-----
  lcd.setCursor(2, 0);
  lcd.print("EWAPOROMETR");
  lcd.setCursor(4, 1);
  lcd.print("POLARNY");
  delay(3000);
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("UKW");
  lcd.setCursor(1, 1);
  lcd.print("MECHATRONIKA");
  delay(3000);
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("INICJALIZACJA");
  delay(3000);
  //-----------------Silnik----------------------------------------
  PrzeciwnieDoZegara.setMaxSpeed(6000); 
  PrzeciwnieDoZegara.setSpeed(6000); 
  ZgodnieDoZegara.setMaxSpeed(6000); 
  ZgodnieDoZegara.setSpeed(6000); 
  //-----------------Zapis na karte--------------------------------
  String badanie = "pomiary.txt";
  testSD(); // Inicjalizacja karty SD
  sprawdzeniePliku(badanie);
  //-----------------Czas------------------------------------------
  Wire.begin();
  czas_rtc.begin();
  //-----------ROZPOCZECIE--PROGRAMU-WYSWIETLANIE DANYCH------------------
  DateTime now = czas_rtc.now(); 
  lcd.clear();
  delay(500);
  lcd.setCursor(0, 0);
  lcd.print("Data:");
  lcd.print(now.day(), DEC);
  lcd.print('.');
  lcd.print(now.month(), DEC);
  lcd.print('.');
  lcd.print(now.year(), DEC);
  lcd.setCursor(0, 1);
  lcd.print("Czas:");
  lcd.print(now.hour(), DEC);
  lcd.print(':');
  lcd.print(now.minute(), DEC);
  //lcd.print('.');
  delay(5000);
  lcd.clear();
  lcd.print("Czestotliwosc");
  lcd.setCursor(0, 1);
  lcd.print("badan na h: ");
  lcd.print(cz_badania);
  delay(5000);
  //-------------------------MENU-------------------------
  lcd.setCursor(0, 0);
  lcd.clear();
  lcd.print("Zmienic Date T/N");
  pierwszyTimer.begin(SECS(30));
}

void loop()
{
  //-------------------------MENU-------------------------
  char customKey = customKeypad.getKey();
  if ((pierwszyTimer.available() && zmiana == 0) || (pierwszyTimer.available() && zmiana == 5))
  {
    pierwszyTimer.restart();
    pierwszyTimer.time(STOP);
    zmiana = 10;
    lcd.clear();
  }
  if (customKey)
  {
    pierwszyTimer.restart();
    pierwszyTimer.time(STOP);
    if (customKey == 'A')
    {
      lcd.clear();
      delay(500);
      lcd.setCursor(0, 0);
      if (zmiana == 0)
      {
        lcd.print("Wprowadz Date");
        zmiana++;
      }
      else if (zmiana == 1) 
      {
        lcd.print("Wprowadz Date");
        if (pos < 10)
        {
          zmiana = 0;
          pos = 0;
          numer = 0;
          lcd.clear();
          delay(200);
          lcd.print("Blad za malo cyfr");
          delay(2000);
          lcd.clear();
          delay(200);
          pomoc_menu = 2;
          lcd.print("Nacisnij A");
          return;
        }
        else
        {
          lcd.clear();
          delay(500);
          lcd.setCursor(0, 0);
          //----Konwersja--Daty---
          lcd.print("Konwersja Daty");
          lcd.setCursor(0, 1);
          lcd.print("Prosze Czekac");
          unsigned int d1, d2, d3, d4, d5, d6, d7, d8;
          d1 = (int)w_data[0];
          d2 = (int)w_data[1];
          d3 = (int)w_data[3];
          d4 = (int)w_data[4];
          d5 = (int)w_data[6];
          d6 = (int)w_data[7];
          d7 = (int)w_data[8];
          d8 = (int)w_data[9];
          d1 = d1 - 48; d2 = d2 - 48; d3 = d3 - 48; d4 = d4 = d4 - 48; d5 = d5 - 48; d6 = d6 - 48; d7 = d7 - 48; d8 = d8 - 48; 

          dzien = 10 * d1 + d2;
          miesiac = 10 * d3 + d4;
          rok = 1000 * d5 + 100 * d6 + 10 * d7 + d8;

          if (dzien > 31 || miesiac > 12 || rok < 2015)
          {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Bledna data");
            lcd.setCursor(0, 1);
            lcd.print("Wpisz ponownie");
            zmiana = 0;
            pos = 0;
            numer = 0;
            dzien = 0;
            miesiac = 0;
            rok = 0;
            delay(3000);
            lcd.clear();
            lcd.setCursor(0, 0);
            pomoc_menu = 2;
            lcd.print("Nacisnij A");
            return;
          }
          delay(1000);
          lcd.clear();
          delay(500);
          lcd.setCursor(0, 0);
          lcd.print("Wprowadz Czas");
          numer = 0;
          pos = 0;
          if (customKey == 'A')
          {
            zmiana = 3;
          }
        }
      }
      else if (zmiana == 2)
      {
        lcd.clear();
        delay(500);
        lcd.setCursor(0, 0);
        lcd.print("Wprowadz Czas");
        numer = 0;
        pos = 0;
        if (customKey == 'A')
        {
          zmiana = 3;
        }
      }
      else if (zmiana == 3)
      {
        if (pos < 5)
        {
          zmiana = 2;
          pos = 0;
          numer = 0;
          lcd.clear();
          delay(200);
          lcd.print("Blad za malo cyfr");
          delay(2000);
          lcd.clear();
          delay(200);
          pomoc_menu = 3;
          lcd.print("Nacisnij A");
          return;
        }
        //konwersja godziny

        lcd.clear();
        delay(500);
        lcd.print("Konwersja Godziny");
        lcd.setCursor(0, 1);
        lcd.print("Prosze Czekac");
        int g1, g2, g3, g4;
        g1 = (int)w_godzina[0];
        g2 = (int)w_godzina[1];
        g3 = (int)w_godzina[3];
        g4 = (int)w_godzina[4];
        g1 = g1 - 48; g2 = g2 - 48; g3 = g3 - 48; g4 = g4 - 48;
        godzina = 10 * g1 + g2;
        minuta = 10 * g3 + g4;

        if (godzina > 23 || minuta > 59)
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Bledny czas");
          lcd.setCursor(0, 1);
          lcd.print("Wpisz ponownie");
          zmiana = 2;
          pos = 0;
          numer = 0;
          godzina = 0;
          minuta = 0;
          delay(3000);
          lcd.clear();
          lcd.setCursor(0, 0);
          pomoc_menu = 3;
          lcd.print("Nacisnij A");
          return;
        }
        delay(500);
        czas_rtc.adjust(DateTime(rok, miesiac, dzien, godzina, minuta, 0));
        DateTime now = czas_rtc.now(); 
        lcd.clear();
        delay(500);
        lcd.setCursor(0, 0);
        lcd.print("Data:");
        lcd.print(now.day(), DEC);
        lcd.print('.');
        lcd.print(now.month(), DEC);
        lcd.print('.');
        lcd.print(now.year(), DEC);
        lcd.setCursor(0, 1);
        lcd.print("Czas:");
        lcd.print(now.hour(), DEC);
        lcd.print(':');
        lcd.print(now.minute(), DEC);
        delay(2000);
        // Pytanie o zmiane badania
        lcd.clear();
        delay(500);
        lcd.setCursor(0, 0);
        lcd.print("Zmienic co ile h");
        lcd.setCursor(0, 1);
        lcd.print("jest badanie T/N");
        pos = 0;
        numer = 0;

        if (customKey == 'A')
        {
          zmiana = 5;
        }
        return;
      }
      else if ((zmiana == 6) || (pomoc_menu == 1 && zmiana == 5)) 
      {
        pomoc_menu = 5;
        lcd.clear();
        delay(500);
        lcd.print("Konwersja czest.");
        lcd.setCursor(0, 1);
        lcd.print("Prosze Czekac");
        delay(1000);
        int b1, b2;
        b1 = (int)w_badania[0];
        b2 = (int)w_badania[1];
        if (pos == 2)
        {
          b1 = b1 - 48; b2 = b2 - 48;
          cz_badania = 10 * b1 + b2;
          pos = 0;
        }
        else if (pos == 1)
        {
          b1 = b1 - 48; b2 = 0;
          cz_badania = 10 * b2 + b1 ;
          pos = 0;
        }
        if (cz_badania > 24 || cz_badania == 0 )
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Bledna czest.");
          lcd.setCursor(0, 1);
          lcd.print("Wpisz ponownie");
          zmiana = 5;
          pos = 0;
          numer = 0;
          delay(3000);
          lcd.clear();
          lcd.setCursor(0, 0);
          pomoc_menu = 4;
          lcd.print("Nacisnij A");
          return;
        }
        else
        {
          lcd.clear();
          delay(500);
          lcd.print("Czestotliwosc");
          lcd.setCursor(0, 1);
          lcd.print("badan na h: ");
          lcd.print(cz_badania);
          delay(5000);
          lcd.clear();
          zmiana = 10;
        }
      }
      else if (zmiana == 4)
      {
        lcd.clear();
        delay(500);
        lcd.setCursor(0, 0);
        lcd.print("Wprowadz co ile");
        zmiana = 5;
      }
      else if (zmiana == 5)// zmiana czasu
      {
        lcd.clear();
        delay(500);
        lcd.setCursor(0, 0);
        lcd.print("Wprowadz co ile ");
      }
      else
      {
        lcd.clear();
        delay(500);
        lcd.setCursor(0, 0);
        lcd.print("ZRESETUJ");
      }
    }
    else if ((zmiana == 1 || zmiana == 3 || zmiana == 5) && (customKey == '1' || customKey == '2' || customKey == '3' || customKey == '4' || customKey == '5' || customKey == '6' || customKey == '7' || customKey == '8' || customKey == '9' || customKey == '0'))
    {
      if (zmiana == 1 && numer < 10) 
      {
        lcd.setCursor(pos, 1);
        w_data[numer] = customKey;
        lcd.print(w_data[numer]);
        pos++;
        numer++;
        if (numer == 2 || numer == 5)
        {
          pos++;
          numer++;
        }
      }

      else if (zmiana == 3 && numer < 5) 
      {
        lcd.setCursor(pos, 1);
        w_godzina[numer] = customKey;
        lcd.print(w_godzina[numer]);
        pos++;
        numer++;

        if (numer == 2)
        {
          pos++;
          numer++;
        }
      }
      else if (zmiana == 5) 
      {
        lcd.setCursor(pos, 1);
        w_badania[numer]  = customKey;
        lcd.print(w_badania[numer]);
        pos++;
        numer++;
        Serial.println("Pozycja =");
        Serial.println(pos);
        Serial.println("zmiana =");
        Serial.println(zmiana);
        pomoc_menu = 1;

        if (pos > 2)
        {
          pos = 0;
          numer = 0;
          lcd.clear();
          delay(500);
          lcd.setCursor(0, 0);
          lcd.print("za duzo cyfr");
          delay(1000);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.println("Wprowadz co ile");
          return;
        }
        else if (pos == 2)
        {
          zmiana = 6;
        }
        return;
      }
    }
    else if (customKey == 'C')
    {
      lcd.clear();
      pos = 0;
      numer = 0;
      pomoc_menu = 0;
      lcd.setCursor(0, 0);
      if (zmiana == 0)
      {
        lcd.print("Zmienic Date T/N");
      }
      else if (zmiana == 1)
      {
        lcd.print("Wprowadz Date");
      }
      else if (zmiana == 2)
      {
        lcd.print("Wprowadz Czas");
      }
      else if (zmiana == 3)
      {
        lcd.print("Wprowadz Czas");
      }
      else if (zmiana == 4)
      {
        lcd.clear();
        delay(500);
        lcd.setCursor(0, 0);
        lcd.print("Zmienic co ile h");
        lcd.setCursor(0, 1);
        lcd.print("jest badanie T/N");
      }
      else if (zmiana == 5 || zmiana == 6)
      {
        lcd.print("Wprowadz co ile");
        zmiana = 5;
      }
      else if (zmiana == 6 && pomoc_menu == 5)
      {
        lcd.clear();
        zmiana == 10;
      }
    }
    else if (customKey == 'B')
    {
      if (pomoc_menu == 2)
      {
        pomoc_menu = 0;
        zmiana = 0;
      }
      else if (pomoc_menu == 3)
      {
        pomoc_menu = 0;
        zmiana = 2;
      }
      else if (pomoc_menu == 4)
      {
        pomoc_menu = 0;
        zmiana = 4;
      }
      else if (zmiana == 0 && pomoc_menu != 2 ) 
      {
        pos = 0;
        numer = 0;
        zmiana = 4;
        lcd.clear();
        delay(500);
        lcd.setCursor(0, 0);
        lcd.print("Zmienic co ile h");
        lcd.setCursor(0, 1);
        lcd.print("jest badanie T/N");
      }
      else if (zmiana == 4)
      {
        pierwszyTimer.time(STOP);
        zmiana = 10;
        lcd.clear();
        return;
      }
      else if (zmiana == 5)
      {
        pierwszyTimer.time(STOP);
        zmiana = 10;
        lcd.clear();
      }
    }
    else 
    {
      if (zmiana == 0)
      {
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Bad jeszcze raz");
        delay(3000);
        pos = 0;
        numer = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Zmienic Date T/N");
      }
      else if (zmiana == 1)
      {
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Bad jeszcze raz");
        delay(3000);
        pos = 0;
        numer = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wprowadz Date");
      }
      else if (zmiana == 2 || zmiana == 3)
      {
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Bad jeszcze raz");
        delay(3000);
        pos = 0;
        numer = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wprowadz Czas");
      }
      else if (zmiana == 4)
      {
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("Bad jeszcze raz");
        delay(3000);
        pos = 0;
        numer = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Zmienic co ile h");
        lcd.setCursor(0, 1);
        lcd.print("jest badanie T/N");
      }
    }
    return;
  }
  //----------------------------------------------------------------------------------------------------------------------------------------
  if (zmiana == 10) 
  {
    pierwszyTimer.time(STOP);
    lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
    lcd.setBacklight(LOW);
    digitalWrite(39, LOW); 
    digitalWrite(41, HIGH); 
    digitalWrite(22, HIGH);
    zmiana = 11;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("KALIBRACJA");
  }
  if (zmiana == 11)
  {
    odczytanaWartosc = analogRead(A0);
    napiecie = odczytanaWartosc * (5.0 / 1023.0); 
    if (napiecie < 4)
    {
      digitalWrite(22, LOW);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("B - 101");
      zmiana = 15;
    }
    else if (napiecie >= 4)
    {
      zmiana = 12 ;
    }
  }

  if (zmiana == 12)//Kalibracja
  {
    if (kierunekSilnik == 0) /
    {
      PrzeciwnieDoZegara.runSpeed();
    }
    if (kierunekSilnik == 1) 
    {
      ZgodnieDoZegara.runSpeed();
    }
    if (digitalRead(40) == 0 && p_kalibracja == 0) 
    {
      kierunekSilnik = 0;
      p_kalibracja = 1;
    }
    if (digitalRead(38) == 0 && (p_kalibracja == 0 || p_kalibracja == 1))
    {
      if (stopSilnik == 1)
      {
        delay(2000);
        stopSilnik = 2;
      }
      kierunekSilnik = 1;
      p_kalibracja = 2;
    }
    if (digitalRead(36) == 1 && p_kalibracja == 2) 
    {
      Serial.println("jestem tu pin 36 Kalibracja");
      p_kalibracja = 3;
      stopSilnik = 1;
    }
    if (p_kalibracja == 3)
    {
      Serial.println("Uruchamiam Timer");
      kaliSilnika.begin(SECS(3));
      Serial.println("Timer Dziala");
      p_kalibracja = 4;
    }
    if (digitalRead(40) == 0 && (p_kalibracja == 2 || p_kalibracja == 3)) 
    {
      short stopczysc = 0; 
      digitalWrite(22, LOW);
      if (stopczysc == 0)
      {
        lcd.clear();
        stopczysc = 1;
      }
      lcd.setCursor(0, 0);
      lcd.print("B - 102");
      stopSilnik = 1;
      p_kalibracja = 6;
      zmiana = 15;
      return;
    }
    if (kaliSilnika.available() && p_kalibracja == 4)
    {
      kaliSilnika.restart();
      kaliSilnika.time(STOP);
      Serial.println("Zatrzymałem się");
      p_kalibracja = 5;
    }
    
    if (p_kalibracja == 5) 
    {
      delay(2000);
      kierunekSilnik = 0;
      p_kalibracja = 0;
      stopSilnik = 1;
      zmiana = 13;
      lcd.clear();
      obrut = 0;
      krok = 0;
      return;
    }
    if (digitalRead(40) == 0 && p_kalibracja == 4)  
    {
      kaliSilnika.restart();
      kaliSilnika.time(STOP);
      delay(2000);
      kierunekSilnik = 0;
      p_kalibracja = 0;
      stopSilnik = 1;
      zmiana = 13;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("SILNIK DZIALA");
      obrut = 0;
      krok = 0;
      return;
    }
  }
  else if (zmiana == 13) 
  {
    if (kierunekSilnik == 0) 
    {
      PrzeciwnieDoZegara.runSpeed();
    }

    if (kierunekSilnik == 1) 
    {
      ZgodnieDoZegara.runSpeed();
    }
    if (digitalRead(40) == 0 && p_pracasilnik == 0) 
    {
      Serial.println("jestem tu pin 40");
      kierunekSilnik = 0;
    }

    if (digitalRead(38) == 0 && (p_pracasilnik == 0 || p_pracasilnik == 1)) 
    {
      Serial.println("jestem tu pin 38");
      Serial.println("A stop silnik to ");
      Serial.println(stopSilnikPraca);
      if (stopSilnikPraca == 1)
      {
        delay(2000);
        stopSilnikPraca = 2;
      }
      kierunekSilnik = 1;
      p_pracasilnik = 2;
    }
    if (digitalRead(36) == 1 && p_pracasilnik == 2) 
    {
      pomiar++;
      zapisWart();
      delay(1000);

      if (stopSilnikPraca == 2)
      {
        delay(2000);
        stopSilnikPraca = 1;
      }
      p_pracasilnik = 1;
      kierunekSilnik = 0;
    }
    if (digitalRead(40) == 0 && (p_pracasilnik == 1 || p_pracasilnik == 2)) 
    {
      short stopczyscP = 0; 
      digitalWrite(22, LOW);
      if (stopczyscP == 0)
      {
        lcd.clear();
        stopczyscP = 1;
      }
      lcd.setCursor(0, 0);
      lcd.print("B - 102");
      stopSilnik = 1;
      zmiana = 15;
      return;
    }
    if (ZgodnieDoZegara.currentPosition() % 20 == 19) 
    {
      krok++;
      if (krok >= 160)
      {
        obrut++;
        krok = 0;
      }
    }
    if (pomiar > 5)
    {
      digitalWrite(22, LOW);
      odliczanie = 1;
      zmiana = 14;
      cz_badania_t= cz_badania -1;
      lcd.clear();
      return;
    }
  }
  else if (zmiana == 14)
  {
    
    digitalWrite(41, LOW); 
    digitalWrite(39, HIGH); 
    
    
    if (odliczanie == 1)
    {
      lcd.setCursor(7, 0);  
      lcd.print ("Godzin ");  
      lcd.print (cz_badania);   
      lcd.print (" ");  
      

      if ( minuty < 0)  
      {
        delay (32);      
        minuty = 59;     
        cz_badania --;    
      }
      if (cz_badania < 0)  
      {
        cz_badania = cz_badania_t;    
      }
      lcd.setCursor (0, 1);  
      lcd.print (minuty);   
      lcd.print (" min ");   
      lcd.setCursor (8, 1);  
      lcd.print (sek);  
      lcd.print (" SEC ");   
      if (sek < 1)     
      {
        minuty --;     
        sek = 60;     
        delay (58);      
      }
      if (sek > 0)     
      {
        delay (988);     
        sek --;      
      }
      if (cz_badania == 0 && minuty == 0 && sek == 0)
      {
        odliczanie = 2;
      }
    }
    if (odliczanie == 2)
    {
      pomiar = 0;
      stopSilnikPraca = 1;
      p_pracasilnik = 0;
      zmiana = 10;
      odliczanie = 0;
      minuty = cz_badania++;
      return;
    }
  }
  else if ( zmiana == 15)
  {
    if (wyllcd == 0)
    {
      digitalWrite(22, LOW);
      digitalWrite(37, HIGH); 
      digitalWrite(41, LOW); 
      digitalWrite(39, LOW);

      lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
      lcd.setBacklight(LOW);
      wyllcd = 1;

    }
    delay (1000);
  }
}

//--------------------------------------------------------------
void sprawdzeniePliku(String nazwaPliku) 
{
  String nnplik = nazwaPliku;
  Serial.println("sprawdzenie istnienia pliku");
  if (SD.exists(nnplik))
  {
    Serial.println("plik istnieje");
    plik = SD.open(nnplik);
    if (plik) 
    {
      Serial.println("Plik otwarty do odczytu");
    }
    else
    {
      pomocSD = 1;
      Serial.println("Plik nie został otwarty! - sprobuj ponownie");
      while (pomocSD == 1)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("B - 100");
        lcd.setCursor(0, 1);
        lcd.print("SPRAWDZ SD");
        delay(2000);
        pomocSD = 1;
        if (wyllcd == 0)
        {
          digitalWrite(37, HIGH); 
          digitalWrite(41, LOW); 
          digitalWrite(39, LOW); 
          lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
          lcd.setBacklight(LOW);
          wyllcd = 1;
        }
      }
    }
    plik.close();
  }
  else
  {
    Serial.println("nie istnieje");
    plik = SD.open(nnplik, FILE_WRITE);
    plik.close();
    Serial.println("pusty plik utworzony");
  }
}
//---------------------------------------------------------------------------------
void zapisWart()
{
  obrut = obrut / 2;
  DateTime now = czas_rtc.now();
  
  String nplik = "pomiary.txt";
  plik = SD.open(nplik, FILE_WRITE);
  plik.print("Pomiar ");
  plik.print(pomiar);
  plik.print("; ");
  plik.print("Liczba Obrotow; ");
  plik.print(obrut);
  plik.print("; ");
  plik.print("Liczba Krokow; ");
  plik.print(krok);
  plik.print("; ");

  //Czas

  plik.print(now.day(), DEC);
  plik.print('.');
  plik.print(now.month(), DEC);
  plik.print('.');
  plik.print(now.year(), DEC);
  plik.print(" ; ");
  plik.print(now.hour(), DEC);
  plik.print(':');
  plik.print(now.minute(), DEC);
  plik.print(':');
  plik.println(now.second(), DEC);
  plik.flush(); 
  plik.close();
  obrut = 0;
  krok = 0;
}
//---------------------------------------------------------------------------------
// inicjalizacja Karty SD
void testSD()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Inicjalizacja");
  lcd.setCursor(0, 1);
  lcd.print("karty SD...");
  delay(2000);

  if (SD.begin(53))
  {
    lcd.clear();
    lcd.print("SD - OK");
    delay(2000);
  }
  else if (!SD.begin(53))
  {
    pomocSD = 1;
  }
  if (pomocSD == 1)
  {
    while (pomocSD == 1)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("B - 100");
      lcd.setCursor(0, 1);
      lcd.print("SPRAWDZ SD");
      delay(2000);
      pomocSD = 1;
      if (wyllcd == 0) 
      {
        digitalWrite(37, HIGH); 
        digitalWrite(41, LOW); 
        digitalWrite(39, LOW); 
        lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
        lcd.setBacklight(LOW);
        wyllcd = 1;
      }
    }
  }
}
