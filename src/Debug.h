#ifndef DEBUG_H
#define DEBUG_H

#define DEBUGGING_MODE
#define H4_DEBUGGING
// #define LEDSTRIP_DEBUGGING

#ifdef DEBUGGING_MODE

#define DEBUG_INIT(x)   Serial.begin(x)

#define DEBUG(...)      Serial.print(__VA_ARGS__)
#define DEBUGLN(...)    Serial.println(__VA_ARGS__)
#define DEBUG_F(f_,...)     Serial.printf((f_), ##__VA_ARGS__)
#define DEBUGFPSTR(...) Serial.println(FPSTR(__VA_ARGS__))


#ifdef H4_DEBUGGING
#define H4_DEBUG(x)   x
#else
#define H4_DEBUG(x)
#endif

#ifdef LEDSTRIP_DEBUGGING
#define LEDSTRIP_DEBUG(x)   x
#else
#define LEDSTRIP_DEBUG(x)
#endif

#else

#define DEBUG_INIT(x)   

#define DEBUG(...)      
#define DEBUGLN(...)    
#define DEBUG_F(f_,...)     
#define DEBUGFPSTR(...) 
#define H4_DEBUG(x)
#define LEDSTRIP_DEBUG(x)

#endif


#endif  //DEBUG_H