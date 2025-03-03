/*
Projeto Pratico - Projeto Final do Curso de Capacitacao em Sistemas Embarcados - IFRN
Titulo do Projeto: EconoHome - Controle de Consumo de Energia Eletrica Residencial
Versao do Pico SDK: 1.5.1
*/

//INCLUSAO DE BIBLIOTECAS:
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/cyw43_arch.h"  //Biblioteca para conexao wi-fi
#include "pico/stdlib.h"      //Biblioteca padrao stdlib.h para Pico
#include "lwip/tcp.h"         //Biblioteca para comunicacao TCP/IP
#include "ssd1306.h"          //Biblioteca para o display OLED
#include "hardware/i2c.h"     //Biblioteca para comunicacao I2C
#include "hardware/pwm.h"     //Biblioteca para PWM
#include "hardware/clocks.h"  //Biblioteca para manipular sinais de clock

//DEFINICAO DE PINOS DO LED RGB E O BUZZER
#define LED_B 12
#define LED_R 13
#define LED_G 11
#define BUZZER_PIN 21

//DEFINICAO DAS CREDENCIAIS DO WI-FI HARDCODED (MUDAR PARA SUA REDE LOCAL)
#define WIFI_SSID "XXXXX"
#define WIFI_PASS "XXXXX"

//DEFINICAO DE PINOS DE COMUNICACAO I2C PARA O DISPLAY OLED
#define I2C_PORT i2c1
#define I2C_SDA 15
#define I2C_SCL 14

//CONTROLE DO DISPLAY OLED
ssd1306_t disp;

//ENVIO DE DADOS DO SERVIDOR WEB LOCAL AO THINGSPEAK VIA HTTP (SUBSTITUIR A API KEY XXXXXXXXXXXXXXXX PARA SUA API KEY)
void send_to_thingspeak(int result, int result_max_min) {
    char request[256];
    snprintf(request, sizeof(request),
             "GET /update?api_key=XXXXXXXXXXXXXXXX&field1=%d&field2=%d HTTP/1.1\r\n"
             "Host: api.thingspeak.com\r\n"
             "Connection: close\r\n\r\n",
             result, result_max_min);

    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) return;
    
    ip_addr_t server_ip;
    //endereco IP do website do ThingSpeak
    if (!ipaddr_aton("184.106.153.149", &server_ip)) return;

    if (tcp_connect(pcb, &server_ip, 80, NULL) == ERR_OK) {
        tcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    }
}

//INICIALIZACAO DO PWM DO BUZZER
void pwm_init_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (1000 * 4096));
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0);
}

//ACIONAMENTO DO BEEP DO BUZZER
void beeping(uint pin) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    for (int i = 0; i < 5; i++) {
        pwm_set_gpio_level(pin, 2048);
        sleep_ms(500);
        pwm_set_gpio_level(pin, 0);
        sleep_ms(500);
    }
}

//CONFIGURACAO INICIAL DO DISPLAY OLED VIA I2C
void setup_display() {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, 0x3C, I2C_PORT);
    ssd1306_clear(&disp);
}

//EXIBICAO DE TEXTO NO DISPLAY OLED
void print_text(int x, int y, int size, char *msg) {
    ssd1306_draw_string(&disp, x, y, size, msg);
    ssd1306_show(&disp);
}

//GERACAO DE PAGINA WEB HTML/CSS PARA CONTROLE DA BITDOGLAB
#define HTTP_RESPONSE "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" \
                      "<!DOCTYPE html><html><head>" \
                      "<meta charset=\"UTF-8\">" \
                      "<style>" \
                      "body { font-family: Arial, sans-serif; text-align: center; background-color: #f4f4f4; }" \
                      "h1 { color: #262626; }" \
                      "h3 { color: #757575}" \
                      "form { background: white; padding: 15px; border-radius: 10px; box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.1); " \
                      "width: 45rem; margin: 20px auto; text-align: left; }" \
                      "input { padding: 8px; margin: 5px 0; border: 1px solid #ccc; border-radius: 5px; width: 18rem; text-align: center; }" \
                      "input[type='submit'] { background: #28a745; color: white; border: none; cursor: pointer; padding: 10px 15px; width: 100%; }" \
                      "input[type='submit']:hover { background: #218838; }" \
                      "p { color: #555; text-align: justify; margin: 10px; }" \
                      "b { color: #333; }" \
                      "label { font-weight: bold; display: block; margin-top: 10px; }" \
                      "</style>" \
                      "</head><body>" \
                      "<h1>EconoHome</h1>" \
                      "<h3>Controle de Consumo de Energia Elétrica Residencial</h3>" \
                      "<form action=\"/set\" method=\"GET\">" \
                      "<p>Seja bem-vindo à ferramenta de controle de consumo de energia elétrica da sua residência. Para começarmos, gostaríamos de saber o seu histórico de consumo faturado. Nos dois campos abaixo, informe, em kWh, o valor do mês em que você <b>mais consumiu</b> e o valor do mês em que você <b>menos consumiu</b> energia. Será calculada a sua média de consumo anual.</p>" \
                      "<label>Mês com maior consumo de energia, em kWh:</label>" \
                      "<input type=\"number\" name=\"value_max\">" \
                      "<br><label>Mês com menor consumo de energia, em kWh:</label>" \
                      "<input type=\"number\" name=\"value_min\">" \
                      "<p>Digite o <b>consumo total de energia elétrica em kWh medido no mês anterior</b> e o <b>consumo total de energia elétrica em kWh medido neste mês</b>, depois clique em enviar:</p>" \
                      "<label>Digite a leitura do consumo total do mês anterior:</label>" \
                      "<input type=\"number\" name=\"value1\">" \
                      "<br><label>Digite a leitura do consumo total desse mês:</label>" \
                      "<input type=\"number\" name=\"value2\">" \
                      "<br><input type=\"submit\" value=\"Enviar\">" \
                      "</form></body></html>\r\n"

//CALLBACK PARA PROCESSAR REQUISICOES HTTP
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    //Processamento das requisicoes e extracao de parametros coletados no servidor web
    char *request = (char *)p->payload;
    char *value1_str = strstr(request, "value1=");       //Consumo total de energia do mes anterior em kWh
    char *value2_str = strstr(request, "value2=");       //Consumo total de energia do mes atual em kWh
    char *value_max_str = strstr(request, "value_max="); //kWh do mes com maior consumo de energia
    char *value_min_str = strstr(request, "value_min="); //kWh do mes com menor consumo de energia
    
    //Verifica se todos os parametros necessarios (value1, value2, value_max, value_min) foram encontrados na requisição HTTP
    if (value1_str && value2_str && value_max_str && value_min_str) {
        //Converte os valores extraidos da requisicao HTTP de string para inteiro
        int value1 = atoi(value1_str + 7);
        int value2 = atoi(value2_str + 7);
        int value_max = atoi(value_max_str + 10);
        int value_min = atoi(value_min_str + 10);

        //Calcula o consumo do mes atual (diferenca entre as leituras do mes atual e do mes anterior) e armazena em result
        int result = value2 - value1;

        //Calcula a media do consumo anual com base no maior e menor consumo registrados e armazena em result_max_min
        int result_max_min = (value_max + value_min) / 2;

        //Define o limite moderado de consumo, considerando 25 por cento acima da media anual e armazena em moderate_limit
        int moderate_limit = result_max_min + (result_max_min * 0.25);
        
        //Define um buffer para armazenar a string formatada com o consumo do mes
        char buffer[20];

        //Define um buffer para armazenar a classificacao do nivel de consumo (LEVE, MODERADO ou ALTO).
        char level[10];

        //Exibicao do consumo em kWh no display OLED
        snprintf(buffer, sizeof(buffer), "Valor: %d kWh", result);

        //Envia dados do consumo do mes atual e a media de consumo anual para o ThingSpeak
        send_to_thingspeak(result, result_max_min);

        //Estrutura de selecao para controle dos LEDs e do buzzer de acordo com o consumo de energia
        if (result < result_max_min) {
            //Se o consumo do mes for menor que a media anual, o LED fica verde (indicando consumo baixo)
            gpio_put(LED_G, 1);
            gpio_put(LED_R, 0);
            gpio_put(LED_B, 0);
            //O display OLED e atualizado com o consumo do mes e a classificacao LEVE
            strcpy(level, "LEVE");
        } else if (result >= result_max_min && result <= moderate_limit) {
            //Se o consumo estiver entre a media anual e o limite moderado, o LED fica amarelo (indicando consumo moderado)
            gpio_put(LED_G, 1);
            gpio_put(LED_R, 1);
            gpio_put(LED_B, 0);
            //O display OLED e atualizado com o consumo do mes e a classificacao MODERADO
            strcpy(level, "MODERADO");
        } else {
            //Se o consumo for maior que o limite moderado, acende o LED vermelho (indicando consumo alto)
            gpio_put(LED_G, 0);
            gpio_put(LED_R, 1);
            gpio_put(LED_B, 0);
            //O display OLED e atualizado com o consumo do mes e a classificacao ALTO
            strcpy(level, "ALTO");
            //Ativa o buzzer para alerta sonoro
            beeping(BUZZER_PIN);
        }

        //Limpa o display OLED
        ssd1306_clear(&disp);

        //Exibe no display OLED o consumo calculado
        print_text(0, 0, 1, buffer);

        //Exibe no display OLED o nivel de consumo (LEVE, MODERADO ou ALTO)
        print_text(0, 16, 1, level);
    }

    //Envia a resposta HTTP para o navegador com a pagina web
    tcp_write(tpcb, HTTP_RESPONSE, strlen(HTTP_RESPONSE), TCP_WRITE_FLAG_COPY);

    //Libera a memoria do buffer de rede apos o processamento da requisicao
    pbuf_free(p);

    return ERR_OK;
}

//CALLBACK DE CONEXAO PARA O SERVIDOR HTTP
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_callback);
    return ERR_OK;
}

//INICIALIZACAO DO SERVIDOR HTTP NA PORTA 80
static void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB\n");
        return;
    }
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);
    printf("Servidor HTTP rodando na porta 80...\n");
}

//FUNCAO PRINCIPAL
int main() {
    //Inicializacao do STDIO
    stdio_init_all();
    sleep_ms(5000);
    printf("Iniciando servidor HTTP\n");

    //Tela de boas-vindas no display OLED
    setup_display();
    print_text(0, 0, 1, "Seja bem-vindo!");
    print_text(0, 16, 1, "Acesse:");

    if (cyw43_arch_init()) {
        printf("Erro ao inicializar o Wi-Fi\n");
        return 1;
    }

    //Configurar no modo STA para conectar ao Wi-Fi
    cyw43_arch_enable_sta_mode();
    printf("Conectando ao Wi-Fi...\n");
    print_text(0, 32, 1, "Conectando ao Wi-Fi...");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        return 1;
    }

    //Gera o endereço IP
    printf("Wi-Fi conectado!\n");
    uint8_t *ip_address = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
    char ip_buffer[20];
    snprintf(ip_buffer, sizeof(ip_buffer), "%d.%d.%d.%d", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    ssd1306_clear(&disp);
    print_text(0, 0, 1, "Seja bem-vindo!");
    print_text(0, 16, 1, "Acesse:");
    print_text(0, 32, 1, ip_buffer); //Mostra o IP para acessar a pagina web no display OLED

    //Inicializacao dos pinos do LED RGB, do buzzer e inicia o servidor HTTP
    gpio_init(LED_B);
    gpio_init(LED_R);
    gpio_init(LED_G);
    gpio_set_dir(LED_B, GPIO_OUT);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_set_dir(LED_G, GPIO_OUT);
    pwm_init_buzzer(BUZZER_PIN);

    start_http_server();

    //LOOP PRINCIPAL
    while (true) {
        cyw43_arch_poll(); //Mantem a comunicacao com o chip Wi-Fi ativa 
        sleep_ms(100);
    }

    //Finaliza o recurso do Wi-Fi
    cyw43_arch_deinit();
    return 0;
} 