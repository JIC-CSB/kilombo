#ifndef NKBAPI_H
#define NKBAPI_H

void set_motor(int cw, int ccw);
void _delay_ms(int t_delay);
void set_color(int r, int g, int b);
float measure_voltage();
void kprinti(int);
void kprints(char*);
void message_out(int tx0, int tx1, int tx2);
void get_message();
int measure_charge_status();

#endif
