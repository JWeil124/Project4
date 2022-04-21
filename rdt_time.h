/*
 * File: rdt_time.h
 *
 * Header / API file for timing component of RDT library.
 *
 * DO NOT MODIFY THIS FILE!
 */
#include <sys/time.h>


/*
 * Get the current time (in milliseconds).
 *
 * @note The current time is given relative to the start of the UNIX epoch.
 *
 * @return The number of milliseconds since the UNIX epoch.
 */
int current_msec();

/*
 * Creates a timeval struct that represents the given number of milliseconds.
 *
 * @note You likely won't need to ever call this function directly in the code
 * you write. It is used by the set_timeout_val function in the ReliableSocket
 * starter code, a function you shouldn't need to modify.
 *
 * @param millis The number of milliseconds to convert.
 * @param out_timeval Pointer to timeval that will be filled in based on
 * 		millis.
 */
void msec_to_timeval(int millis, struct timeval *out_timeval);


/*
 * Converts a timeval struct to milliseconds
 *
 * @note You likely won't need to use this function. It's here only for
 * posterity.
 *
 * @param t The timeval struct to convert.
 * @return Number of milliseconds (as specified by t)
 */
int timeval_to_msec(struct timeval *t);
