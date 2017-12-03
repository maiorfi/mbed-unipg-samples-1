#include "mbed.h"

Serial debug_serial_port(SERIAL_TX, SERIAL_RX); // uart per i messaggi di debug
DigitalOut led(LED1);                           // porta digitale di uscita che pilota il led (polarità su scheda: alto->acceso)
InterruptIn btnInt(BUTTON1);                    // oggetto per la gestione degli interrupt relativi alla porta digitale di ingresso collegata al pulsante blu (polarità su scheda: premuto->basso)

Thread thread_blink;                            // per il task che fa lampeggiare il led
Thread thread_recurrent;                        // per il task periodico
Thread thread_button_queue_handler;             // per il task che esegue la callback creata nella coda dal gestore di interrupt del pulsante

EventQueue eq_recurrent;                        // coda che mantiene le operazioni (chiamate a funzioni) del task periodico
EventQueue eq_button_interrupt;                 // coda che mantiene le operazioni (chiamate a funzioni, o callback) generate dal gestore di interrupt del pulsante

Timer timer;                                    // cronometro che misura il tempo trascorso dal boot

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

void event_interrupt_handler(bool state)
{
    debug_serial_port.printf("[DEBUG - btn_interrupt_handler] Interrupt pulsante: %s\n", state ? "L->H" : "H->L");
}

void btn_interrupt_handler()
{
    // Le funzioni di standard I/O della libreria standard del C non sono "interrupt-safe" (né thread-safe, peraltro), quindi la chiamata a printf() che segue NON FUNZIONA!
    // In particolare il firmware genera l'errore: "Mutex 0x20000c44 error -6: Not allowed in ISR context" (0x20000c44 è un indirizzo in RAM e può variare da caso a caso)
    
    // debug_serial_port.printf("[DEBUG - btn_interrupt_handler] Interrupt pulsante: %s\n", btnInt.read() ? "L->H" : "H->L");

    // Questo invece lo posso fare, perché l'accodamento in una EventQueue è interrupt-safe (e pure thread-safe, peraltro)
    eq_button_interrupt.call(&event_interrupt_handler, btnInt.read());

    // Da provare anche (esecuzione differita di 500ms):
    //eq_button_interrupt.call_in(500, &event_interrupt_handler, btnInt.read());
}

//////////
// MAIN //
//////////
int main()
{
    debug_serial_port.printf("[DEBUG - main] Lablet RTOS Demo #1 main()...\n");

    eq_recurrent.call_every(1000, &event_function_recurrent, '*');                                       // il task periodico viene "programmato" per essere eseguito una volta al secondo
    
    thread_blink.start(callback(&thread_function_blink, &led));                                          // il task che effettua il toggling del led viene avviato qui, passando come parametro il puntatore alla porta di uscita da usare

    btnInt.rise(&btn_interrupt_handler);                                                                 // registrazione handler (funzione di gestione, detta anche isr) L->H
    btnInt.fall(&btn_interrupt_handler);                                                                 // registrazione handler (funzione di gestione, detta anche isr) H->L (stessa dell'interrupt "rise")
    
    timer.start();                                                                                      // il cronometro viene avviato qui
    
    thread_recurrent.start(callback(&eq_recurrent, &EventQueue::dispatch_forever));                     // il task periodico viene avviato qui
    thread_button_queue_handler.start(callback(&eq_button_interrupt, &EventQueue::dispatch_forever));   // il task che esegue lo scodamento ed esecuzione delle callback create dall'interrupt viene avviato qui
}
