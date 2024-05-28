// Ce code est celui du module Arduino Déporté.
// Il monitorera les informations provenant du module Embarqué qu'il recevra
// via un module RF.
// La communication vers le téléphone se fera avec un module Bluetooth
//-------------------------------------------------------------------------------------------------------------------------------
// librairies necessairses
//-------------------------------------------------------------------------------------------------------------------------------
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "rgb_lcd.h"
#include <Wire.h>

//-------------------------------------------------------------------------------------------------------------------------------
// Vitesse des communications de la ligne serie
const int Vitesse_Serie = 9600;
const int Broche_btn_Marche_Arret = 2;  //Broches de l'arduino utilisée pour le bouton marche/arret

//-----------------------------------------------------------------------------
//           Afficheur LCD
//-----------------------------------------------------------------------------
const int nbr_lgn = 2;   // Nombre de lignes du LCD
const int nbr_col = 16;  // Nombre de colonnes du LCD

const int couleur_rouge = 0;   // Constantes utilisées pour initialiser RGB - Red/Green/Blue - de l'afficheur LCD
const int couleur_verte = 255; //   idem
const int couleur_bleue = 0;   //   idem

const int ligne_qualite = 0;    // Ligne du LCD où sera placée l'info Qualite
const int ligne_turbidite = 0;  // Ligne où LCD sera placée l'info turbidite
const int ligne_niveau = 1;     // Ligne où LCD sera placée l'info niveau
const int ligne_pompe = 1;      // Ligne où LCD sera placée l'info pompe
const int ligne_valve = 1;      // Ligne où LCD sera placée l'info valve
const int colonne_qualite = 0;    //  idem pour les colonnes
const int colonne_turbidite = 10; //
const int colonne_niveau = 0;     //
const int colonne_pompe = 6;      // 
const int colonne_valve = 12;     // 

rgb_lcd lcd; // Déclaration de notre afficheur LCD

//-----------------------------------------------------------------------------
// Etat ouvert/ferme de la valve
//-----------------------------------------------------------------------------
const char OUVERT[] = "1";  // Ouverte
const char FERMEE[] = "0";  // Fermée
//-----------------------------------------------------------------------------
// Etat on/off de la pompe
//-----------------------------------------------------------------------------
const char ON[] = "ON";
const char OFF[] = "Off";

const int delayLectureSensor = 1000; // Délai entre 2 lectures des sensor

//-----------------------------------------------------------------------------
// APPLI BLUETOOTH
//-----------------------------------------------------------------------------
const char MSG_ON[] = "DEPART";
const char MSG_OFF[] = "ARRET";
#define TxD 0  //5
#define RxD 1  //4
// SoftwareSerial blueToothSerial(RxD, TxD); On ne l'utilisera pas car on utilise la ligne standard = Attention tous les 
// Serial.print() iront sur le bluetooth

//-------------------------------------------------------------------------------------------------------------------------------
//              XBEE
//-----------------------------------------------------------------------------
// Déclaration de notre module de communication
// brancher le module xbee sur la broche D7/D8 (7 Broche_Reception, 8 Broche_Transmission)
const int Vitesse_XBEE = 9600;
const int Broche_Reception = 7;    // transmission XBEE
const int Broche_Transmission = 8; // Reception XBEE

SoftwareSerial xbee(Broche_Reception, Broche_Transmission);

//-----------------------------------------------------------------------------
//         Nos capteurs
//-----------------------------------------------------------------------------
String Capteur_Niveau;
String Capteur_Turbidite;
String Capteur_Qualite;
String Pompe_Marche_Arret = OFF;
String Valve_Ouverte_Fermee = FERMEE;
String Erreur = "";
// Ci dessous les valeurs précédentes de ces memes capteurs
String Capteur_Niveau_Prec;                 
String Capteur_Turbidite_Prec;           
String Capteur_Qualite_Prec;           
String Pompe_Marche_Arret_Prec = OFF;       
String Valve_Ouverte_Fermee_Prec = FERMEE;  

volatile int btn_Poussoir_Marche_Arret = LOW;  // état actuel du bouton poussoir (modifié ds laprocedure d'interruption)

// Chaine de caractères que l'on enverra sur le module Embarqué
String a_envoyer;
String a_envoyer_prec;  //Pour ne pas envoyer 2 fois la meme chaine d'affilé
// Chaine de caractères que l'on recevra du module Embarqué
String lu;
String lu_prec;  //Pour savoir si la chaine est défférente de la précédente

//-------------------------------------------------------------------------------------------------------------------------------
//
//                                         SETUP
//
//-------------------------------------------------------------------------------------------------------------------------------



void setup() {
  Serial.begin(Vitesse_Serie);  // initialisation de la communication serie pour Arduino

  xbee.begin(Vitesse_XBEE);  // initialisation de la communication serie pour le module xbee

  Serial.println("");
  Serial.println("Module déporté");

  pinMode(Broche_btn_Marche_Arret, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(Broche_btn_Marche_Arret), ServiceInterruptionBouton, CHANGE);

  pinMode(Broche_btn_Marche_Arret, INPUT);  // Déclaration du bouton marche arret en entrée

  pinMode(RxD, INPUT);   // Bluetooth
  pinMode(TxD, OUTPUT);  // Bluetooth

  setupBlueToothConnection();
  Serial.flush();
  //blueToothSerial.flush();


  Wire.begin();                 // initialise l'I2C
  lcd.begin(nbr_col, nbr_lgn);  // initialisation de l'affichuer LCD
  lcd.setRGB(couleur_rouge, couleur_verte, couleur_bleue);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write("Module deporte");
  delay(1500);
  lcd.clear();
  LCD_AfficheTout(true);
}

void afficheSurSerial() {

  Serial.println();
  Serial.print("Q: ");
  Serial.print(Capteur_Qualite);
  Serial.print("/");
  Serial.print("T: ");
  Serial.print(Capteur_Turbidite);
  Serial.print("/");
  Serial.print("N: ");
  Serial.print(Capteur_Niveau);
  Serial.print("/");
  Serial.print("P: ");
  Serial.print(Pompe_Marche_Arret);
  Serial.print("/");
  Serial.print("V: ");
  Serial.print(Valve_Ouverte_Fermee);
  Serial.print("/");
  Serial.print("Lu: ");
  Serial.println(lu);


  Serial.print("Q-: ");
  Serial.print(Capteur_Qualite_Prec);
  Serial.print("/");
  Serial.print("T-: ");
  Serial.print(Capteur_Turbidite_Prec);
  Serial.print("/");
  Serial.print("N-: ");
  Serial.print(Capteur_Niveau_Prec);
  Serial.print("/");
  Serial.print("P-: ");
  Serial.print(Pompe_Marche_Arret_Prec);
  Serial.print("/");
  Serial.print("V-: ");
  Serial.print(Valve_Ouverte_Fermee_Prec);
  Serial.print("/");
  Serial.print("Lu-: ");
  Serial.println(lu_prec);
  Serial.print("Erreur: ");
  Serial.print(Erreur);
  Serial.println("");
}

//-------------------------------------------------------------------------
// Ensemble de procedures pour ne raffraichir qu'une partie du LCD
//-------------------------------------------------------------------------


void LCD_Qualite(bool forcage) {
  if ((Capteur_Qualite != Capteur_Qualite_Prec) || (forcage == true)) {
    lcd.setCursor(colonne_qualite, ligne_qualite);
    lcd.write("Q:");
    lcd.setCursor(colonne_qualite + 2, ligne_qualite);
    lcd.write("Q:    ");
    lcd.setCursor(colonne_qualite + 2, ligne_qualite);
    lcd.write(Capteur_Qualite.c_str());
  }
}

void LCD_Turbidite(bool forcage) {
  if ((Capteur_Turbidite != Capteur_Turbidite_Prec) || (forcage == true)) {
    lcd.setCursor(colonne_turbidite, ligne_turbidite);
    lcd.write("T:");
    lcd.setCursor(colonne_turbidite + 2, ligne_turbidite);
    lcd.write("T:    ");
    lcd.setCursor(colonne_turbidite + 2, ligne_turbidite);
    lcd.write(Capteur_Turbidite.c_str());
  }
}


void LCD_Niveau(bool forcage) {
  if ((Capteur_Niveau != Capteur_Niveau_Prec) || (forcage == true)) {
    lcd.setCursor(colonne_niveau, ligne_niveau);
    lcd.write("N:");
    lcd.setCursor(colonne_niveau, ligne_niveau);
    lcd.write("N:    ");
    lcd.setCursor(colonne_niveau + 2, ligne_niveau);
    lcd.write(Capteur_Niveau.c_str());
  }
}

void LCD_Pompe() {
  //  if (Pompe_Marche_Arret != Pompe_Marche_Arret_Prec) {
  lcd.setCursor(colonne_pompe, ligne_pompe);
  lcd.write("P:");
  lcd.setCursor(colonne_pompe + 2, ligne_pompe);  //effacement du On/Off  sans effacer tout l'écran
  lcd.write("   ");
  lcd.setCursor(colonne_pompe + 2, ligne_pompe);
  lcd.write(Pompe_Marche_Arret.c_str());
  // }
}

void LCD_Valve() {
  // if (Valve_Ouverte_Fermee != Valve_Ouverte_Fermee_Prec) {
  lcd.setCursor(colonne_valve, ligne_valve);
  lcd.write("V:");
  lcd.setCursor(colonne_valve + 2, ligne_valve);
  lcd.write(Valve_Ouverte_Fermee.c_str());
  //   }
}

void LCD_Efface() {
  lcd.clear();
}

void LCD_AfficheTout(bool forcage) {
  int i;

  if (Erreur == "") {
    LCD_Qualite(forcage);
    LCD_Niveau(forcage);
    LCD_Turbidite(forcage);
    LCD_Pompe();
    LCD_Valve();
  } else {

    LCD_Efface();
    for (i = 0; i <= 2; i++) {  // on fait clignoter 3 fois l'erreur et on reinitislaise l'erreur
      lcd.setRGB(255, 0, 0);
      lcd.setCursor(6, 0);
      lcd.write("ERREUR");
      lcd.setCursor(0, 1);
      lcd.write(Erreur.c_str());
      delay(1000);
      lcd.setRGB(0, 255, 0);
    }
    LCD_Efface();
    Erreur = "";
    LCD_Qualite(true);  //on reaffiche les précédentes valeurs
    LCD_Turbidite(true);
    LCD_Niveau(true);
    LCD_Pompe();
    LCD_Valve();
  }
}
//-------------------------------------------------------------------------




bool ChaineRecue_ok(String lu) {  // a préciser
  bool ok = true;
  return ok;
}

void Receptionne_XBEE() {
  //-------------------------------------------------------------------------------------------
  // Dans cette procedure on va récupérer la chaine de caractere XBEE du module Embarqué et
  // extraire les paramètres des capteurs, pompe, valve et erreur.
  // Ces variables sont déclarées en variables globales en début de programme.
  //-------------------------------------------------------------------------------------------
  String recup;
  int i;                  // Indice de boucle
  char caractereEnCours;  // Premier caractere du parametre a récupérer dans la chaine "lu_tmp", càd:(q,t,n,p,v ou E)
  char LeCar;             // Caractère en cours d'analyse (ième caractère)


  if (xbee.available() > 0) {  // Des caractères sont disponible dans XBEE
                               //if (Serial.available() > 0) {  // Des caractères sont disponible dans XBEE

    String recup = xbee.readString();

    Serial.println(recup);
    if (ChaineRecue_ok(recup) == true) {

      if (recup != lu_prec) {
        // la chaine de caracteres est différente de la précédente
        lu_prec = recup;

        for (i = 0; i < recup.length(); i++) {  // Avec cette boucle for, on parcours la chaine recue caractère par caractère
          char LeCar = recup.charAt(i);         // On mémorisele ième caractere
          switch (LeCar) {                      // En fonction du ième caractere
            case 'q':
              caractereEnCours = 'q';
              Capteur_Qualite_Prec = Capteur_Qualite;
              Capteur_Qualite = "";
              break;

            case 't':
              caractereEnCours = 't';
              Capteur_Turbidite_Prec = Capteur_Turbidite;
              Capteur_Turbidite = "";
              break;

            case 'n':
              caractereEnCours = 'n';
              Capteur_Niveau_Prec = Capteur_Niveau;
              Capteur_Niveau = "";
              break;

            case 'p':
              caractereEnCours = 'p';
              Pompe_Marche_Arret_Prec = Pompe_Marche_Arret;
              Pompe_Marche_Arret = "";
              break;

            case 'v':
              caractereEnCours = 'v';
              Valve_Ouverte_Fermee_Prec = Valve_Ouverte_Fermee;
              Valve_Ouverte_Fermee = "";
              break;

            case 'E':
              caractereEnCours = 'E';
              Erreur = recup;
              i = recup.length() + 1;  // pour sortir dela boucle
              break;

            default:  //Pour tous les autre caractere on les concataine dans la bonne variable

              switch (caractereEnCours) {
                case 'q':
                  Capteur_Qualite += LeCar;

                  break;
                case 't':
                  Capteur_Turbidite += LeCar;

                  break;
                case 'n':
                  Capteur_Niveau += LeCar;

                  break;
                case 'p':
                  Pompe_Marche_Arret += LeCar;

                  break;
                case 'v':
                  Valve_Ouverte_Fermee += LeCar;

                case '\0':
                  break;
                default:

                  break;

              }       // fin du deuxieme switch
              break;  // du default du premier switch
          }           // fin du premier switch
        }             // fin du for (i = 0; i < strlen(lu); i++)
        LCD_AfficheTout(false);

      }  // Chaine identique a la précédente
    }    // if (ChaineRecue_ok(recup) == true) {
  }      // fin du if (xbee.available() > 0)
}  // fin de fonction
//-------------------------------------------------------------------------------------------------------------------------------
//
//                                         ENVOI DE DONNEES au module embarqué
//
//-------------------------------------------------------------------------------------------------------------------------------

void ENVOI(String Aenvoyer) {
  // Cette procedure envoie la chaine "aenvoyer" sur XBee
  if (Aenvoyer != a_envoyer_prec) {
    xbee.print(Aenvoyer);
    Serial.print(Aenvoyer);
    delay(delayLectureSensor);

    Serial.print("Envoi: ");
    Serial.print(Aenvoyer);
    Serial.print("/ -: ");
    Serial.print(a_envoyer_prec);
    Serial.print("/P: ");
    Serial.print(Pompe_Marche_Arret);
    Serial.print("/P-: ");
    Serial.print(Pompe_Marche_Arret_Prec);
    

  }
}

void PON() {
  a_envoyer = "p";
  a_envoyer += ON;  //le caractere de fin de chaine est ajouté par la fonction strncat. 2 est lenombre de caractère copié de "ON"
  ENVOI(a_envoyer);
}

void POFF() {
  a_envoyer = "p";
  a_envoyer += OFF;  //le caractere de fin de chaine est ajouté par la fonction strncat. 2 est lenombre de caractère copié de "ON"
  ENVOI(a_envoyer);
}

void Envoie_XBEE_Marche_Arret() {
  if ((btn_Poussoir_Marche_Arret == HIGH) && (Pompe_Marche_Arret == OFF)) {
    //afficheSurSerial();
    PON();
  }

  if ((btn_Poussoir_Marche_Arret == HIGH) && (Pompe_Marche_Arret == ON)) {
    //afficheSurSerial();
    POFF();
   }
}
//-------------------------------------------------------------------------------------------------------------------------------
//
//                         Bluetooth pour communication avec appli android MIT
//
//-------------------------------------------------------------------------------------------------------------------------------

void setupBlueToothConnection() {
  Serial.begin(9600);
  Serial.print("AT");
  delay(400);
  Serial.print("AT+DEFAULT");
  delay(2000);  // Restore all setup value to factory setup
  Serial.print("AT+NAMEJeanXXIII");
  delay(400);  // set the bluetooth name as "SeeedMaster" ,the length of bluetooth name must less than 12 characters.
  Serial.print("AT+ROLES");
  delay(400);  // set the bluetooth work in master mode
  Serial.print("AT+AUTH1");
  delay(400);
  Serial.print("AT+CLEAR");
  delay(400);  // Clear connected device mac address
  Serial.flush();
}

void ChechBluetooth() {
  String ChaineRecue;

  // ChaineRecue = blueToothSerial.readString();
  ChaineRecue = Serial.readString();
  Serial.println(ChaineRecue);

  if (ChaineRecue == MSG_OFF) {
    POFF();
  }
  if (ChaineRecue == MSG_ON) {
    PON();
  }
}
//-----------------------------------------------------------------------------------------
//
//                                  Boucle principale
//
//-----------------------------------------------------------------------------------------
void loop() {
  String recup;
  ChechBluetooth();  // blueToothSerial.flush();
  //delay(1000);

  Receptionne_XBEE();
  Envoie_XBEE_Marche_Arret();
}

//-------------------------------------------------------------------------------------------------------------------------------
//
//                                        PROCEDURE D'INTERRUPTION
//
//-------------------------------------------------------------------------------------------------------------------------------

void ServiceInterruptionBouton() {

  if (btn_Poussoir_Marche_Arret == HIGH) {
    btn_Poussoir_Marche_Arret = LOW;

  } else {
    btn_Poussoir_Marche_Arret = HIGH;
  }
}
