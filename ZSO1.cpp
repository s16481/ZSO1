#include "ZSO1.h"
/*
Cysterny dostarczają wino na statki zacumowane w porcie. Mamy 4 statki i 50 cystern. Statek może odpłynąć, 
jak zatankuje wino z przynajmniej 100 cystern. Cysterny jeżdżą miedzy portem a winnicą. W winnicy czekają na zalanie wina, 
a w porcie czekają na przelanie wina do statku. Cysterny jeżdżą, aż zatankują wszystkie 4 statki. Jeżeli jakiś statek odpłynął z towarem, 
ale wrócił pusty, w czasie kiedy pozostałe jeszcze takowały to on również bedzie jeszcze raz tankowany. 
Cysterny wracają do bazy jak nie mają statków do zatankowania. Statek po odpłynięciu z portu,sam decyduje czy do niego powróci czy nie.
*/

std::list<Boat*> port;
pthread_mutex_t listBusy = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t vineyard = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t start = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cout = PTHREAD_MUTEX_INITIALIZER;
bool started = false;

void* fun_boat(void* arg) {
	if (!started) {
		pthread_mutex_lock(&start);
		pthread_cond_wait(&cond, &start);
		pthread_mutex_unlock(&start);
	}

	Boat* boat = (Boat*)arg;
	while(true) {
		if (boat->inPort && boat->amountOfTanks == 100) {
			pthread_mutex_lock(&(boat->busy));
			boat->inPort = false;
			pthread_mutex_unlock(&(boat->busy));
			pthread_mutex_lock(&(cout));
			std::cout << "\033[1;31m" << "Boat with id: " << boat->id << " exit port." << "\033[0m" << std::endl;
			pthread_mutex_unlock(&(cout));
			pthread_mutex_lock(&listBusy);
			port.remove(boat);
			pthread_mutex_unlock(&listBusy);
		}
		if (port.empty()) {
			pthread_exit(NULL);
		}
		if(!boat->inPort) {
			//pthread_mutex_lock(&listBusy);
			if (!port.empty()) {
				pthread_mutex_lock(&(cout));
				std::cout << "\033[1;34m" << "Return boat: " << boat->id << " to port ?" << "\033[0m" << std::endl;
				char return_to_port;
				std::cin >> return_to_port;
				if (return_to_port == 'n') {
					//pthread_mutex_unlock(&listBusy);
					pthread_mutex_unlock(&(cout));
					pthread_exit(NULL);
				}
				pthread_mutex_unlock(&(cout));
				boat->amountOfTanks = 0;
				boat->inPort = true;
				pthread_mutex_lock(&listBusy);
				port.push_back(boat);
				//pthread_mutex_unlock(&(boat->busy));
				pthread_mutex_lock(&(cout));
				std::cout << "Boat: " << boat->id << " returned to port." << std::endl;
				pthread_mutex_unlock(&(cout));
			}
			pthread_mutex_unlock(&listBusy);
		}
	}
	return NULL;
}

void* fun_tank(void* arg) {
	if (!started) {
		pthread_mutex_lock(&start);
		pthread_cond_wait(&cond, &start);
		pthread_mutex_unlock(&start);
	}
	Tank* tank = (Tank*)arg;
	while(true) {
		if (tank->empty) {
			tank->inBase = false;
			pthread_mutex_lock(&vineyard);
			tank->empty = false;
			pthread_mutex_unlock(&vineyard);
			#ifdef DEBUG
			//std::cout << "Tank with id: " << tank->id << " filled." << std::endl;
			#endif // DEBUG			
		}
		else
		{
			//pthread_mutex_lock(&listBusy);
			Boat* selected_boat = nullptr;
			if (port.empty() && !tank->inBase) {
				pthread_mutex_lock(&(cout));
				std::cout << "Tank id: " << tank->id << " returned to base" << std::endl;
				pthread_mutex_unlock(&(cout));
				tank->inBase = true;
			}
			else {
				pthread_mutex_lock(&listBusy);
				for (const auto& boat : port) {
					int locked = pthread_mutex_trylock(&(boat->busy));
					if (locked == 0) {
						selected_boat = boat;
						pthread_mutex_unlock(&listBusy);
						break;
					}
				}
				if (selected_boat == nullptr) {
					selected_boat = port.back();
					pthread_mutex_unlock(&listBusy);
					pthread_mutex_lock(&(selected_boat->busy));
				}
				else if (selected_boat->amountOfTanks < 100) {
					selected_boat->amountOfTanks++;
					tank->empty = true;
#					ifdef DEBUG
					pthread_mutex_lock(&(cout));
					std::cout << "Boat with id: " << selected_boat->id << " tanked. Amount of tanks: " << selected_boat->amountOfTanks << std::endl;
					pthread_mutex_unlock(&(cout));
					#endif // DEBUG	
				}
				pthread_mutex_unlock(&(selected_boat->busy));
			}
		}
	}
	return NULL;
}

int main()
{
	#define boats 4
	pthread_t boat_thread[boats];
	Boat arr_boats[boats];
	for (int i = 0; i < boats; i++) {
		arr_boats[i] = { i, 0, true, PTHREAD_MUTEX_INITIALIZER };
		port.push_back(&arr_boats[i]);
		int exit_code = pthread_create(&boat_thread[i], NULL, fun_boat, (void*)&arr_boats[i]);
		if (exit_code == 0)
			std::cout << "Boat thread created id: " << i << std::endl;
	}
	#define tanks 50
	pthread_t tank_thread[tanks];
	Tank arr_tanks[tanks];
	for (int i = 0; i < tanks; i++) {
		arr_tanks[i] = { i, true, false };
		int exit_code = pthread_create(&tank_thread[i], NULL, fun_tank, (void*)&arr_tanks[i]);
		if(exit_code == 0)
			std::cout << "Tank thread created id: " << i << std::endl;
	}

	started = true;
	pthread_cond_broadcast(&cond);

	for (int i = 0; i < boats; i++) {
		errno = pthread_join(boat_thread[i], NULL);
	}
	std::cout << "Boat threads exited." << std::endl;
}
