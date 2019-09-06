/*
 * based on
 * https://gist.github.com/whatnick/73c244694466355f6a14#file-energy_monitor-ino
// http://whatnicklife.blogspot.com/2015/09/experimenting-with-energy-monitors.html

#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <ESP8266WiFi.h>

#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h> // Библиотека для OTA-прошивки

//WIFI credentials go here
const char* ssid = "XXXXX";
const char* password = "XXXXX";

const char* node_id  = "1";
const char* domain = "example.com";
const char* path = "emoncms";
const char* apikey = "XXXXXXXXXXX";
char c;

WiFiClient Client;
Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */

unsigned long old;
double offsetI;
double filteredI;
double sqI,sumI;
long sampleI;
double Irms;


double squareRoot(double fg)  
    {
    double n = fg / 2.0;
    double lstX = 0.0;
    while (n != lstX)
    {
        lstX = n;
        n = (n + fg / n) / 2.0;
    }
    return n;
    }

double calcIrms(unsigned int Number_of_Samples)
    {
    /* Be sure to update this value based on the IC and the gain settings! */
    float multiplier = 4e-3;    /* 1 бит 0,188e-3V/bit*1000A/A/47 ohm = 0,188e-3 V/bit*21,3 A/V = 4e-3 V/bit */

    for (unsigned int n = 0; n < Number_of_Samples; n++)
    {
        sampleI = ads.readADC_Differential_0_1();
        sqI = sampleI * sampleI;
        sumI += sqI;
    }
    
    Irms = squareRoot(sumI / Number_of_Samples)*multiplier; 
    sumI = 0;
    //--------------------------------------------------------------------------------------       
    
    return Irms;
    }

void setup() {
    delay(2000);
    
    Serial.begin(115200);
    Serial.flush();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    //зависает здесь
    
    int ctr = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        ctr++;
        if (ctr > 120)
        {
            Serial.println("Connection Failed! Rebooting...");
            delay(1000);
            ESP.restart();
        }
    }

    // Port defaults to 8266
    // ArduinoOTA.setPort(8266);

    // Hostname defaults to esp8266-[ChipID]
    // ArduinoOTA.setHostname("myesp8266");

    // No authentication by default
    // ArduinoOTA.setPassword((const char *)"123");

    ArduinoOTA.setHostname("emon"); // Задаем имя сетевого порта
//     ArduinoOTA.setPassword((const char *)"0000"); // Задаем пароль доступа для удаленной прошивки
    
    ArduinoOTA.onStart([]() {
        Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
        Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    ads.begin();
}

int ctr = 0;
void loop() {
    
    ArduinoOTA.handle(); // Всегда готовы к прошивке
    
    ctr++;
    double current = calcIrms(1024);
    double power = current*210;

    Serial.println();    
    Serial.println(ctr);    
    Serial.print("I= ");
    Serial.print(current, 3);
    Serial.println(" A");    

    Serial.print("P= ");
    Serial.print(power, 2);
    Serial.println(" W");    

    Serial.println("connect to Server...");
    if( Client.connect(domain, 80) )
        {  
        Serial.println("connected");
        Client.print("GET /");
        Client.print(path);
        Client.print("/input/post.json&apikey=");
        Client.print(apikey);
        Client.print("&node=");
        Client.print(node_id);
        Client.print("&csv=");
        Client.print(power);
        Client.println();


        old = millis();
        while( (millis()-old)<500 )    // 500ms timeout for 'ok' answer from server
            {
            while( Client.available() )
                {  
                c = Client.read();
                Serial.write(c);
                }
            }
            Client.stop();
            Serial.println("\nclosed");
        }    
    
    
//   delay(1000);  
}

