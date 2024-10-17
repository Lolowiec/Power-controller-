#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <WiFiServer.h>
#include <ArduinoJson.h> // Dodanie biblioteki ArduinoJson
#include <Servo.h>       // Dodanie biblioteki Servo

// Parametry wyświetlacza OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Połączenie Wi-Fi
const char *ssid = "Speedy IoT";
const char *password = "jozbar22081337";

// Ustawienia serwera WWW
WiFiServer server(80);
String header;

// Parametry przycisków
int buttonPin1 = 12;
int buttonPin2 = 13;
int buttonState1 = HIGH;
int lastButtonState1 = HIGH;
int buttonState2 = HIGH;
int lastButtonState2 = HIGH;
int Value = 3;
int lastValue = Value;

// Parametry wyświetlacza OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Parametry serwa
Servo myServo;           // Utworzenie obiektu serwa
const int servoPin = D1; // Pin do którego podłączone jest serwo

void setup()
{
    Serial.begin(115200);
    Wire.begin();

    // Inicjalizacja wyświetlacza
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println(F("SSD1306 allocation failed"));
        while (true)
            ;
    }

    // Inicjalizacja serwa
    myServo.attach(servoPin); // Podłączenie serwa do pinu

    pinMode(buttonPin1, INPUT_PULLUP);
    pinMode(buttonPin2, INPUT_PULLUP);

    // Połączenie z Wi-Fi
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    server.begin();

    display.clearDisplay();
    display.display();
}

void loop()
{
    // Obsługa przycisków
    buttonState1 = digitalRead(buttonPin1);
    if (buttonState1 != lastButtonState1)
    {
        if (buttonState1 == LOW)
        {
            Value += 1;
            if (Value > 10)
            {
                Value = 10;
            }
        }
        lastButtonState1 = buttonState1;
    }

    buttonState2 = digitalRead(buttonPin2);
    if (buttonState2 != lastButtonState2)
    {
        if (buttonState2 == LOW)
        {
            Value -= 1;
            if (Value < 3)
            {
                Value = 3;
            }
        }
        lastButtonState2 = buttonState2;
    }

    if (Value != lastValue)
    {
        Serial.println(Value);
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.setCursor(0, 0);
        display.print("Moc: ");
        display.print(Value * 10);
        display.print("%");
        display.display();
        lastValue = Value;

        // Przeliczenie wartości na kąt dla serwa
        int servoAngle = (Value - 3) * 18; // 3 to dolny zakres
        myServo.write(servoAngle);         // Ustaw kąt serwa
        delay(1000);                       // Czekaj 1 sekundę, aby serwo mogło się ustawić
    }

    // Obsługa zapytań HTTP
    WiFiClient client = server.accept();
    if (client)
    {
        String currentLine = "";
        header = ""; // Upewnij się, że nagłówek jest pusty na początku

        while (client.connected())
        {
            if (client.available())
            {
                char c = client.read();
                Serial.write(c);
                header += c;

                if (c == '\n')
                {
                    if (currentLine.length() == 0)
                    {
                        // Wysłanie odpowiedzi HTTP
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type: application/json");
                        client.println("Connection: close");
                        client.println();

                        // Tworzenie obiektu JSON
                        DynamicJsonDocument doc(1024);

                        // Obsługa endpointów
                        if (header.indexOf("GET /api/v1/status") >= 0)
                        {
                            // Endpoint do statusu
                            doc["power"] = Value * 10;
                        }
                        else if (header.indexOf("GET /api/v1/set") >= 0)
                        {
                            // Endpoint do ustawiania wartości
                            String valueStr = header.substring(header.indexOf("set?value=") + 10);
                            int newValue = valueStr.toInt();

                            // Sprawdzanie, czy nowa wartość jest w przedziale 3-10
                            if (newValue < 3 || newValue > 10)
                            {
                                doc["error"] = "Invalid request";
                            }
                            else
                            {
                                Value = newValue;
                                doc["new_value"] = Value * 10;
                            }
                        }
                        else if (header.indexOf("GET /api/v1/ping") >= 0)
                        {
                            // Endpoint ping
                            doc["response"] = "Pong";
                        }
                        else
                        {
                            doc["error"] = "Invalid request";
                        }

                        // Serializowanie JSON i wysłanie odpowiedzi
                        serializeJson(doc, client);
                        client.println();

                        break;
                    }
                    else
                    {
                        currentLine = "";
                    }
                }
                else if (c != '\r')
                {
                    currentLine += c;
                }
            }
        }
        header = "";
        client.stop();
        Serial.println("Client disconnected.");
    }

    delay(50);
}
