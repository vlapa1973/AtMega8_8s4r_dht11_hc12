/*
    ATmega8 - часы и др....
    = vlapa = 20230311 - 20230525
    v.005

    Сегменты:
              A = PD7, B = PC3, C = PB4, D = PB2
              E = PB1, F = PB0, G = PB5, d = PB3

    Разряды:
              0 = PD6, 1 = PC1, 2 = PC2, 3 = PC0
*/

#define F_CPU 8000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
// #include <string.h>

uint8_t INDICATOR = 0; //  - индикатор ОК = 1, ОА = 0

#define GLOW_TIME 50 //  яркость
const uint8_t sensorNum = 11;

char data[] = {0xC, 0xD, 0xD, 0xC}; //  данные для отображения
const uint8_t bufSize = 19;
uint8_t fraza[bufSize] = {};

uint8_t flagDot = 0;
uint8_t flagRazr1 = 0;
uint8_t flagData = 0;
uint16_t pause = 2000;

uint32_t millis = 0;
uint32_t millisOld = 0;
uint16_t pauseData = 30000;

//===============================================//
// декодер индикатора:
uint8_t dataSeg[] = {
    0b11111100, //  0
    0b01100000, //  1
    0b11011010, //  2
    0b11110010, //  3
    0b01100110, //  4
    0b10110110, //  5
    0b10111110, //  6
    0b11100000, //  7
    0b11111110, //  8
    0b11110110, //  9
    0b00000000, //  10 - пусто
    0b00000010, //  11 - минус
    0b11000110, //  12 - градус
    0b00010000, //  13 - нижнее подчеркивание
    0b00000001, //  14 - точка
    0b00101110, //  15 - h
    0b00111000  //  16 - v
};

//===============================================//
// задаем сегменты PIN:
#define A_DDR DDRD
#define A_pin PD7
#define A_PORT PORTD

#define B_DDR DDRC
#define B_pin PC3
#define B_PORT PORTC

#define C_DDR DDRB
#define C_pin PB4
#define C_PORT PORTB

#define D_DDR DDRB
#define D_pin PB2
#define D_PORT PORTB

#define E_DDR DDRB
#define E_pin PB1
#define E_PORT PORTB

#define F_DDR DDRB
#define F_pin PB0
#define F_PORT PORTB

#define G_DDR DDRB
#define G_pin PB5
#define G_PORT PORTB

#define dot_DDR DDRB
#define dot_pin PB3
#define dot_PORT PORTB

//------------//
// задаем разряды PIN:
#define r0_DDR DDRD
#define r0_pin PD6
#define r0_PORT PORTD

#define r1_DDR DDRC
#define r1_pin PC1
#define r1_PORT PORTC

#define r2_DDR DDRC
#define r2_pin PC2
#define r2_PORT PORTC

#define r3_DDR DDRC
#define r3_pin PC0
#define r3_PORT PORTC

//===============================================//
//  Декодер символов
void decoder(uint8_t s)
{
  uint8_t k = dataSeg[s];
  if (flagRazr1 && flagDot)
  {
    k++;
  }
  if (!INDICATOR)
    k = ~k;
  (k & (1 << 7)) ? A_PORT |= (1 << A_pin) : A_PORT &= ~(1 << A_pin);
  (k & (1 << 6)) ? B_PORT |= (1 << B_pin) : B_PORT &= ~(1 << B_pin);
  (k & (1 << 5)) ? C_PORT |= (1 << C_pin) : C_PORT &= ~(1 << C_pin);
  (k & (1 << 4)) ? D_PORT |= (1 << D_pin) : D_PORT &= ~(1 << D_pin);
  (k & (1 << 3)) ? E_PORT |= (1 << E_pin) : E_PORT &= ~(1 << E_pin);
  (k & (1 << 2)) ? F_PORT |= (1 << F_pin) : F_PORT &= ~(1 << F_pin);
  (k & (1 << 1)) ? G_PORT |= (1 << G_pin) : G_PORT &= ~(1 << G_pin);
  (k & (1 << 0)) ? dot_PORT |= (1 << dot_pin) : dot_PORT &= ~(1 << dot_pin);
}

//===============================================//
//
ISR(TIMER1_COMPA_vect)
{
  millis++;
}

//===============================================//
//  Гасим все сегменты и разряды
ISR(TIMER2_COMP_vect)
{
  if (!INDICATOR)
  {
    r0_PORT &= ~(1 << r0_pin);
    r1_PORT &= ~(1 << r1_pin);
    r2_PORT &= ~(1 << r2_pin);
    r3_PORT &= ~(1 << r3_pin);

    A_PORT |= (1 << A_pin);
    B_PORT |= (1 << B_pin);
    C_PORT |= (1 << C_pin);
    D_PORT |= (1 << D_pin);
    E_PORT |= (1 << E_pin);
    F_PORT |= (1 << F_pin);
    G_PORT |= (1 << G_pin);
    dot_PORT |= (1 << dot_pin);
  }
  else
  {
    r0_PORT |= (1 << r0_pin);
    r1_PORT |= (1 << r1_pin);
    r2_PORT |= (1 << r2_pin);
    r3_PORT |= (1 << r3_pin);

    A_PORT &= ~(1 << A_pin);
    B_PORT &= ~(1 << B_pin);
    C_PORT &= ~(1 << C_pin);
    D_PORT &= ~(1 << D_pin);
    E_PORT &= ~(1 << E_pin);
    F_PORT &= ~(1 << F_pin);
    G_PORT &= ~(1 << G_pin);
    dot_PORT &= ~(1 << dot_pin);
  }
}

//===============================================//
// настройка таймера Т1:
void timer_1_ini()
{
  // по сравнению:
  TCCR1B |= (1 << CS11) | (1 << WGM12); // делитель 8 (1mks), режим CTC
  TIMSK |= (1 << OCIE1A);               //  включаем прерывание по сравнению
  OCR1A = 1000;
}

//===============================================//
// настройка таймера Т2:
void timer_2_ini()
{
  // по переполнению:
  TCCR2 |= (1 << CS21);  // | (1 << CS20);  //  делитель 8
  TIMSK |= (1 << TOIE2); //  включаем прерывание по переполнению

  // по сравнению:
  OCR2 = GLOW_TIME;      //  время свечения (яркость)
  TIMSK |= (1 << OCIE2); //  включаем прерывание по сравнению
}

//===============================================//
// настройка PIN порты на выход:
void pin_ini()
{
  A_DDR |= (1 << A_pin);
  B_DDR |= (1 << B_pin);
  C_DDR |= (1 << C_pin);
  D_DDR |= (1 << D_pin);
  E_DDR |= (1 << E_pin);
  F_DDR |= (1 << F_pin);
  G_DDR |= (1 << G_pin);
  dot_DDR |= (1 << dot_pin);

  r0_DDR |= (1 << r0_pin);
  r1_DDR |= (1 << r1_pin);
  r2_DDR |= (1 << r2_pin);
  r3_DDR |= (1 << r3_pin);
}

//===============================================//
// настройка USART
void usart_ini()
{ //  скорость 9600
  UBRRH = 0;
  UBRRL = 103; //  9600;
  UCSRA |= (1 << U2X);
  UCSRB = (1 << RXCIE) | (1 << RXEN) | (1 << TXEN);   //  прием и передача
  UCSRC = (1 << URSEL) | (1 << UCSZ1) | (3 << UCSZ0); //  8 бит 1 стоп
}

//===============================================//
//  Рабочий режим (включение сегментов)
ISR(TIMER2_OVF_vect)
{
  static uint8_t countRazr = 0;
  switch (countRazr)
  {
  case 0:
    decoder(data[0]);
    if (INDICATOR)
    {
      r0_PORT &= ~(1 << r0_pin);
      r1_PORT |= (1 << r1_pin);
      r2_PORT |= (1 << r2_pin);
      r3_PORT |= (1 << r3_pin);
    }
    else
    {
      r0_PORT |= (1 << r0_pin);
      r1_PORT &= ~(1 << r1_pin);
      r2_PORT &= ~(1 << r2_pin);
      r3_PORT &= ~(1 << r3_pin);
    }
    break;

  case 1:
    flagRazr1 = 1;
    decoder(data[1]);
    if (INDICATOR)
    {
      r0_PORT |= (1 << r0_pin);
      r1_PORT &= ~(1 << r1_pin);
      r2_PORT |= (1 << r2_pin);
      r3_PORT |= (1 << r3_pin);
    }
    else
    {
      r0_PORT &= ~(1 << r0_pin);
      r1_PORT |= (1 << r1_pin);
      r2_PORT &= ~(1 << r2_pin);
      r3_PORT &= ~(1 << r3_pin);
    }
    break;

  case 2:
    flagRazr1 = 0;
    decoder(data[2]);
    if (INDICATOR)
    {
      r0_PORT |= (1 << r0_pin);
      r1_PORT |= (1 << r1_pin);
      r2_PORT &= ~(1 << r2_pin);
      r3_PORT |= (1 << r3_pin);
    }
    else
    {
      r0_PORT &= ~(1 << r0_pin);
      r1_PORT &= ~(1 << r1_pin);
      r2_PORT |= (1 << r2_pin);
      r3_PORT &= ~(1 << r3_pin);
    }
    break;

  case 3:
    decoder(data[3]);
    if (INDICATOR)
    {
      r0_PORT |= (1 << r0_pin);
      r1_PORT |= (1 << r1_pin);
      r2_PORT |= (1 << r2_pin);
      r3_PORT &= ~(1 << r3_pin);
    }
    else
    {
      r0_PORT &= ~(1 << r0_pin);
      r1_PORT &= ~(1 << r1_pin);
      r2_PORT &= ~(1 << r2_pin);
      r3_PORT |= (1 << r3_pin);
    }
    break;
  }

  (countRazr < 3) ? countRazr++ : countRazr = 0;
}

//===============================================//
void usart_send(char *str)
{
  uint8_t i = 0;
  while (str[i])
  {
    while (!(UCSRA & (1 << UDRE)))
      ;
    UDR = str[i];
    i++;
  }
}

//===============================================//
//  Прием данных USART
ISR(USART_RXC_vect)
{
  uint8_t str[bufSize];
  static uint8_t i = 0;
  uint8_t r = 0;
  r = UDR;

  if (r == 0x0D || r == 0x0A)
  {
    i = 0;
  }
  else
  {
    if (r > 0x2F && r < 0x3A) //  цифры
      r -= 0x30;
    if (r > 0x40 && r < 0x47) //  A ... F
      r -= 0x37;
    if (r == 0x2E) //  точка
      r -= 0x20;
    str[i] = r;
    (i < bufSize) ? i++ : i = 0;
  }
  uint8_t z = str[0] * 10 + str[1];
  if ((z) == sensorNum)
  {
    for (uint8_t i = 0; i < bufSize; ++i)
    {
      fraza[i] = str[i];
    }
    millisOld = millis;
  }
}

//===============================================//
int main(void)
{
  pin_ini();
  timer_1_ini();
  timer_2_ini();
  usart_ini();

  // data[0] = 0; // fraza[0];
  // data[1] = 1; // fraza[2];
  // data[2] = 5; // fraza[4];
  // data[3] = 9; // fraza[5];

  millisOld = millis;

  sei();

  //----------------------//
  while (1)
  {
    if (!flagData)
    {
      data[0] = fraza[6];
      data[1] = fraza[7];
      data[2] = fraza[9];
      data[3] = 12;
      flagDot = 1;
      _delay_ms(pause);

      flagDot = 0;
      flagRazr1 = 0;
      data[0] = 15;
      data[1] = 10;
      data[2] = fraza[3];
      data[3] = fraza[4];
      _delay_ms(pause);

      data[0] = 16;
      data[1] = fraza[11];
      data[2] = fraza[12];
      data[3] = fraza[13];
      flagDot = 1;
      _delay_ms(pause);
    }

    if (millisOld + pauseData <= millis)
    {
      for (uint8_t i = 0; i < 4; ++i)
      {
        data[i] = 0xB;
      }
      flagData = 1;
      flagDot = 0;
      _delay_ms(pause);
    }
    else
    {
      flagData = 0;
    }
  }
}

//===============================================//