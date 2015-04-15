#include "mbed.h"
#include "TSI_Sensor.h"

AnalogIn pot(A2);
InterruptIn zc(A3);
InterruptIn sns(A4);
DigitalOut igbt(A5);
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
Ticker snsTmr;
Ticker zcTmr;
TSIElectrode elec0(TSI_ELEC0);

// Modos de temporizacion
#define MODE_NO_DELAY   0
#define MODE_DELAY      1
// Estado de activacion 
#define STAT_TIMMING    1
// Ajuste de la intensidad
#define ACTION_DECREASING   1
#define ACTION_INCREASING   2

static float timeout,delay;
static int mode, status, action;

// Manejadores de interrupcion
static void SensorPressed_ISR(void);
static void SensorReleased_ISR(void);
static void SensorRepeated_ISR(void);
static void TimeoutDelay_ISR(void);
static void TimeoutTimming_ISR(void);
static void ZeroCross_ISR(void);

/** Al pulsar el sensor o se activa la temporizacin o se finaliza,
 *  dependiendo de como estuviera anteriormente. Si se inicia, miro
 *  si deja el sensor pulsado para ajustar la intensidad */
static void SensorPressed_ISR(void){
    if(!status){
        status = STAT_TIMMING;
        snsTmr.attach(&SensorRepeated_ISR, 1.0);
    }
    else
        status = 0;
	led1 = 1;
}

/** Al soltar el sensor, detengo el  chequeo de ajuste de intensidad */
static void SensorReleased_ISR(void){
    snsTmr.detach();
	led1 = 0;
	led2 = 0;
	led3 = 0;	
}

/** Cada segundo retoco la intensidad entre el 100% y el 50%. Cuando llega a
 *  50% vuelve a subir y viceversa */
static void SensorRepeated_ISR(void){
    if(action == ACTION_INCREASING){
        timeout = (timeout < 0.009f) ? (timeout + 0.001f) : 0.01f;
        if(timeout == 0.01f){
            action = ACTION_DECREASING;
		}
		led2 = 1;
		led3 = 0;
    }
    else{
        timeout = (timeout > 0.005f) ? (timeout - 0.001f) : 0.005f;
        if(timeout == 0.005f)
            action = ACTION_INCREASING;
		led2 = 0;
		led3 = 1;
    }
}

/** Al retardar la activacion despues del zerocross, activo el igbt y
 *  temporizo el tiempo que debe estar encendido */
static void TimeoutDelay_ISR(void){
    zcTmr.detach();
    igbt = 1;
    zcTmr.attach(&TimeoutTimming_ISR, timeout);
}

/** Al finalizar la temporizacion, desactivo el igbt y termino las 
 *  temporizaciones */
static void TimeoutTimming_ISR(void){
    zcTmr.detach();
    igbt = 0;
}

/** En los pasos por cero, o retarda o temporiza en funcin del modo */
static void ZeroCross_ISR(void){
    if(status == STAT_TIMMING){
        switch(mode){
            case MODE_DELAY:{
                if(delay > 0.0f){
                    zcTmr.attach(&TimeoutDelay_ISR, delay);
                    break;
                }
                // else continua debajo
            }
            case MODE_NO_DELAY:{
                igbt = 1;
                zcTmr.attach(&TimeoutTimming_ISR, timeout);
                break;
            }
        }
    }
    else{
        igbt = 0;
    }
}
 
int main() {
	elec0.getBaseline();
	elec0.getDelta();
	led1 = 1;led2=0;led3=0;
	wait(1);
    led1 = 1;led2=1;led3=0;
	wait(1);
    led1 = 1;led2=0;led3=1;
	wait(1);
    led1 = 0;led2=0;led3=0;
	wait(1);
    timeout = 0.01f;
    delay = 0;
    mode = (pot < 0.5f)? MODE_NO_DELAY : MODE_DELAY;
    action = 0;
    status = 0;
    igbt = 0;
    zc.fall(&ZeroCross_ISR);
    sns.fall(&SensorPressed_ISR);
    sns.rise(&SensorReleased_ISR);
    while(1) {
    }
}
