/*********
  Código de Servidor Web ESP32 para Análise de Segurança
  Baseado em: https://randomnerdtutorials.com/esp32-web-server-arduino-ide/
  Atividade Ponderada - Semana 9
*********/

// Carregar biblioteca Wi-Fi
#include <WiFi.h>

// Substituir com suas credenciais de rede
const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

// Definir porta do servidor web como 80
WiFiServer server(80);

// Variável para armazenar a requisição HTTP
String header;

// Variáveis auxiliares para armazenar o estado atual das saídas
String output26State = "off";
String output27State = "off";

// Atribuir variáveis de saída aos pinos GPIO
const int output26 = 26;
const int output27 = 27;

// Tempo atual
unsigned long currentTime = millis();
// Tempo anterior
unsigned long previousTime = 0; 
// Definir tempo de timeout em milissegundos (exemplo: 2000ms = 2s)
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);
  // Inicializar as variáveis de saída como saídas
  pinMode(output26, OUTPUT);
  pinMode(output27, OUTPUT);
  // Definir saídas como LOW
  digitalWrite(output26, LOW);
  digitalWrite(output27, LOW);

  // Conectar à rede Wi-Fi com SSID e senha
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Imprimir endereço IP local e iniciar servidor web
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop(){
  WiFiClient client = server.available();   // Aguardar por clientes

  if (client) {                             // Se um novo cliente conectar,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // imprimir mensagem na porta serial
    String currentLine = "";                // criar String para armazenar dados do cliente
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop enquanto o cliente está conectado
      currentTime = millis();
      if (client.available()) {             // se há bytes para ler do cliente,
        char c = client.read();             // ler um byte, então
        Serial.write(c);                    // imprimir na porta serial
        header += c;
        if (c == '\n') {                    // se o byte é um caractere de nova linha
          // se a linha atual está em branco, você recebeu dois caracteres de nova linha seguidos.
          // esse é o fim da requisição HTTP do cliente, então enviar uma resposta:
          if (currentLine.length() == 0) {
            // Cabeçalhos HTTP sempre começam com um código de resposta (ex: HTTP/1.1 200 OK)
            // e um content-type para que o cliente saiba o que está vindo, então uma linha em branco:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // ligar e desligar os GPIOs
            if (header.indexOf("GET /26/on") >= 0) {
              Serial.println("GPIO 26 on");
              output26State = "on";
              digitalWrite(output26, HIGH);
            } else if (header.indexOf("GET /26/off") >= 0) {
              Serial.println("GPIO 26 off");
              output26State = "off";
              digitalWrite(output26, LOW);
            } else if (header.indexOf("GET /27/on") >= 0) {
              Serial.println("GPIO 27 on");
              output27State = "on";
              digitalWrite(output27, HIGH);
            } else if (header.indexOf("GET /27/off") >= 0) {
              Serial.println("GPIO 27 off");
              output27State = "off";
              digitalWrite(output27, LOW);
            }
            
            // Exibir a página web HTML
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS para estilizar os botões on/off 
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Cabeçalho da página Web
            client.println("<body><h1>ESP32 Web Server</h1>");
            
            // Exibir estado atual e botões ON/OFF para GPIO 26  
            client.println("<p>GPIO 26 - State " + output26State + "</p>");
            // Se output26State está off, exibir o botão ON       
            if (output26State=="off") {
              client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            // Exibir estado atual e botões ON/OFF para GPIO 27  
            client.println("<p>GPIO 27 - State " + output27State + "</p>");
            // Se output27State está off, exibir o botão ON       
            if (output27State=="off") {
              client.println("<p><a href=\"/27/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/27/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("</body></html>");
            
            // A resposta HTTP termina com outra linha em branco
            client.println();
            // Sair do loop while
            break;
          } else { // se você recebeu uma nova linha, limpar currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // se você recebeu qualquer coisa além de um caractere de retorno,
          currentLine += c;      // adicionar ao final de currentLine
        }
      }
    }
    // Limpar a variável header
    header = "";
    // Fechar a conexão
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
