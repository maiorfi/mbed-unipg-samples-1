#include "mbed.h"

Serial debug_serial_port(SERIAL_TX, SERIAL_RX); // uart per i messaggi di debug
DigitalOut led(LED1);                           // porta digitale di uscita che pilota il led (polarità su scheda: alto->acceso)
DigitalIn btn(BUTTON1);                         // porta digitale di ingresso che rileva il pulsante blu (polarità su scheda: premuto->basso)

Thread thread_blink;                            // per il task che fa lampeggiare il led
Thread thread_recurrent;                        // per il task periodico
Thread thread_button_poll;                      // per il task che rileva le variazioni di stato del pulsante

EventQueue eq_recurrent;                        // coda che mantiene le operazioni (chiamate a funzioni) del task periodico
EventQueue eq_button_poll;                      // coda che mantiene le operazioni (chiamate a funzioni) del task che rileva le variazioni di stato del pulsante

Timer timer;                                    // cronometro che misura il tempo trascorso dal boot

int latest_button_state;                        // ultimo stato pulsante rilevato

// task (avviato nel main) che rileva variazioni nello stato corrente del pulsante rispetto al precedente stato rilevato
void event_function_button_poll()
{
    // polarità "inversa" (premuto->basso, ecco perché il "NOT", ossia l'operatore "!")
    int state = !btn.read();

    // variazione rispetto all'ultimo stato?
    if (state != latest_button_state)
    {
        // messaggio di debug...
        debug_serial_port.printf("[DEBUG - event_function_button_poll] Stato pulsante : %s\n", state ? "ON" : "OFF");

        // ...e memorizzo quello corrente come ultimo stato registrato
        latest_button_state = state;
    }
}

// task (avviato nel main) eseguito periodicamente
// nota: il parametro "c" viene passato dal main(); questo è un modo per avviare più task che eseguono indipendentemente la stessa funzione ma con contesto diverso
void event_function_recurrent(char c)
{
    // messaggio di debug; legge il valore del timer/cronometro
    debug_serial_port.printf("[DEBUG - event_function_recurrent] Trascorsi %d ms (%c)\n", timer.read_ms(), c);
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

//////////
// MAIN //
//////////
int main()
{
    debug_serial_port.printf("[DEBUG - main] Lablet RTOS Demo #1 main()...\n");

    eq_recurrent.call_every(1000, event_function_recurrent, '*');                       // il task periodico viene "programmato" per essere eseguito una volta al secondo
    eq_button_poll.call_every(10, event_function_button_poll);                          // il task che rileva lo stato del pusante viene "programmato" per essere eseguito 100 volte al secondo

    thread_blink.start(callback(thread_function_blink, &led));                          // il task che effettua il toggling del led viene avviato qui, passando come parametro il puntatore alla porta di uscita da usare

    timer.start();                                                                      // il cronometro viene avviato qui
    
    thread_recurrent.start(callback(&eq_recurrent, &EventQueue::dispatch_forever));     // il task periodico viene avviato qui

    latest_button_state = !btn.read();                                                  // viene rilevato lo stato iniziale del pulsante; da qui in poi contano solo le variazioni 
    thread_button_poll.start(callback(&eq_button_poll, &EventQueue::dispatch_forever)); // il task che rileva lo stato del pulsante viene avviato qui
}
