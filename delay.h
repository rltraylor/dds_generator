// delay.h

// macro to generate a specified # of microseconds delay (F_CPU Hz crystal)
#define delay_us(us) delay_cycles(us*(F_CPU/1.0e6))

// macro to generate a specified # of milliseconds delay (F_CPU Hz crystal)
#define delay_ms(ms) delay_cycles(ms*(F_CPU/1.0e3))

// delays from 32 upto 2^24-1 cpu cycles (1 cpu cycle = 1 crystal clock cycle)
// Warning: no error catching is implemented if delay is <4uS
extern void	delay_cycles(uint32_t cpucycles);

