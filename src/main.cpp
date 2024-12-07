#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Định nghĩa chân kết nối và loại cảm biến DHT
#define DHTPIN 4       // Chân kết nối DHT11
#define DHTTYPE DHT11  // DHT 11
#define D0_PIN 14
#define RELAY_PIN 18 

// Định nghĩa các tham số cho màn hình OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1  // Chân reset của màn hình OLED, -1 nếu không sử dụng

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "Easystay-2.4Ghz";
const char* password = "eS123456";
const char* mqtt_server = "broker.mqtt.cool";
const int mqtt_port = 1883;
const char* mqtt1 = "home/sensors/dht11";
const char* mqtt2 = "home/sensors/MQ2";

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Đang kết nối WiFi...");
  }
  Serial.println("Kết nối WiFi thành công!");

  // Kết nối MQTT broker
  client.setServer(mqtt_server, mqtt_port);
  while (!client.connected()) {
    Serial.println("Đang kết nối MQTT...");
    if (client.connect("ESP8266Client")) {
      Serial.println("Kết nối MQTT thành công!");
    } else {
      Serial.print("Kết nối thất bại, lỗi: ");
      Serial.print(client.state());
      delay(2000);
    }
  }
//ĐỊNH NGHĨA CHÂN CHO MQ2(CHÂN VÀO DIGTAL)-RELAY
  pinMode(D0_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
// ĐẶT CHÂN 18 Ở MỨC CAO
  Serial.println("Warming up the MQ-2 sensor");
  digitalWrite(RELAY_PIN, HIGH);
  delay(5000);
// chuẩn bị cho cảm biến dht11
  dht.begin();
  Serial.println("DHT11 sensor initialized");
 
  // Khởi động màn hình OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Địa chỉ I2C của OLED có thể khác
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  Serial.println("OLED display initialized");

  // Xóa và chuẩn bị màn hình OLED
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 30);
  display.println("XIN CHAO");
  display.display();
  delay(2000);
  
  
  //hiển thị tên wifi địa chỉ ip và cường độ tín hiệu của wifi đang kết nối trên màn hình oled
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Connected to:");
  display.println(ssid);
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.print("RSSI: ");
  display.println(WiFi.RSSI());
  display.display();
  delay(3000);
}

void loop() {
 
  // Chờ một chút giữa các lần đo
  delay(4000);

  // Đọc độ ẩm
  float h = dht.readHumidity();
  // Đọc nhiệt độ theo Celsius
  float t = dht.readTemperature();

  // Kiểm tra xem việc đọc cảm biến dht11 có thành công không
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    // In nhiệt độ và độ ẩm ra Serial
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println(" *C");
  // Tạo chuỗi JSON để gửi lên MQTT broker GỬI DỮ LIỆU ĐO ĐƯỢC TEM VÀ HUM LÊN SERVER 
    String payload = "{\"temperature\":" + String(t) + ", \"humidity\":" + String(h) + "}";
    Serial.println("Du lieu gui: " + payload);

    // Gửi dữ liệu lên MQTT broker
    if (client.publish(mqtt1, payload.c_str())) {
      Serial.println("Gửi dữ liệu thành công");
    } else {
      Serial.println("Gửi dữ liệu thất bại");
    }
  }
  if(t>=25.9 )
  {
    Serial.println("Nhiệt độ cao! CHÁY!");
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 30);
    display.setTextColor(SSD1306_WHITE);
    display.println("CHAY!!!!!!");
    display.display();
    //Kích hoạt relay (thông còi) 
    digitalWrite(RELAY_PIN, LOW);
    // Không cần gửi thêm thông tin nào nữa khi nhiệt độ quá cao
     delay(10000); 
     return;
  }
  // Đọc trạng thái khí gas
  int GasState = digitalRead(D0_PIN);
  if (GasState == HIGH) {
    Serial.println("The gas is NOT present");
    digitalWrite(RELAY_PIN, HIGH); // Turn off relay
  } else {
    Serial.println("The gas is present");
    digitalWrite(RELAY_PIN, LOW);  // Turn on relay
    Serial.println("PHÁT HIỆN KHÍ GA ");
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 30);
    display.setTextColor(SSD1306_WHITE);
    display.println("CHAY!!!!!!");
    display.display();
    delay(2500);
  }

  // Gửi trạng thái khí gas lên MQTT broker
  String gasStateStr = GasState == HIGH ? "No Gas" : "Gas Detected";
  if (client.publish(mqtt2, gasStateStr.c_str())) {
    Serial.println("Gửi trạng thái khí gas thành công");
  } else {
    Serial.println("Gửi trạng thái khí gas thất bại");
  }

  
    // Hiển thị dữ liệu đọc từ cảm biến lên màn hình OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Temp: ");
    display.print(t);
    display.println(" C");
    display.setTextSize(1);
    display.setCursor(0, 30);
    display.print("Humi: ");
    display.print(h);
    display.println(" %");
    display.display();

    
}
