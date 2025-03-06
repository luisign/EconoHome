# EconoHome - Residential Electric Energy Consumption Control

**(PT-BR - Brazilian Portuguese)** Projeto final do Curso de Capacitação em Sistemas Embarcados do IFRN - "EconoHome - Controle de Consumo de Energia Elétrica Residencial" monitora, coleta informações e usa classificações visuais e sonoras para consumo elétrico residencial (em kWh), com transmissão para o ThingSpeak. Firmware exclusivo para a placa BitDogLab.

## Componentes da placa BitDogLab que o projeto utiliza:

- Raspberry Pi Pico W (RP2040);
- Display OLED (SSD1306);
- LED RGB;
- Buzzer piezoelétrico.

## De forma geral, o sistema:

- Permite que usuários insiram leituras do medidor de energia por meio de uma interface web provisória gerada localmente com comunicação Wi-Fi via protocolo HTTP para o microcontrolador RP2040 da placa de desenvolvimento BitDogLab;
- Calcula o consumo atual em kWh (quilowatts-hora) desde as últimas medições e o classifica como leve, moderado ou alto de acordo com a média anual do consumo da própria residência;
- Exibe o consumo e a classificação em um display OLED integrado na placa BitDogLab e indica também essa classificação visualmente pelas cores do LED RGB (verde, amarelo ou vermelho, respectivamente), com um alerta sonoro emitido pelo buzzer em caso de consumo excessivo;
- Envia os dados para a API do [ThingSpeak](https://thingspeak.mathworks.com/) para análise remota.

## O que pode (e deve) ser melhorado nesse projeto, futuramente:
- Integração com sensores de medição de energia, como o PZEM-004T, em vez da alimentação de dados manualmente pelo usuário;
- Desenvolvimento de um aplicativo web e/ou mobile para personalização de alertas e relatórios;
- Implementação de notificações via e-mail, SMS ou <i>push notifications</i>.
