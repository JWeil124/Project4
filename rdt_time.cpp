/*
 * File: rdt_time.cpp
 *
 * Reliable data transport (RDT) timing library implementation.
 *
 * DO NOT MODIFY THIS FILE!
 */
#include "rdt_time.h"

int timeval_to_msec(struct timeval *t) { 
	return t->tv_sec*1000+t->tv_usec/1000;
}

void msec_to_timeval(int millis, struct timeval *out_timeval) {
	out_timeval->tv_sec = millis/1000;
	out_timeval->tv_usec = (millis%1000)*1000;
}

int current_msec() {
	struct timeval t;
	gettimeofday(&t,0);
	return timeval_to_msec(&t);
}
