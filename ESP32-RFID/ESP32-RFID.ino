#include <MFRC522.h> //biblioteca responsável pela comunicação com o módulo RFID-RC522
#include <SPI.h> //biblioteca para comunicação do barramento SPI

#include <WiFi.h> // biblioteca responsável por conexão com o Wi-Fi
#include <WiFiClient.h> // biblioteca para envio de requests HTTP

/* Configurações do Wi-FI */
const char ssid[]     = "GVT-8534";
const char password[] = "8603001076";

// Site remoto - dados do site que vai receber a requisição GET
const char http_site[] = "http://dahan-pc";

// Define a porta do servidor para 80
WiFiServer server(80);

// Variável para receber requisições HTTP
String header;

/* Configurações do módulo RC-522 */
#define SS_PIN    21
#define RST_PIN   22

//esse objeto 'chave' é utilizado para autenticação
MFRC522::MIFARE_Key key;
//código de status de retorno da autenticação
MFRC522::StatusCode status;

// Definições dos pinos do modulo RC522
MFRC522 mfrc522(SS_PIN, RST_PIN);

/* LEDs */
#define redLed    2
#define greenLed  4

void setup() {
  // Inicia a serial
  Serial.begin(115200); // 115200: velociadade do serial para esp32
  SPI.begin(); // Inicia o barramento SPI

  // Configura os LEDs
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);

  digitalWrite(greenLed, LOW);
  digitalWrite(redLed, HIGH);
  
  // Tenta conexão com Wi-fi
  WiFi.begin(ssid, password);
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(100);
    Serial.print(".");
  }

  digitalWrite(redLed, LOW);
  digitalWrite(greenLed, HIGH);
  Serial.print("\nWI-FI conectado com sucesso! IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("Acesse este site no seu navegador para comunicar-se com a ESP32.\n");
  server.begin();
  
  // Inicia MFRC522
  mfrc522.PCD_Init();
}

void loop() {
  // Espera a disponibilidade de clientes
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Novo cliente conectado");
    
    // Variável para guardar o UID do cartão RFID
    String uid = "";
    
    // String para guardar dados recebido do cliente
    String linhaAtual = "";
    
    // Ações realizadas enquanto o cliente está conectado
    while (client.connected()) {
        if (client.available()) { // Se houver dados do clientes
          char c = client.read(); // Lê esses dados
          Serial.write(c);        // Mostra os dados no Serial
          header += c;            // Salva os dados no header

          if (c == '\n') {        // se o dado for uma quebra de linha...

          // Se a linha atual estiver em branco, temos duas quebras de linha
          // Isso significa o fim da requisição HTTP do cliente.
          // É hora de mandar a resposta
          if (linhaAtual.length() == 0) {
            // Padrão de resposta HTTP:
            // Código de resposta + tipo do conteudo + linha em branco
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Lê o cartão caso seja requisitado
            if (header.indexOf("GET /ler") >= 0) {
              uid = lerCartao();
            }

            atualizarPaginaWEB(client, uid);

            // Quebra o loop while
            break;
          } else { // if you got a newline, then clear currentLine
            linhaAtual = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          linhaAtual += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Cliente disconectado.");
    Serial.println("");
  }
}

String lerCartao() {

    String uid = "";
    
    // Aguarda a aproximação do(s) cartão(ões)       
    while (!mfrc522.PICC_IsNewCardPresent()) {
        digitalWrite(greenLed, LOW);
        delay(200);
        digitalWrite(greenLed, HIGH);
    }

    // Seleciona um dos cartões (se houver mais de um)
    while (!mfrc522.PICC_ReadCardSerial()) {
        digitalWrite(greenLed, HIGH);
        delay(200);
        digitalWrite(greenLed, HIGH);
    }
    
    // Armazenando UID
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
      uid.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
      uid.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    uid.toUpperCase();

    
    digitalWrite(greenLed, HIGH);
    return uid;
}

// Monta a página WEB para exibir o UID do cartão lido
void atualizarPaginaWEB(WiFiClient client, String uid) {

    if (uid == "") {
      uid = "Nenhum cartão lido";
    }

    client.println("<!DOCTYPE html><html lang=\"pt-br\">");
    client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    client.println("<meta charset=\"utf-8\">");
    
    // CSS para estilizar a página
    client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
    client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
    client.println("text-decoration: none; font-size: 16pt; margin: 2px; cursor: pointer; border-radius: 6px;}");
    client.println(".uid { display: flex; justify-content: center;}");
    client.println(".uid p { padding: 8px; border: 2px solid black; max-width: 175px;} </style></head>");
    
    // Página WEB
    client.println("<p><a href=\"/ler\"><button class=\"button\"><b>Ler Cartão</b></button></a></p>");
    client.println("<br><div class=\"uid\"><p>" + uid + "</p></div>");
    client.println("</body></html>");

    // Script de comunicação com o site Bikeifs.com
    client.println("<script type=\"text/javascript\">");
    client.println("const dominios = [\"http://bikeifs.com\"]"); // Lista de domínios liberados
   
    // Escuta por eventos de postMessage
    client.println("window.addEventListener(\"message\", function(e) {"); /* FUNÇÃO EVENT LISTENER */
    
    client.println("if (!dominios.includes(e.origin)) return;"); // Se o site conectado não estiver liberado, retorna.
    
    client.println("const {acao, chave} = e.data"); // recupera a ação e a chave requisitada dos dados do evento
    
    // se a requisicão HTTP for do tipo GET e o valor requisitado é o UID do cartão
    client.println("if (acao == \'get\' && chave == \'uid\') {"); /* IF */ 
    client.println("var valor = \"" + uid + "\""); // salva o UID lido na variável que será enviada como resposta 
    client.println("e.source.postMessage({"); /* POST MESSAGE */
    client.println("acao: \'returnData\', chave, valor"); // Dados enviados em formato JSON 
    client.println("}, \'*\')"); /* FIM POST MESSAGE */
    client.println("}"); /* FIM IF */
    client.println("});"); /* FIM FUNÇÃO EVENT LISTENER */
  
    client.println("</script>");
    
    // The HTTP response ends with another blank line
    client.println();
    
    
}
