/*
 * spi.c
 *
 * Created: 22.05.2022 20:49:07
 *  Author: Heinrich
 */ 

#include "spi.h"

void spi_init (void)
{
	DDRB |= (1 << PB1)|(1 << PB7);
	// spi ss
	DDRD |= (1 << PD5);
	
	SPI_SS_DESELECT;
	
	// enable spi, 2MHz (div 8), master, mode1 (cpol 0, cpha 1)
	SPCR |= (1 << SPE)|(0 << DORD)|(1 << MSTR)|(0 << CPOL)|(1 << CPHA)|(1 << SPI2X)|(1 << SPR0);
}

uint16_t spi_read_write (uint16_t data)
{
	SPI_SS_SELECT;
	uint8_t byte1, byte2;
	
	SPDR = (data >> 8);
	while(!(SPSR & (1<<SPIF)));
	byte1 = SPDR;
	byte1 &= ~((1 << 7)|(1 << 6));	// clear two most significant bits in control byte because of don't care, cj125 manual, page 16
	SPDR = (data & 0xFF);
	while(!(SPSR & (1<<SPIF)));
	byte2 = SPDR;
	
	SPI_SS_DESELECT;
	
	return make_u16t (byte1, byte2);
}