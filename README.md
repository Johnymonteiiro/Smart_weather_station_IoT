# Estação Meteorológica Inteligente com Alerta Remoto

## 📝 Descrição
Este projeto consiste em um sistema de redes sem fio desenvolvido para demonstrar a aplicação prática de comunicação IoT (Internet das Coisas). O sistema utiliza dois microcontroladores ESP32 que operam de forma integrada: um atua como **nó sensor**, coletando dados ambientais, e o outro como **nó receptor**, responsável por processar as informações e acionar alertas.

A comunicação entre os dispositivos é realizada via Wi-Fi utilizando o protocolo MQTT, garantindo uma troca de mensagens leve e eficiente. O projeto simula uma aplicação real de monitoramento ambiental com foco em saúde e bem-estar.

## 🎯 Objetivo
O principal objetivo deste projeto é criar uma solução IoT completa que integre sensores, conectividade de rede e sistemas de alerta remoto. Ele visa:
* Demonstrar a comunicação sem fio (wireless) entre dois dispositivos ESP32.
* Monitorar variáveis ambientais críticas (temperatura, umidade e pressão atmosférica) em tempo real.
* Prover alertas visuais ou sonoros automatizados baseados nas condições do ambiente.

## ⚙️ Principais Funcionalidades

### 1. Coleta de Dados Ambientais
O **Nó Sensor (ESP32 #1)** realiza leituras periódicas do ambiente utilizando:
* **DHT22:** Para medição de temperatura e umidade.
* **BMP280:** Para medição da pressão atmosférica.

### 2. Transmissão via MQTT
Os dados coletados são encapsulados e enviados via Wi-Fi para um Broker MQTT (ex: `test.mosquitto.org`), permitindo que as informações trafeguem pela rede de forma assíncrona e desacoplada.

### 3. Monitoramento e Recepção
O **Nó Receptor (ESP32 #2)** assina os tópicos do MQTT para receber os dados enviados pelo sensor. As informações podem ser visualizadas em um display OLED ou diretamente no Monitor Serial.

### 4. Sistema de Alerta Inteligente
Com base nos valores recebidos, o sistema toma decisões autônomas:
* Aciona **LEDs** ou um **Buzzer** caso a temperatura ou umidade ultrapassem limites pré-estabelecidos, indicando condições críticas.

### 5. Expansibilidade (Opcional)
O projeto está preparado para enviar dados para painéis web ou aplicativos móveis e pode ser expandido para plataformas como ThingSpeak, Node-RED ou Blynk.

---

### 🛠️ Componentes Utilizados
* 2x ESP32 (1 Sensor, 1 Receptor)
* 1x Sensor DHT22
* 1x Sensor BMP280
* 1x Buzzer ou LEDs de alerta
* Broker MQTT