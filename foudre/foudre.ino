#include <Wire.h>
#include <Time.h>
#include <DS1307RTC.h>
#include <LiquidCrystal.h>
#include <SPI.h>
#include <PWFusion_AS3935.h>

// définition des broches pour LCD
#define LCD_RS 8
#define LCD_EN 7
#define LCD_D4 A0
#define LCD_D5 A1
#define LCD_D6 A2
#define LCD_D7 A3

// déclaration afficheur LCD
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// configuration brocheq module AS3935
#define CS_PIN   10 // chip select
#define SI_PIN   9  // sélection interface
#define IRQ_PIN  2  // broche 2 et 3 avec interruption
#define IRQ_NUM  0  // interruption 0 ou 1 sur Uno (broches 2 ou 3)

// variable d'état pour interruption
volatile int8_t AS3935_ISR_Trig = 0;

// définition pour configuration AS3935
#define AS3935_INDOORS      0    // en intérieur
#define AS3935_OUTDOORS     1    // en extérieur
#define AS3935_DIST_DIS     0    // désactivation distance
#define AS3935_DIST_EN      1    // activation distance
#define AS3935_CAPACITANCE  104  // calibration / étiquette

// fonction interruption
void AS3935_ISR() {
  AS3935_ISR_Trig = 1;
}

// déclaration capteur AS3935
PWF_AS3935 lightning(CS_PIN, IRQ_PIN, SI_PIN);

// Affichage valeur sur 2 chiffres LCD
void lcdprint2digits(int number) {
  if (number >= 0 && number < 10)
    lcd.print('0');
  lcd.print(number);
}

// affichage heure LCD
void lcdprinttime(int ligne) {
  tmElements_t tm;
  
  if (RTC.read(tm)) {
    lcd.setCursor(0,ligne);
    lcdprint2digits(tm.Day);
    lcd.print("/");
    lcdprint2digits(tm.Month);
    lcd.print("/");
    lcd.print(tmYearToCalendar(tm.Year));
    lcd.print("  ");
    lcdprint2digits(tm.Hour);
    lcd.print(":");
    lcdprint2digits(tm.Minute);
    lcd.print(":");
    lcdprint2digits(tm.Second);
  }
}

// configuration
void setup() {
  // activation LCD 4*20
  lcd.begin(20, 4);
  // la date et heure
  lcdprinttime(0);
  // + message de démarrage
  lcd.setCursor(0,1);
  lcd.print("Boot...");
  
  // activation SPI
  SPI.begin();
  // horloge SPI /16
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  // SPI mode1
  SPI.setDataMode(SPI_MODE1);
  // octet de poids fort en premier
  SPI.setBitOrder(MSBFIRST);
  
  // initialisation AS3935
  lightning.AS3935_DefInit();
  // calibration AS3935
  lightning.AS3935_ManualCal(AS3935_CAPACITANCE, AS3935_INDOORS, AS3935_DIST_EN);
  // ajout interruption sur front montant
  attachInterrupt(IRQ_NUM, AS3935_ISR, RISING);
}

// boucle principale
void loop() {
  // boucle sans fin en attente d'interruption
  while(AS3935_ISR_Trig == 0){}
  
  delay(3);
  // Interruption détectée, on repasse à 0 et on traite
  AS3935_ISR_Trig = 0;
  
  // vérification de la source d'interruption
  uint8_t int_src = lightning.AS3935_GetInterruptSrc();
  
  /*
  if(int_src == 0) {
    lcd.clear();
    lcdprinttime(0);
    lcd.setCursor(0,1);
    lcd.print("Autre interrupt.");
  } else if(int_src == 1) {
    uint8_t lightning_dist_km = lightning.AS3935_GetLightningDistKm();
    lcd.clear();
    lcdprinttime(0);
    lcd.setCursor(0,1);
    lcd.print("Impact Foudre !");
    lcd.setCursor(0,2);
    lcd.print(lightning_dist_km);
    lcd.print(" km");
  } else if(int_src == 2) {
    lcd.clear();
    lcdprinttime(0);
    lcd.setCursor(0,1);
    lcd.print("Perturbation...");
  } else if(3 == int_src) {
    lcd.clear();
    lcdprinttime(0);
    lcd.setCursor(0,1);
    lcd.print("Trop de bruit...");
  }
  */
  
  // effacement LCD
//  lcd.clear();
  // affichage de la date/heure
//  lcdprinttime(0);
  // seconde ligne LCD
//  lcd.setCursor(0,1);
  // On switch sur la valeur de la source
  switch(int_src) {
    case 0:
      // source non prise en chagre
      // lcd.print("Autre interruption");
      break;
    case 1:
      // c'est une vrai détection
      lcd.clear();
      lcdprinttime(0);
      lcd.setCursor(0,1);
      lcd.print("Impact Foudre !");
      // troisième ligne LCD
      lcd.setCursor(0,2);
      lcd.print("Distance : ");
      // on affiche la distance estimée par l'AS3935
      lcd.print(lightning.AS3935_GetLightningDistKm());
      lcd.print(" km");
      break;
    case 2:
      // C'est une interruption
      // mais pas un impact reconnu
      // lcd.print("Perturbation...");
      break;
    case 3:
      // l'AS3935 nous informe qu'il ne fait plus la 
      // différence entre le bruit et les impacts
      lcd.print("Trop de bruit...");
      break;
  }
}
