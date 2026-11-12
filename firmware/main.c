
#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

// LCD connections:
// RS = PD2, E = PD3
// D4 = PD4, D5 = PD5, D6 = PD6, D7 = PD7

#define LCD_RS PD2
#define LCD_EN PD3

void LCD_Pulse(void)
{
    PORTD |= (1 << LCD_EN);
    _delay_us(2);
    PORTD &= ~(1 << LCD_EN);
    _delay_us(50);
}

void LCD_Send4(uint8_t data)
{
    // Send 4 bits through PD4-PD7
    PORTD = (PORTD & 0x0F) | (data & 0xF0);
    LCD_Pulse();
}

void LCD_Command(uint8_t command)
{
    PORTD &= ~(1 << LCD_RS);
    LCD_Send4(command);
    LCD_Send4(command << 4);

    if (command == 0x01 || command == 0x02)
        _delay_ms(2);
}

void LCD_Data(uint8_t data)
{
    PORTD |= (1 << LCD_RS);
    LCD_Send4(data);
    LCD_Send4(data << 4);
}

void LCD_String(const char *str)
{
    while (*str)
    {
        LCD_Data(*str++);
    }
}


void LCD_Init(void)
{
    DDRD |= 0xFC;
    PORTD &= ~0xFC;

    _delay_ms(100);

    PORTD &= ~(1 << LCD_RS);

    LCD_Send4(0x30);
    _delay_ms(5);

    LCD_Send4(0x30);
    _delay_ms(5);

    LCD_Send4(0x30);
    _delay_ms(1);

    LCD_Send4(0x20);
    _delay_ms(1);

    LCD_Command(0x28);
    LCD_Command(0x08);
    LCD_Command(0x01);
    LCD_Command(0x06);
    LCD_Command(0x0C);
}


void ADC_Init(void)
{
    ADMUX = (1 << REFS0); // AVCC reference

    ADCSRA = (1 << ADEN) |
             (1 << ADPS2) |
             (1 << ADPS1); // Prescaler 64
}

uint16_t ADC_Read(uint8_t channel)
{
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

    ADCSRA |= (1 << ADSC);

    while (ADCSRA & (1 << ADSC));

    return ADC;
}

void LCD_Number(uint16_t number)
{
    char digits[6];
    uint8_t i = 0;

    do
    {
        digits[i++] = '0' + (number % 10);
        number /= 10;
    } while (number > 0);

    while (i > 0)
    {
        LCD_Data(digits[--i]);
    }
}

void PWM_Init(void)
{
    // PB3 / OC2A is the fan PWM output
    DDRB |= (1 << PB3);

    // Timer2 Fast PWM, non-inverting mode
    TCCR2A = (1 << COM2A1) |
             (1 << WGM21) |
             (1 << WGM20);

    // Prescaler = 64
    TCCR2B = (1 << CS22);

    // Fan initially OFF
    OCR2A = 0;
}

void Fan_SetSpeed(uint8_t duty)
{
    if (duty == 0)
    {
        // Disconnect PWM and force fan OFF
        TCCR2A &= ~(1 << COM2A1);
        PORTB &= ~(1 << PB3);
        OCR2A = 0;
    }
    else if (duty == 255)
    {
        // Disconnect PWM and force fan fully ON
        TCCR2A &= ~(1 << COM2A1);
        PORTB |= (1 << PB3);
        OCR2A = 255;
    }
    else
    {
        // Enable hardware PWM
        OCR2A = duty;
        TCCR2A |= (1 << COM2A1);
    }
}


int main(void)
{
    uint16_t battery_adc;
    uint16_t temperature_adc;
    uint16_t battery_mv;
    uint16_t temperature;

    // PB0 Green, PB1 Yellow, PB2 Red
    DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB2);
    // PD0 controls the cooling fan


    ADC_Init();
    LCD_Init();
    PWM_Init();

    while (1)
    {
        battery_adc = ADC_Read(0);
        temperature_adc = ADC_Read(1);

        // Simulated 3S battery: 9.0V to 12.6V
        battery_mv = 9000UL +
                     ((uint32_t)battery_adc * 3600UL / 1023UL);

        // Simulated temperature: 0 to 100C
        temperature = (uint32_t)temperature_adc *
                      100UL / 1023UL;
                      
                // Automatic cooling fan control


// Automatic PWM fan speed control
if (temperature < 40)
{
    Fan_SetSpeed(0);     // Fan OFF
}
else if (temperature < 50)
{
    Fan_SetSpeed(77);    // 30% speed
}
else if (temperature < 60)
{
    Fan_SetSpeed(153);   // 60% speed
}
else
{
    Fan_SetSpeed(255);   // Full speed
}

     

        // Turn OFF all LEDs
        PORTB &= ~((1 << PB0) | (1 << PB1) | (1 << PB2));

        // First LCD line
        LCD_Command(0x80);
        LCD_String("BAT: ");
        LCD_Number(battery_mv / 1000);
        LCD_Data('.');
        LCD_Number((battery_mv % 1000) / 100);
        LCD_Data('0' + (battery_mv % 100) / 10);
        LCD_String("V     ");

        // Second LCD line
        LCD_Command(0xC0);
        LCD_Number(temperature);
        LCD_String("C ");

        if (battery_mv < 9600 || battery_mv > 12300)
        {
            PORTB |= (1 << PB2);
            LCD_String("VOLT FAULT    ");
        }
        else if (temperature >= 60)
        {
            PORTB |= (1 << PB2);
            LCD_String("TEMP FAULT    ");
        }
        else if (temperature >= 40)
        {
            PORTB |= (1 << PB1);
            LCD_String("COOLING       ");
        }
        else
        {
            PORTB |= (1 << PB0);
            LCD_String("NORMAL        ");
        }

        _delay_ms(300);
    }
}
