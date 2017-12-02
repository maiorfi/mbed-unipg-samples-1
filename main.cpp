#include "mbed.h"

Serial debug_serial_port(SERIAL_TX, SERIAL_RX); // uart per i messaggi di debug
DigitalOut led(LED1);                           // porta digitale di uscita che pilota il led (polarità su scheda: alto->acceso)
InterruptIn btnInt(BUTTON1);                    // oggetto per la gestione degli interrupt relativi alla porta digitale di ingresso collegata al pulsante blu (polarità su scheda: premuto->basso)

Thread thread_blink;                            // per il task che fa lampeggiare il led
Thread thread_recurrent;                        // per il task periodico

EventQueue eq_recurrent;                        // coda che mantiene le operazioni (chiamate a funzioni) del task periodico

Timer timer;                                    // cronometro che misura il tempo trascorso dal boot

unsigned int counterLH;                         // contatore fronti basso->alto
unsigned int counterHL;                         // contatore fronti alto->basso

// task (avviato nel main) eseguito periodicamente
// nota: il parametro "c" viene passato dal main(); questo è un modo per avviare più task che eseguono indipendentemente la stessa funzione ma con contesto diverso
void event_function_recurrent(char c)
{
    // messaggio di debug; legge il valore del timer/cronometro
    debug_serial_port.printf("[DEBUG - event_function_recurrent] Trascorsi %d ms, registrati %d fronti L->H e %d fronti H->L del pulsante blu (%c)\n", timer.read_ms(), counterLH, counterHL, c);
}

// task (avviato nel main) che in un ciclo infinito effettua il toggling della porta di uscita collegata al led e attende (senza bloccare gli altri task) mezzo secondo
void thread_function_blink(DigitalOut* pled)
{
    while (true)
    {
        pled->write(!pled->read());
        wait_ms(500);
    }
}

void btn_interrupt_handler()
{
    if(btnInt.read()) counterLH++; else counterHL++;
}

//////////
// MAIN //
//////////
int main()
{
    debug_serial_port.printf("[DEBUG - main] Lablet RTOS Demo #1 main()...\n");

    eq_recurrent.call_every(1000, event_function_recurrent, '*');                       // il task periodico viene "programmato" per essere eseguito una volta al secondo
    
    thread_blink.start(callback(thread_function_blink, &led));                          // il task che effettua il toggling del led viene avviato qui, passando come parametro il puntatore alla porta di uscita da usare

    btnInt.rise(btn_interrupt_handler);                                                 // registrazione handler (funzione di gestione, detta anche isr) L->H
    btnInt.fall(btn_interrupt_handler);                                                 // registrazione handler (funzione di gestione, detta anche isr) H->L
    
    timer.start();                                                                      // il cronometro viene avviato qui
    
    thread_recurrent.start(callback(&eq_recurrent, &EventQueue::dispatch_forever));     // il task periodico viene avviato qui
}
