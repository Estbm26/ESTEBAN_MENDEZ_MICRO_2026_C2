/*
 JUEGO DE AGILIDAD
 ESP32 + ESP-IDF + MQTT

 Boton 1:
 Mantener presionado hasta que encienda LED

 Boton 2:
 Presionar al reaccionar

 MQTT:
 estado
 resultado
 falsa_salida
*/


#include <stdio.h>
#include <string.h>
#include <stdbool.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "nvs_flash.h"

#include "mqtt_client.h"

#include "esp_random.h"
#include "esp_timer.h"

#include "esp_netif.h"


//================ WIFI =================

#define WIFI_SSID       "Docentes_Administrativos"
#define WIFI_PASSWORD   "Adm1N2584km"


//================ MQTT =================

#define MQTT_BROKER_URI "mqtt://test.mosquitto.org:1883"


#define TOPIC_ESTADO       "juego/agilidad/estado"
#define TOPIC_REACCION1 "juego/agilidad/reaccion1"
#define TOPIC_REACCION2 "juego/agilidad/reaccion2"
#define TOPIC_FALSA        "juego/agilidad/falsa_salida"


//================ PINES =================


#define PIN_BOTON1 GPIO_NUM_22
#define PIN_BOTON2 GPIO_NUM_18
#define PIN_LED    GPIO_NUM_32



#define PRESIONADO 0
#define SUELTO     1



static const char *TAG="JUEGO";



//================ MQTT VARIABLES =================


static esp_mqtt_client_handle_t mqtt_client=NULL;

static bool mqtt_conectado=false;


static void mqtt_app_start(void);



//================ ESTADOS =================


typedef enum
{
    ESPERANDO_INICIO,
    ESPERANDO_LED,
    ESPERANDO_ACCION,
    JUEGO_TERMINADO

}estado_juego_t;



static estado_juego_t estado=ESPERANDO_INICIO;



static int64_t tiempo_inicio_espera=0;

static int64_t tiempo_suelta_boton1=0;

static int64_t tiempo_led_encendido=0;

static int64_t delay_random=0;


static bool boton1_suelto=false;



//================ MQTT EVENT =================


static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)

{

    esp_mqtt_event_handle_t event=event_data;


    switch(event->event_id)
    {


        case MQTT_EVENT_CONNECTED:

            ESP_LOGI(TAG,"MQTT conectado");

            mqtt_conectado=true;

            break;



        case MQTT_EVENT_DISCONNECTED:

            ESP_LOGW(TAG,"MQTT desconectado");

            mqtt_conectado=false;

            break;



        default:

            break;
    }

}




static void mqtt_app_start(void)

{

    esp_mqtt_client_config_t mqtt_cfg =
    {

        .broker.address.uri = MQTT_BROKER_URI,

    };



    mqtt_client=esp_mqtt_client_init(&mqtt_cfg);



    esp_mqtt_client_register_event(

        mqtt_client,
        ESP_EVENT_ANY_ID,
        mqtt_event_handler,
        NULL

    );



    esp_mqtt_client_start(mqtt_client);


}



static void mqtt_publicar(char *topic,char *mensaje)

{

    if(mqtt_conectado)

    {

        esp_mqtt_client_publish(

            mqtt_client,
            topic,
            mensaje,
            0,
            1,
            1

        );

    }

}






//================ WIFI =================



static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)

{


    if(event_base==WIFI_EVENT &&
       event_id==WIFI_EVENT_STA_START)

    {

        esp_wifi_connect();

    }


    else if(event_base==WIFI_EVENT &&
            event_id==WIFI_EVENT_STA_DISCONNECTED)

    {

        ESP_LOGW(TAG,"WiFi perdido");

        esp_wifi_connect();

    }



    else if(event_base==IP_EVENT &&
            event_id==IP_EVENT_STA_GOT_IP)

    {

        ESP_LOGI(TAG,"WiFi conectado");



        if(mqtt_client==NULL)

        {

            mqtt_app_start();

        }

    }

}



static void wifi_init(void)

{


    ESP_ERROR_CHECK(esp_netif_init());


    ESP_ERROR_CHECK(
        esp_event_loop_create_default()
    );



    esp_netif_create_default_wifi_sta();



    wifi_init_config_t cfg=WIFI_INIT_CONFIG_DEFAULT();



    ESP_ERROR_CHECK(
        esp_wifi_init(&cfg)
    );



    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL
        )
    );



    ESP_ERROR_CHECK(
        esp_event_handler_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &wifi_event_handler,
            NULL
        )
    );



    wifi_config_t wifi_config={

        .sta={

            .ssid=WIFI_SSID,

            .password=WIFI_PASSWORD,

        },

    };



    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_STA)
    );


    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_STA,
            &wifi_config
        )
    );


    ESP_ERROR_CHECK(
        esp_wifi_start()
    );



}//================ GPIO =================


static void gpio_init_juego(void)

{

    gpio_config_t botones={

        .pin_bit_mask =
        (1ULL<<PIN_BOTON1) |
        (1ULL<<PIN_BOTON2),

        .mode=GPIO_MODE_INPUT,

        .pull_up_en=GPIO_PULLUP_ENABLE,

        .pull_down_en=GPIO_PULLDOWN_DISABLE,

        .intr_type=GPIO_INTR_DISABLE

    };


    gpio_config(&botones);



    gpio_config_t led={

        .pin_bit_mask=(1ULL<<PIN_LED),

        .mode=GPIO_MODE_OUTPUT,

        .pull_up_en=GPIO_PULLUP_DISABLE,

        .pull_down_en=GPIO_PULLDOWN_DISABLE,

        .intr_type=GPIO_INTR_DISABLE

    };


    gpio_config(&led);



    gpio_set_level(PIN_LED,0);

}





//================ LOGICA DEL JUEGO =================


static void iniciar_juego(void)

{


    estado=ESPERANDO_LED;



    tiempo_inicio_espera=
        esp_timer_get_time();



    delay_random =
        1000000 +
        (esp_random()%4000000);



    boton1_suelto=false;



    gpio_set_level(PIN_LED,0);



    ESP_LOGI(TAG,
    "Juego iniciado");


    mqtt_publicar(
        TOPIC_ESTADO,
        "ESPERANDO_LED"
    );


    mqtt_publicar(
        TOPIC_FALSA,
        "0"
    );


}





static void encender_led(void)

{

    gpio_set_level(PIN_LED,1);

    tiempo_led_encendido = esp_timer_get_time();

    estado=ESPERANDO_ACCION;

    ESP_LOGI(TAG,
    "YA!");

    mqtt_publicar(
        TOPIC_ESTADO,
        "LISTO"
    );


}





static void finalizar_juego(void)

{


    int64_t tiempo_us =
        esp_timer_get_time()
        -
        tiempo_suelta_boton1;



    int tiempo_ms =
        tiempo_us/1000;



    gpio_set_level(PIN_LED,0);



    estado=JUEGO_TERMINADO;



    char resultado[20];


    sprintf(resultado,
            "%d",
            tiempo_ms);



    ESP_LOGI(TAG,
    "Tiempo: %d ms",
    tiempo_ms);



    mqtt_publicar(
        TOPIC_RESULTADO,
        resultado
    );



    mqtt_publicar(
        TOPIC_ESTADO,
        "FINALIZADO"
    );


}





static void falsa_salida(void)

{


    gpio_set_level(PIN_LED,0);



    estado=JUEGO_TERMINADO;



    ESP_LOGW(TAG,
    "Falsa salida");



    mqtt_publicar(
        TOPIC_FALSA,
        "1"
    );


    mqtt_publicar(
        TOPIC_ESTADO,
        "FALSA_SALIDA"
    );



}





//================ TASK PRINCIPAL =================


static void juego_task(void *pv)

{


    ESP_LOGI(TAG,
    "Esperando boton 1");



    while(1)

    {


        int boton1 =
            gpio_get_level(PIN_BOTON1);



        int boton2 =
            gpio_get_level(PIN_BOTON2);




        switch(estado)

        {



            case ESPERANDO_INICIO:


                if(boton1==PRESIONADO)

                {

                    iniciar_juego();

                }


            break;





            case ESPERANDO_LED:



                if(boton1==SUELTO)

                {

                    falsa_salida();

                    break;

                }



                if(
                esp_timer_get_time()
                -
                tiempo_inicio_espera
                >=
                delay_random)

                {

                    encender_led();

                }



            break;







            case ESPERANDO_ACCION:



                if(!boton1_suelto &&
   boton1==SUELTO)

{

    boton1_suelto=true;


    tiempo_suelta_boton1 =
        esp_timer_get_time();


    int64_t reaccion1_us =
        tiempo_suelta_boton1 -
        tiempo_led_encendido;


    float reaccion1_s =
        reaccion1_us / 1000000.0;


    char dato1[20];


    sprintf(dato1,
            "%.3f",
            reaccion1_s);


    mqtt_publicar(
        TOPIC_REACCION1,
        dato1
    );

}




               if(boton1_suelto &&
   boton2==PRESIONADO)

{


    int64_t reaccion2_us =
        esp_timer_get_time()
        -
        tiempo_suelta_boton1;



    float reaccion2_s =
        reaccion2_us / 1000000.0;



    char dato2[20];


    sprintf(dato2,
            "%.3f",
            reaccion2_s);



    mqtt_publicar(
        TOPIC_REACCION2,
        dato2
    );



    finalizar_juego();

}

            break;






            case JUEGO_TERMINADO:



                if(boton1==SUELTO &&
                   boton2==SUELTO)

                {


                    estado=
                    ESPERANDO_INICIO;



                    ESP_LOGI(TAG,
                    "Nueva ronda");


                }


            break;


        }



        vTaskDelay(
            pdMS_TO_TICKS(5)
        );


    }


}






//================ MAIN =================


void app_main(void)

{


    ESP_ERROR_CHECK(
        nvs_flash_init()
    );



    gpio_init_juego();



    wifi_init();




    xTaskCreate(

        juego_task,

        "juego_task",

        4096,

        NULL,

        5,

        NULL

    );


}