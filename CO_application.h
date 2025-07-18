#ifndef CO_APPLICATION_H
#define CO_APPLICATION_H

#include "CANopen.h"

/**
 * Application interface, similar to Arduino, extended to CANopen and
 * additional, realtime thread.
 */

/**
 * Function is called once on the program startup, after Object dictionary
 * initialization and before CANopen initialization.
 *
 * @param [in,out] bitRate Stored CAN bit rate, can be overridden.
 * @param [in,out] nodeId Stored CANopen NodeId, can be overridden.
 * @param [out] errInfo Variable may indicate error information - index of
 * erroneous OD entry.
 *
 * @return @ref CO_ReturnError_t CO_ERROR_NO in case of success.
 */
CO_ReturnError_t app_programStart(uint16_t *bitRate,
                                  uint8_t *nodeId,
                                  uint32_t *errInfo);


/**
 * Function is called after CANopen communication reset.
 *
 * @param co CANopen object.
 */
void app_communicationReset(CO_t *co);


/**
 * Function is called just before program ends.
 */
void app_programEnd();


/**
 * Function is called cyclically from main().
 *
 * Place for the slower code (all must be non-blocking).
 *
 * @warning
 * Mind race conditions between this functions and following three functions
 * (app_programRt() app_peripheralRead() and app_peripheralWrite()), which all
 * run from the realtime thread. If accessing Object dictionary variable which
 * is also mappable to PDO, it is necessary to use CO_LOCK_OD() and
 * CO_UNLOCK_OD() macros from @ref CO_critical_sections.
 *
 * @param co CANopen object.
 * @param timer1usDiff Time difference since last call in microseconds
 */
void app_programAsync(CO_t *co, uint32_t timer1usDiff);


/**
 * Function is called cyclically from realtime thread at constant intervals.
 *
 * Code inside this function must be executed fast. Take care on race conditions
 * with app_programAsync.
 *
 * @param co CANopen object.
 * @param timer1usDiff Time difference since last call in microseconds
 */
void app_programRt(CO_t *co, uint32_t timer1usDiff);


/**
 * Function is called in the beginning of the realtime thread.
 *
 * @param co CANopen object.
 * @param timer1usDiff Time difference since last call in microseconds
 */
void app_peripheralRead(CO_t *co, uint32_t timer1usDiff);


/**
 * Function is called in the end of the realtime thread.
 *
 * @param co CANopen object.
 * @param timer1usDiff Time difference since last call in microseconds
 */
void app_peripheralWrite(CO_t *co, uint32_t timer1usDiff);


#endif /* CO_APPLICATION_H */
