// Ce code est celui du module Arduino embarqué


//-------------------------------------------------------------------------------------------------------------------------------
// On importe les librairies necessairses
//-------------------------------------------------------------------------------------------------------------------------------
#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <TimerOne.h>

// macros pour utilisation du sensor de niveau d'eau
#define NO_TOUCH 0xFE
#define SEUIL 100
#define ATTINY1_HIGH_ADDR 0x78
#define ATTINY2_LOW_ADDR 0x77

//-------------------------------------------------------------------------------------------------------------------------------
//
//                                        Déclaration des constantes
//
//-------------------------------------------------------------------------------------------------------------------------------
// Constantes utilisées pour initialiser RGB - Red/Green/Blue - de l'afficheur LCD
/*const int colorR = 0;
const int colorG = 255;
const int colorB = 0;*/
//---------------------------------------------
// Vitesse des communications de la ligne serie
//---------------------------------------------
const int Vitesse_Serie = 9600;
//---------------------------------------------
// Vitesse des communications de la ligne XBEE
//---------------------------------------------
const int Vitesse_XBEE = 9600;
//-----------------------------------------------------------------------------
// Broches de l'arduino utilisées pour la transmissio XBEE
//-----------------------------------------------------------------------------
const int Broche_Reception = 2;
const int Broche_Transmission = 3;
//-----------------------------------------------------------------------------
// Etat ouvert/ferme de la valve
//-----------------------------------------------------------------------------
const char OUVERT[] = "1";
const char FERMEE[] = "0";
//-----------------------------------------------------------------------------
// Etat on/off de la pompe
//-----------------------------------------------------------------------------
const char ON[] = "ON";
const char OFF[] = "Off";
//-----------------------------------------------------------------------------
// Broches de l'arduino utilisées pour la pompe
//-----------------------------------------------------------------------------
const int Broche_relay_Pompe = 4;
//-----------------------------------------------------------------------------
// Broches de l'arduino utilisées pour la valve
//-----------------------------------------------------------------------------
const int Broche_relay_Valve = 5;
//-----------------------------------------------------------------------------
// Declaration de 2 tableaux pour le sensor de Niveau d'eau
//-----------------------------------------------------------------------------
unsigned char donnees_basses[8] = { 0 };
unsigned char donnees_hautes[12] = { 0 };

//-----------------------------------------------------------------------------
// le capteur turbidité utilise A0
// le capteur qualite utilise A1
//-----------------------------------------------------------------------------
const int Broche_Turbidite = A0;
const int Broche_Qualite = A1;

const int delayLectureSensor = 2000;

const char ERREUR_QUALITE[16] = "E:Qualite !";
const char ERREUR_TURBIDITE[16] = "E:Turidite !";
const char ERREUR_NIVEAU[16] = "E:Niveau bas !";
const char ERREUR_DUREE_POMPE_EN_MARCHE[16] = "E:Duree max !";
const char ERREUR_NIVEAU_BLOQUE[16] = "E:Niveau Bloque !";

const char ERREUR_VIDE[2] = " ";

int Seuil_Niveau = 10;        //=> a définir
float Seuil_Turbidite = 1.5;  //=> a définir
float Seuil_Qualite = 0.4;    //=> a définir

// pendant le pompage on déclenche un timer
int DelaiMaxPompage = 60;              // Durée de fonctionnement max la pompe apres laquelleon la coupe quoi qu'il arrive ici 60 s => a définir
int DelaiMaxAvantBaisseDeNiveau = 10;  // Durée au dela de laquelle le niveau doit avoir changé, sinon on coupe lapompe +valve ici 10 s
int chrono_suivi_niveau;               // Temps intermediaire:
                                       // indication temporelle (chrono général - chrono suivi niveau) > DelaiMaxAvantBaisseDeNiveau 
                                       // => controller le niveau et arreter la pompe + valve si ca nebouge pas
int niveau_Debut_Pompage;              // Copie du capteur de niveau au demarrage du pompage et a chaque étape de controle
//-------------------------------------------------------------------------------------------------------------------------------
//
//                                         Déclaration des Variables
//
//-------------------------------------------------------------------------------------------------------------------------------
// Variable "lcd" de type rgb_lcd => ça sera notre affichage LCD
// rgb_lcd lcd;
//------------------------------------------------------------------
// Déclaration de notre module de communication
// brancher le module xbee sur la broche D2/D3 (2 Broche_Reception, 3 Broche_Transmission)
SoftwareSerial xbee(Broche_Reception, Broche_Transmission);
//------------------------------------------------------------------
String Capteur_Niveau;
String Capteur_Turbidite;
String Capteur_Qualite;
String Pompe_Marche_Arret = OFF;
String Valve_Ouverte_Fermee = FERMEE;

String Pompe_Marche_Arret_Prec = OFF;
String Valve_Ouverte_Fermee_Prec = FERMEE;


volatile int chrono_general = 0;


String a_envoyer;
String a_envoyer_prec;  //Pour ne pas envoyer 2 fois la meme chaine d'affilé

String lu;
String lu_prec;  //Pour savoir si la chaine est défférente de la précédente

//-------------------------------------------------------------------------------------------------------------------------------
//
//                                         SETUP
//
//-------------------------------------------------------------------------------------------------------------------------------

void setup() {
  Serial.begin(Vitesse_Serie);  // initialisation de la communication serie pour Arduino
  xbee.begin(Vitesse_XBEE);     // initialisation de la communication serie pour le module xbee
  Wire.begin();

  Serial.print("");
  Serial.println("Module Embarqué");

  // Initialiser le "Timer1 de la bibliotheque déclaré plus haut
  Timer1.initialize(1000000);                        // Ajuste la base de temps du timer à une seconde (1000000 microseconds)
  Timer1.attachInterrupt(ServiceInterruptionTimer);  // Relie la procédure d'interruption (ISR)
  // A chaque fois qu'elle se déclenchera on incrémentera un compteur "volatile" chrono_general déclaré ci dessus
  // On prendra soin de le remettre a 0 avant de l'utiliser.
  chrono_general = 0;

  pinMode(Broche_relay_Pompe, OUTPUT);    // Déclaration du relai pompe en sortie
  digitalWrite(Broche_relay_Pompe, LOW);  // Initialise le relai à 0
  pinMode(Broche_relay_Valve, OUTPUT);    // Déclaration du relai valve en sortie
  digitalWrite(Broche_relay_Valve, LOW);  // Initialise le relai à 0

  Serial.println("Embarqué prêt...");
  ENVOI("Embarque pret...");  // pour dire ok au module déporté
}


//-----------------------------------------------------------------------------------------
//
// Section  relative au capteur de niveau d'eau
//
//-----------------------------------------------------------------------------------------
void Niveau_lire_12_DonneesHautes(void) {
  memset(donnees_hautes, 0, sizeof(donnees_hautes));
  Wire.requestFrom(ATTINY1_HIGH_ADDR, 12);
  while (12 != Wire.available())
    ;

  for (int i = 0; i < 12; i++) { donnees_hautes[i] = Wire.read(); }
  delay(10);
}

void Niveau_lire_8_DonneesBasses(void) {
  memset(donnees_basses, 0, sizeof(donnees_basses));
  Wire.requestFrom(ATTINY2_LOW_ADDR, 8);
  while (8 != Wire.available())
    ;

  for (int i = 0; i < 8; i++) { donnees_basses[i] = Wire.read(); }  // receive a byte as character
  delay(10);
}

void Verif_Niveau_Eau() {
  char N[8];
  int pourcentage;
  int compteur_section_basse = 0;
  int compteur_section_haute = 0;

  uint32_t barre_recouverte = 0;
  uint8_t nombre_de_barres = 0;
  Niveau_lire_8_DonneesBasses();
  Niveau_lire_12_DonneesHautes();

  for (int i = 0; i < 8; i++) {
    // On place un 1 au ième rang de barre_recouverte sans changer les autres bits
    if (donnees_basses[i] > SEUIL) {
      barre_recouverte |= 1 << i;
    }
  }
  for (int i = 0; i < 12; i++) {
    // On place un 1 au ième rang de barre_recouvertea partir du 8ème sans changer les autres bits
    if (donnees_hautes[i] > SEUIL) {
      barre_recouverte |= (uint32_t)1 << (8 + i);
    }
  }

  while (barre_recouverte & 0x01)
  // On parcours chaque bit de "barre_recouverte" s'il est egale à 1 en masquant les autres bits avec 0x01
  // on incremente le nombre de barre et ca tant qu'on ne rencontre pas de 0
  {
    nombre_de_barres++;
    barre_recouverte >>= 1;  // on passe au bit suivant
  }

  pourcentage = nombre_de_barres * 5;


  itoa(pourcentage, N, 10);  //converti l'entier en chaine de caracteres (10 = base 10)
  Capteur_Niveau = N;
}
//-----------------------------------------------------------------------------------------
//
// Section  relative au capteur de turbidite
//
//-----------------------------------------------------------------------------------------

void Verif_Turbidite() {
  char T[8];
  int Turbidite = analogRead(Broche_Turbidite);  // Lire la valeur analogique sur la broche Broche_Turbidite
  float tension = Turbidite * (5.0 / 1024.0);    // Converti la valeur entiere (qui va de 0 à 1023) en une tension de  (0 - 5V)

  dtostrf(tension, 5, 2, T);  //dtostrf(float_value, min_width, num_digits_after_decimal, where_to_store_string);
  Capteur_Turbidite = T;
}
//-----------------------------------------------------------------------------------------
//
// Section  relative au capteur de Qualité
//
//-----------------------------------------------------------------------------------------
void Verif_Qualite() {
  char Q[8];
  int Qualite = analogRead(Broche_Qualite);  // Lire la valeur analogique sur la broche Broche_Qualite
  float tension = Qualite * (5.0 / 1024.0);  // Converti la valeur entiere (qui va de 0 à 1023) en une tension de  (0 - 5V)
  dtostrf(tension, 5, 2, Q);                 //dtostrf(float_value, min_width, num_digits_after_decimal, where_to_store_string);
  Capteur_Qualite = Q;
}


void Receptionne_XBEE() {
  String recup;
  int i;
  char caractereEnCours;
  char LeCar;

  if (xbee.available() > 0) {

    recup = xbee.readString();

    lu_prec = lu;
    lu = recup;

    for (i = 0; i <= recup.length(); i++) {
      LeCar = recup.charAt(i);
      Serial.print(LeCar);
      switch (LeCar) {  // ième caractere
        case 'p':
          caractereEnCours = 'p';
          Pompe_Marche_Arret_Prec = Pompe_Marche_Arret;
          Pompe_Marche_Arret = "";
          break;

        default:
          switch (caractereEnCours) {
            case 'p':
              Pompe_Marche_Arret += LeCar;
              break;

            default:
              break;
          }       // fin du deuxième switch
          break;  // du default du premier switch
      }           // fin du premier switch
    }             // fin de la boucle for
  }               // fin du if (xbee.available() > 0)

  if ((Pompe_Marche_Arret_Prec == OFF) && (Pompe_Marche_Arret == ON)) {
    // demande de miseen route recu
    chrono_suivi_niveau = chrono_general;  // on récupère la valeur du comptage par interruption
    niveau_Debut_Pompage = atoi(Capteur_Niveau.c_str());
  }
}  // fin de fonction


void ENVOI(String Aenvoyer) {
  // Cette procedure envoie la chaine "aenvoyer" sur XBee
  if (Aenvoyer != a_envoyer_prec) {
    xbee.print(Aenvoyer);
    delay(delayLectureSensor);

    Serial.print("Envoi: ");
    Serial.print(Aenvoyer);
    Serial.print(" / avant: ");
    Serial.print(a_envoyer_prec);
    Serial.print(" / Pompe act: ");
    Serial.print(Pompe_Marche_Arret);
    Serial.print(" / Pompe avant: ");

    Serial.print(Pompe_Marche_Arret_Prec);
    Serial.print(" / Chrono Gnral: ");
    Serial.print(chrono_general);
    Serial.print(" / Chrono Suivi Niv: ");
    Serial.println(chrono_suivi_niveau);
  }
}
void Envoie_XBEE_Tout() {

  if (xbee.available() == 0) {  // Si la ligne RF est disponible

    a_envoyer_prec = a_envoyer;
    a_envoyer = "";
    // On constitue ici la chaine a envoyer
    a_envoyer += "q";
    a_envoyer += Capteur_Qualite;
    a_envoyer += "t";
    a_envoyer += Capteur_Turbidite;
    a_envoyer += "n";
    a_envoyer += Capteur_Niveau;
    a_envoyer += "p";
    a_envoyer += Pompe_Marche_Arret;
    a_envoyer += "v";
    a_envoyer += Valve_Ouverte_Fermee;

    ENVOI(a_envoyer);
  }
}



void Envoie_XBEE_ERREUR(String message) {  // Message est déja préformatté au moment ou se produit l'erreur

  if (xbee.available() == 0) {  // Si la ligne RF est disponible
    ENVOI(message);
    message = "";
  }
}

void DemarrePompe_ouvreValve() {
  digitalWrite(Broche_relay_Pompe, HIGH);
  digitalWrite(Broche_relay_Valve, HIGH);
  Pompe_Marche_Arret_Prec = Pompe_Marche_Arret;
  Pompe_Marche_Arret = ON;
  Valve_Ouverte_Fermee_Prec = Valve_Ouverte_Fermee;
  Valve_Ouverte_Fermee = OUVERT;
}

void ArretePompe_FermeValve() {
  digitalWrite(Broche_relay_Pompe, LOW);
  digitalWrite(Broche_relay_Valve, LOW);
  Pompe_Marche_Arret_Prec = Pompe_Marche_Arret;
  Pompe_Marche_Arret = OFF;
  Valve_Ouverte_Fermee_Prec = Valve_Ouverte_Fermee;
  Valve_Ouverte_Fermee = FERMEE;
}
//-----------------------------------------------------------------------------------------
//
//                                  Boucle principale
//
//-----------------------------------------------------------------------------------------

void loop() {
  int niv;
  float turbi, quali;
  Verif_Niveau_Eau();
  Verif_Turbidite();
  Verif_Qualite();

  Envoie_XBEE_Tout();
  Receptionne_XBEE();

  // -------------------------SI demarrage pompe demandé-------------------------------------------------
  if (Pompe_Marche_Arret == ON) {  //&& (strcmp(Pompe_Marche_Arret_Prec, OFF) == 0)) {

    //------------------------------Si le niveau est correct----------------------------------------------
    niv = atoi(Capteur_Niveau.c_str());  // Converti la chaine correspondant au Niveau d'eau, en entier (pourcentage) pour le comparer au seuil
    if (niv >= Seuil_Niveau) {

      //------------------------------Si la turbidité est satisfaisante-------------------------------------
      turbi = atof(Capteur_Turbidite.c_str());  // Converti la chaine correspondant a la turbidite  en float pour la comparer au seuil
      if (turbi >= Seuil_Turbidite) {

        //------------------------------Si la qualité est satisfaisante-------------------------------------
        quali = atof(Capteur_Qualite.c_str());  // Converti la chaine correspondant a la qualité  en float  pour la comparer au seuil
        if (quali >= Seuil_Qualite) {

          if (chrono_general < DelaiMaxPompage) {

            if ((chrono_general - chrono_suivi_niveau) < DelaiMaxAvantBaisseDeNiveau) {  // la pompe tourne depuis plus de DelaiMaxAvantBaisseDeNiveau

              DemarrePompe_ouvreValve();

            } else {
              if (niveau_Debut_Pompage != atoi(Capteur_Niveau.c_str())) {  // le niveau baisse dans le temps impartit
                niveau_Debut_Pompage = atoi(Capteur_Niveau.c_str());       // on memorise le niveau actuel et le compteur actuel pour vérifier que le niveaucontinu toujours a descendre qd on pompe
                chrono_suivi_niveau = chrono_general;
              } else {
                ArretePompe_FermeValve();
                Envoie_XBEE_ERREUR(ERREUR_NIVEAU_BLOQUE);
                chrono_general = 0;
              }
            }

          } else {
            ArretePompe_FermeValve();
            Envoie_XBEE_ERREUR(ERREUR_DUREE_POMPE_EN_MARCHE);
            chrono_general = 0;
          }

        } else {
          ArretePompe_FermeValve();
          Envoie_XBEE_ERREUR(ERREUR_QUALITE);
          chrono_general = 0;
        }
      }  //fin Qualite
      else {
        ArretePompe_FermeValve();
        Envoie_XBEE_ERREUR(ERREUR_TURBIDITE);
        chrono_general = 0;
      }
    }       //fin turbi
    else {  //le niveau de la cuve est insuffisant on arrete la ponpe et on ferme la valve
      ArretePompe_FermeValve();
      Envoie_XBEE_ERREUR(ERREUR_NIVEAU);
      chrono_general = 0;
    }
  } else {
    if (Pompe_Marche_Arret == OFF) {  //} && (strcmp(Pompe_Marche_Arret_Prec, ON) == 0)) {
      // Si une demande d'arret de la pompe est demandé et que la pompe était en fonctionnement
      ArretePompe_FermeValve();
      chrono_general = 0;
    } else {
    }
  }
}

void ServiceInterruptionTimer() {
  if (chrono_general >= 0) {
    chrono_general++;
  }
}