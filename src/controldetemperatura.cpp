/**
 * @file controldetemperatura.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../lvgl.h"
#include "controldetemperatura.h"
#include "Arduino.h"
#include "stdio.h"
#include "string.h"
#include "i2s.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "freertos/queue.h"
// #include "../.pio/libdeps/esp32-s3-devkitc-1/lvgl/examples/lv_examples.h"

#define I2C_SLAVE_ADRR 0X68 // si tienes varios esclavos , cada esclavo tiene un numero de esclavo correspondiente en su hoja de especificaciones, putito
#define RX_BUFFER_LEN 3
#define TIMEOUT_MS 1000
#define DELAY_MS 1000
#define pin12scl 12
#define pin13sda 13

lv_timer_t *timer;
static lv_obj_t *arc;
lv_obj_t *label, *label2;
static lv_obj_t *spinbox;
static lv_obj_t *btn1;
lv_obj_t *sw, *sw1, *sw2;
char panel;

uint16_t tempfinal;
uint32_t tempusiario;

uint8_t rx_data[2]; // este es un bus de informacion que vamos a mandar
uint8_t comando[5]; // este comando tambien lo creamos para uso comun
uint8_t count = 0;
uint16_t tempHorno;

bool Encendido = false;
bool temUsuar = false;

static esp_err_t set_i2c(void)
{

    i2c_config_t i2c_config = {};
    i2c_config.mode = I2C_MODE_MASTER;
    i2c_config.sda_io_num = pin13sda;
    i2c_config.scl_io_num = pin12scl;
    i2c_config.sda_pullup_en = true;
    i2c_config.scl_pullup_en = true;
    i2c_config.master.clk_speed = 400000;
    i2c_config.clk_flags = 0;

    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_1, &i2c_config));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_1, I2C_MODE_MASTER, 0, 0, ESP_INTR_FLAG_LEVEL1));

    return ESP_OK;
}

void temperaturadehorno(lv_timer_t *timer) // este es un timer de peticion de temperatura que se ejecuta cada 1 segundo
{

    comando[0] = 1;  // Comando 0 es el primer comando que nos indica la funcion a realizar en el nano funcion 1 = entregar informacion del sensor de temperatura
    comando[1] = 10; // los demas comandos son de uso comun
    comando[2] = 50;
    comando[3] = 60;
    comando[4] = 70;
    i2c_master_write_read_device(I2C_NUM_1, I2C_SLAVE_ADRR, comando, sizeof(comando), rx_data, RX_BUFFER_LEN, 0); // le hacemos la peticion al nano de latemperatura del sensor de horno

    if (rx_data[0] == 1)
    {
        tempHorno = rx_data[1] * 100;
        tempHorno = tempHorno + rx_data[2];
        tempfinal = tempHorno * .25;

        printf("temperatura del horno  %.2d °c\n", tempfinal);
        lv_arc_set_value(arc, tempfinal);
        lv_label_set_text_fmt(label, "Temperatura de horno %d °C", tempfinal);
        tempusiario = lv_spinbox_get_value(spinbox);
        printf("temperatura usuario  %d °c\n", tempusiario);
    }

    if (tempusiario > tempfinal)
    {

        if (Encendido == true)
        {
            printf("Calentando\n");
            lv_label_set_text(label2, "Calentando");
        }
        else
        {
            printf("Apagado\n");
            lv_label_set_text(label2, "Apagado");
        }
    }
    else
    {
        if (Encendido == false)
        {
            printf("Apagado\n");
            lv_label_set_text(label2, "Apagado");
        }
        printf("En Descanso\n");
        lv_label_set_text(label2, "Descanso");
    }
}

void termometrosDigitales()
{
    // creamos un punto de color paara indicar si se todo el sensor tactil o no
    /*Create a LED and switch it OFF*/

    ESP_ERROR_CHECK(set_i2c()); // configuramos el i2c a usar

    label = lv_label_create(lv_scr_act());

    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xEC407A), LV_PART_MAIN); // Cambiamos el color del texto estiqueta
    lv_obj_set_pos(label, 10, 210);
    lv_label_set_recolor(label, true);

    arc = lv_arc_create(lv_scr_act());
    lv_obj_set_size(arc, 270, 250);
    lv_arc_set_rotation(arc, 150);
    lv_arc_set_range(arc, 0, 700);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_MAIN);      // cambiamos el grosor del arco
    lv_obj_set_style_arc_width(arc, 15, LV_PART_INDICATOR); // cambiamos el grosor del indicador de posicion de arco
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x3300FF), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0xCC0000), LV_PART_INDICATOR);
    lv_arc_set_bg_angles(arc, 0, 240);
    // lv_obj_center(arc);
    lv_obj_set_pos(arc, 100, 20);

    lv_timer_create(temperaturadehorno, 1000, NULL); // creamos un timer para ejecutar la rutina necesaria en este caso
                                                     // ejecutaremos la peticion de temperatura cada 1 segundo
}
static void lv_spinbox_increment_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT)
    {
        lv_spinbox_increment(spinbox);
    }
}
static void lv_spinbox_decrement_event_cb(lv_event_t *e)

{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT)
    {
        lv_spinbox_decrement(spinbox);
    }
}
static void interrupcion_evento(lv_event_t *e) // este evento interrupcion nos permite obtener datos del eventos que han ocurrido
{
    lv_event_code_t code = lv_event_get_code(e); // crea un codigo y agregale el evento qu ocurrio

    if (code == LV_EVENT_PRESSED)
    { // en este caso se pregunta si fue tocado
    }
    else if (code == LV_EVENT_RELEASED)
    { // si se todo y se solto
        Encendido = !Encendido;
    }
}

void incrementodecremento(void)
{
    spinbox = lv_spinbox_create(lv_scr_act());
    lv_spinbox_set_range(spinbox, 0, 700);
    lv_obj_set_size(spinbox, 70, 60);
    lv_obj_set_style_text_font(spinbox, &lv_font_montserrat_30, 0);
    lv_spinbox_set_value(spinbox, 0);
    lv_spinbox_set_digit_format(spinbox, 3, 3);
    lv_spinbox_step_prev(spinbox);
    lv_spinbox_set_cursor_pos(spinbox, 0);
    lv_obj_set_width(spinbox, 80);
    // lv_obj_center(spinbox);
    lv_obj_align(spinbox, LV_ALIGN_CENTER, -15, 0);

    lv_coord_t h = lv_obj_get_height(spinbox);

    lv_obj_t *btnmas = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btnmas, h, h);
    lv_obj_align_to(btnmas, spinbox, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
    lv_obj_set_style_bg_img_src(btnmas, LV_SYMBOL_PLUS, 0);
    lv_obj_add_event_cb(btnmas, lv_spinbox_increment_event_cb, LV_EVENT_ALL, NULL);

    // lv_obj_add_event_cb(btnmas,increment_event_cb,LV_EVENT_INSERT NULL);

    lv_obj_t *btnmenos = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btnmenos, h, h);
    lv_obj_align_to(btnmenos, spinbox, LV_ALIGN_OUT_LEFT_MID, -3, 0);
    lv_obj_set_style_bg_img_src(btnmenos, LV_SYMBOL_MINUS, 0);

    lv_obj_add_event_cb(btnmenos, lv_spinbox_decrement_event_cb, LV_EVENT_ALL, NULL);
}
static void eventobotones(lv_event_t *e)
{ /*The original target of the event. Can be the buttons or the container*/
    u_int8_t id = lv_obj_get_child_id(lv_event_get_target(e));
    // printf("id  %d \n", id);
    uint8_t estado = lv_obj_get_state(lv_event_get_target(e));
    // printf("estado %d \n", estado);

    if (id == 3)
    {
        printf("si fue id  %d \n", id);
        if (estado == 3)
        {
            printf("estado  %d \n", estado);
            comando[0] = id;  // Comando 0 es el primer comando que nos indica la funcion a realizar en el nano funcion 1 = entregar informacion del sensor de temperatura
            comando[1] = estado; // los demas comandos son de uso comun
            comando[2] = 50;
            comando[3] = 60;
            comando[4] = 70;
            i2c_master_write_read_device(I2C_NUM_1, I2C_SLAVE_ADRR, comando, sizeof(comando), rx_data, RX_BUFFER_LEN, 50); // le hacemos la peticion al nano de latemperatura del sensor de horno
        }
        if (estado == 2)
        {
            printf("apagado  %d \n", estado);
            comando[0] = id;  // Comando 0 es el primer comando que nos indica la funcion a realizar en el nano funcion 1 = entregar informacion del sensor de temperatura
            comando[1] = estado; // los demas comandos son de uso comun
            comando[2] = 50;
            comando[3] = 60;
            comando[4] = 70;
            i2c_master_write_read_device(I2C_NUM_1, I2C_SLAVE_ADRR, comando, sizeof(comando), rx_data, RX_BUFFER_LEN, 50); // le hacemos la peticion al nano de latemperatura del sensor de horno
        
        }
    }

    if (id == 5)
    {
        printf("si fue 5  %d \n", id);
        if (estado == 3)
        {
            printf("prendido  %d \n", estado);
             printf("estado  %d \n", estado);
            comando[0] = id;  // Comando 0 identificador de boton
            comando[1] = estado; // los demas comandos son de uso comun
            comando[2] = 50;
            comando[3] = 60;
            comando[4] = 70;
            i2c_master_write_read_device(I2C_NUM_1, I2C_SLAVE_ADRR, comando, sizeof(comando), rx_data, RX_BUFFER_LEN, 50); // le hacemos la peticion al nano de latemperatura del sensor de horno
       
        }
        if (estado == 2)
        {
             printf("apagado  %d \n", estado);
            comando[0] = id;  // Comando 0 es el primer comando que nos indica la funcion a realizar en el nano funcion 1 = entregar informacion del sensor de temperatura
            comando[1] = estado; // los demas comandos son de uso comun
            comando[2] = 50;
            comando[3] = 60;
            comando[4] = 70;
            i2c_master_write_read_device(I2C_NUM_1, I2C_SLAVE_ADRR, comando, sizeof(comando), rx_data, RX_BUFFER_LEN, 50); // le hacemos la peticion al nano de latemperatura del sensor de horno
        
        }
    }
    //}
    // i2c_master_write_read_device(I2C_NUM_1, I2C_SLAVE_ADRR, comando, sizeof(comando), rx_data, RX_BUFFER_LEN, 50); // le hacemos la peticion al nano de latemperatura del sensor de horno

    /*Make the clicked buttons red*/

    // printf("este es un if mas chingon %s °c\n", lv_obj_has_state(obj, LV_STATE_CHECKED) ? "On" : "Off");
}

// LV_LOG_USER("State: %s\n", lv_obj_has_state(obj, LV_STATE_CHECKED) ? "On" : "Off");
void botonencendido()
{
    
/* Find the image here: https://github.com/lvgl/lv_examples/tree/master/assets */
LV_IMG_DECLARE(Antarescorp);

    btn1 = lv_btn_create(lv_scr_act());
    label2 = lv_label_create(btn1);
    lv_obj_set_style_text_font(label2, &lv_font_montserrat_16, 0);
    lv_label_set_text(label2, "Apagado\n");
    lv_obj_center(label2);

    lv_obj_add_flag(btn1, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(btn1, interrupcion_evento, LV_EVENT_RELEASED, NULL);
    lv_obj_set_pos(btn1, 10, 10);
    lv_obj_set_size(btn1, 100, 90);
    /////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////

    sw1 = lv_switch_create(lv_scr_act());
    lv_obj_add_event_cb(sw1, eventobotones, LV_EVENT_RELEASED, NULL);
    lv_obj_add_flag(sw1, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_pos(sw1, 420, 10);

    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_obj_add_state(sw1, LV_STATE_PRESSED);
    lv_label_set_text(label, "Valvula de gas");
    lv_obj_align_to(label, sw1, LV_ALIGN_LEFT_MID, -110, 0);
    // lv_obj_set_size(btn1, 100, 90);
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////

    sw2 = lv_switch_create(lv_scr_act());
    lv_obj_add_state(sw2, LV_STATE_PRESSED);
    lv_obj_add_event_cb(sw2, eventobotones, LV_EVENT_RELEASED, NULL);
    lv_obj_set_pos(sw2, 420, 50);

    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Encendedor");
    lv_obj_align_to(label, sw2, LV_ALIGN_LEFT_MID, -90, 0);
    // lv_obj_set_size(btn1, 100, 90);
    /////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////
    sw = lv_switch_create(lv_scr_act());
    lv_obj_add_state(sw, LV_STATE_DISABLED);
    lv_obj_add_event_cb(sw, eventobotones, LV_EVENT_RELEASED, NULL);
    lv_obj_set_pos(sw, 420, 90);
    // lv_obj_set_size(btn1, 100, 90);
    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////
    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Control \n   de \nHumedad");
    lv_obj_align_to(label, sw, LV_ALIGN_LEFT_MID, -70, 0);
    // lv_obj_set_size(btn1, 100, 90);

    sw = lv_switch_create(lv_scr_act());
    lv_obj_add_state(sw, LV_STATE_DISABLED | LV_STATE_DISABLED);
    lv_obj_add_event_cb(sw, eventobotones, LV_EVENT_ALL, NULL);
    lv_obj_set_pos(sw, 420, 230);

    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Estado de flama");
    lv_obj_align_to(label, sw, LV_ALIGN_LEFT_MID, -125, 0);
    // lv_obj_set_size(btn1, 100, 90);
    
    lv_obj_t * img1 = lv_img_create(lv_scr_act());
    lv_img_set_src(img1, &Antarescorp);
    lv_obj_align(img1,LV_ALIGN_BOTTOM_RIGHT, -20, -50);
}

/*



*/