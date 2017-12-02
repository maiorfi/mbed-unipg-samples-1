#include "mbed.h"

Serial debug_serial_port(SERIAL_TX, SERIAL_RX);
DigitalOut led(LED1);
DigitalIn btn(BUTTON1);
Thread thread_blink;
Thread thread_recurrent;
Thread thread_button_poll;
EventQueue eq_recurrent;
EventQueue eq_button_poll;
Timer timer;

int latest_button_state;

void event_function_button_poll()
{
    int state = !btn.read();

    if (state != latest_button_state)
    {
        debug_serial_port.printf("[DEBUG] Stato pulsante : %s\n", state ? "ON" : "OFF");

        latest_button_state = state;
    }
}

void event_function_recurrent(char c)
{
    debug_serial_port.printf("[DEBUG - event_proc_recurrent_1] Trascorsi %d ms (%c)\n", timer.read_ms(), c);
}

void thread_function_blink(DigitalOut* pled)
{
    while (true)
    {
        pled->write(!pled->read());
        wait_ms(500);
    }
}

int main()
{
    debug_serial_port.printf("[DEBUG - main] Lablet RTOS Demo #1 main()...\n");

    eq_recurrent.call_every(1000, event_function_recurrent, '*');
    eq_button_poll.call_every(10, event_function_button_poll);

    thread_blink.start(callback(thread_function_blink, &led));

    timer.start();
    thread_recurrent.start(callback(&eq_recurrent, &EventQueue::dispatch_forever));

    latest_button_state = btn.read();
    thread_button_poll.start(callback(&eq_button_poll, &EventQueue::dispatch_forever));
}
