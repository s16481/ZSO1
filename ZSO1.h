#pragma once
#define HAVE_STRUCT_TIMESPEC
#include <iostream>
#include <pthread.h>
#include <list>

struct Tank {
	int id;
	bool empty;
	bool inBase;
};

struct Boat {
	int id;
	int amountOfTanks;
	bool inPort;
	pthread_mutex_t busy;
};