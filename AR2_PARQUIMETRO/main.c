#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <float.h>
#include "nokia5110.h"
#include <stdint.h>
#include <avr/interrupt.h>

uint8_t status = 0;
float credito = 0;
uint8_t timer = 0;
uint8_t timerState = 0;
uint8_t tempo = 0;
uint8_t start = 0;

ISR(TIMER2_COMPA_vect)
{ // Interrupção do TIMER2, indica que passaram-se 10 segundos
    timer++;
    timerState = 1;
}

ISR(INT1_vect)
{
    if (PIND & (1 << PD3))
    {
        credito += 0.50;
    }
}

ISR(PCINT2_vect)
{
    if (PIND & (1 << PD0))
    {
        credito += 0.25;
    }
    if (PIND & (1 << PD4))
    {
        credito += 1.00;
    }
}

int main(void)
{
    cli(); // desabilita interrupções
    char creditos[30];
    char segundos[30];
    credito = 0;

    // BOTAO
    DDRB |= ((1 << PB1) | (1 << PB2) | (1 << PB3));               // saidas
    DDRD &= ~((1 << PD0) | (1 << PD3) | (1 << PD4) | (1 << PD6)); // entradas
    PORTD |= (1 << PD0) | (1 << PD3) | (1 << PD4) | (1 << PD6);
    EICRA |= (1 << ISC10); // interrupt sense control, borda de descida (falling edge) para INT1
    EIMSK |= (1 << INT1);  // enable INT1 and INT0
    PCICR |= (1 << PCIE2); // enable Pin Change Interrupt 2
    PCMSK2 |= ((1 << PCINT16) | (1 << PCINT20) | (1 << PCINT22));

    // TIMER
    TCCR2A |= (1 << WGM21); // modo CTC
    TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20); // Prescaler de 1024
    OCR2A = 156;  // (16HZ / (1024 * 10)) - 1
    TIMSK2 |= (1 << OCIE2A); // Habilita a interrupção de comparação do Timer2

    sei(); // habilita interrupções

    while (1)
    {
        start = 0;
        tempo = 10;
        PORTB &= ~(1 << PB3); // desliga vermelho
        PORTB |= (1 << PB1);  // liga verde
        // DISPLAY
        nokia_lcd_init();
        nokia_lcd_clear();
        nokia_lcd_write_string("PARQUIMETRO", 1);
        sprintf(creditos, "credito: %.2f", credito);
        nokia_lcd_set_cursor(0, 12);
        nokia_lcd_write_string(creditos, 1);
        sprintf(segundos, "tempo: %02d", tempo);
        nokia_lcd_set_cursor(0, 24);
        nokia_lcd_write_string(segundos, 1);
        nokia_lcd_render();
        // enquanto credito não chegar a 1.50
        while (credito < 1.50)
        {
            sprintf(creditos, "credito: %.2f", credito);
            nokia_lcd_set_cursor(0, 12);
            nokia_lcd_write_string(creditos, 1);
            nokia_lcd_render();
            _delay_ms(500);
        }
        // se credito é 1.50 desliga led verde
        if (credito == 1.50)
        {
            sprintf(creditos, "credito: %.2f", credito);
            nokia_lcd_set_cursor(0, 12);
            nokia_lcd_write_string(creditos, 1);
            nokia_lcd_render();
            PORTB &= ~(1 << PB1); // desliga verde
        }
        // enquanto botão não for precionado (start = 1) não faz nada
        while (!start)
        {
            if (PIND & (1 << PD6))
            {
                start = 1;
            }
        }
        // enquanto o tempo não zerar liga o led amarelo
        while (tempo > 0)
        {
            if (timerState == 1)
            {
                PORTB |= (1 << PB2); // liga amarelo
                // atualiza o tempo no display a cada 1 segundo
                sprintf(segundos, "tempo: %02d", tempo);
                nokia_lcd_set_cursor(0, 24);
                nokia_lcd_write_string(segundos, 1);
                nokia_lcd_render();
                _delay_ms(1000);
                tempo--;
                if (tempo == 0 && timer >= 10)
                {
                    credito = 0;
                    // mostra tempo e credito zerados no display
                    sprintf(creditos, "credito: %.2f", credito);
                    nokia_lcd_set_cursor(0, 12);
                    nokia_lcd_write_string(creditos, 1);
                    sprintf(segundos, "tempo: %02d", tempo);
                    nokia_lcd_set_cursor(0, 24);
                    nokia_lcd_write_string(segundos, 1);
                    nokia_lcd_set_cursor(0, 36);
                    nokia_lcd_write_string("sem creditos", 1);
                    nokia_lcd_render();
                    PORTB &= ~(1 << PB2); // desliga amarelo
                    timerState = 0;
                    while (credito == 0) // enquanto não adiciona mais credito
                    {//pisca led vermelho
                        PORTB ^= (1 << PB3);
                        _delay_ms(500); 
                        PORTB ^= (1 << PB3);
                        _delay_ms(500);
                    }
                }
            }
        }
    }
}