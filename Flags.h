#ifndef Flags_h
#define Flags_h

// Compilation flags used by all modules

// Define READY_FOR_FLIGHT in the final build. Any debugging code should
// have a #error and break the build if it was accidentally left in.
#define READY_FOR_FLIGHT

//#define USE_SERIAL
//#define USE_DEBUG_TRANSMIT

#define USE_FLASH

// Verify all compilation flags are set for the flight.
#ifdef READY_FOR_FLIGHT
// USE_SERIAL must be disabled
#ifdef USE_SERIAL
#error
#endif

// USE_DEBUG_TRANSMIT must be disabled
#ifdef USE_DEBUG_TRANSMIT
#error
#endif

// USE_FLASH must be enabled
#ifndef USE_FLASH
#error
#endif
#endif

// Setup cross-platform data types
#ifdef ENERGIA
// Types for use in Energia for Sprite
typedef unsigned char byte;
typedef int int16;
typedef unsigned int uint16;
typedef long int32;
typedef unsigned long uint32;
#else
// Only the ENERGIA platform is supported for flight
#ifdef READY_FOR_FLIGHT
#error
#endif
// Types for use in test code
typedef unsigned char byte;
typedef short int int16;
typedef unsigned short int uint16;
typedef int int32;
typedef unsigned int int32;
#endif

#endif

