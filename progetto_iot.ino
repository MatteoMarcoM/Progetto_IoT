// includo le librerie necessarie
#include <WiFi.h>             // per protocollo WiFi
#include <InfluxDbClient.h>   // per libreria di comunicazione con InfluxDB
#include <InfluxDbCloud.h>    // per gestire il token di sicurezza 
#include "MHZCO2.h"           // libreria MHZCO2 (sensore di CO2)

// definisco le costanti del programma

// connessione al WiFi
#define WIFI_SSID       ""
#define WIFI_PASSWORD   ""

// connessione al server IoT (InfluxDB)
#define INFLUXDB_URL    ""
#define INFLUXDB_ORG    ""
#define INFLUXDB_BUCKET ""
#define INFLUXDB_TOKEN  ""

// quante volte tento la connessione al WiFi prima del Reboot
#define WIFI_CONN_TRIALS 20

// definisco la frequenza di campionamento (SENSORE DI RUMORE)
// freq. = 22 kHz -> 22 000 samples / sec
// 1 s / 22 000 samples = 0.000 045 s = 45 us di delay 
#define SAMPLING_TIME_STEP_MICROSEC 45 // (SENSORE DI RUMORE)

// definisco un tempo di campionamento di 10 secondi (SENSORE DI RUMORE)
// -> quanti step (SAMPLING_TIME_STEP_MICROSEC) devo fare?
// 22 kHz * 10 sec -> 22 000 samples / sec * 10 sec => 220 000 samples
#define N_SAMPLES 220000 // (SENSORE DI RUMORE)

// attesa tra una misurazione e l'altra 
// 60 000 ms (1 min) * 5 = 300 000 ms = 5 min
#define MEASURE_TIME_STEP_MILLISEC 300000

// definisco i pin del seriale 2 (SENSORE DI CO2)
#define RXD2 16
#define TXD2 17

// definisco il pin di output del sensore di rumore (analogico)
#define NOISE_SENSOR 32

// definisco le variabili globali
// creo un'istanza di InfluxDBClient
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// definisco la _measurement cioe' la tabella in cui inserire i dati 
Point sensors("corso_IoT");

// istanzio un oggetto di tipo MHZ19B (SENSORE DI CO2)
MHZ19B MHZ19B;

// non posso salvare tutti i valori di rumore che campiono perche' la 
// memoria RAM di ESP32 non e' sufficiente quindi memorizzo una sola 
// variabile che chimo somma per fare la media aritmetica dei sample
double somma = 0.0; // (SENSORE DI RUMORE)

// variabili dove inserire i dati di CO2 e di rumore correnti
int CO2 = 0;
double noise = 0.0;

// metodo setup() di Arduino
void setup() {
  // configuro il seriale
  Serial.begin(115200);
  // faccio uscire l'output di MHZ19B sul seriale 2
  MHZ19B.begin(&Serial2);
  // definisco i parametri di comunicazione col seriale 2
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  // inizializzo la connessione WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Inizio a connettermi al WiFi...");
  int j = 0;
  while(WiFi.status() != WL_CONNECTED && j < WIFI_CONN_TRIALS){
    j++;
    Serial.print(".");
    delay(100);
  }
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("ERRORE: Non riesco a connettermi al WiFi");
    Serial.println("Reboot in 10 secondi...");
    delay(10000);
    // REBOOT
    ESP.restart();
  }
  Serial.println("Connesso!");

  // preparo la connessione su InfluxDB
  // aggiungo i tag nel Point per salvare in modo ordinato i dati sul server
  sensors.addTag("host", "ESP_MONTANARI");
  sensors.addTag("location", "Sassocorvaro");
  sensors.addTag("room", "bedroom");

  // testo la connessione a InfluxDB
  if(client.validateConnection()){
    Serial.println("Connesso a InfluxDB!");
  }else {
    Serial.println("Connessione a InfluxDB FALLITA!");
    Serial.println(client.getLastErrorMessage());
    Serial.println("Reboot in 10 secondi...");
    delay(10000);
    // REBOOT
    ESP.restart();
  }

  Serial.println("Dispositivo configurato correttamente!");
}

// metodo loop() di Arduino
void loop()
{
  // --------------
  // SENSORE DI CO2
  
  MHZ19B.measure();
  CO2 = MHZ19B.getCO2();
  Serial.print("Il valore corrente di CO2 è: ");
  Serial.println(CO2);
  
  // -----------------
  // SENSORE DI RUMORE

  Serial.println("Campiono il segnale di rumore per 10 secondi...");
  for(int i = 0; i < N_SAMPLES; i++){
    
    // sommo cumulativamente il sample corrente per fare la media
    somma += analogRead(NOISE_SENSOR);
    
    // registro un sample con frequenza di 22 kHz
    delayMicroseconds(SAMPLING_TIME_STEP_MICROSEC);
  }

  // ora posso calcolare il valore medio del rumore stamparlo
  noise = somma / N_SAMPLES;
  Serial.print("Rumore nella stanza: ");
  Serial.println(noise);

  // resetto il valore della somma cumulativa per prossima misurazione
  somma = 0.0;

  // scrivo le misurazioni su InfluxDB
  sensors.clearFields();
  sensors.addField("CO2", CO2);
  sensors.addField("noise", noise);
  Serial.print("Scrivo: ");
  Serial.println(sensors.toLineProtocol());

  // ora scrivo sul server
  if(!client.writePoint(sensors)){
    Serial.print("Scrittura su InfluxDB FALLITA!");
    Serial.println(client.getLastErrorMessage());
  } 

  // aspetto 5 minuti prima di fare un altra rilevazione
  Serial.println("Aspetto 5 minuti prima di effettuare un'altra misurazione...");
  delay(MEASURE_TIME_STEP_MILLISEC);
}
