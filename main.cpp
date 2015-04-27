#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "gpioctrl.h"
#include "pthread.h"
#include "time.h"
#include <ctime>
#include <fstream>


using namespace std;

string in_init;
string in_scan;
bool initialized = false;
bool scanning = false;

/* LED's */
GPIOClass* yellow = new GPIOClass("14");  // 14                                                                                                                                                                  
GPIOClass* red = new GPIOClass("23");  // 15                                                                                                                                                                     
GPIOClass* green = new GPIOClass("15");  // 23                                                                                                                                                                   

/* Buttons */
GPIOClass* init = new GPIOClass("13");  // 13                                                                                                                                                                    
GPIOClass* scan = new GPIOClass("6");    // 06        

/* Function for executing commands on the UNIX platform */
int execute(char* cmd) {

	FILE *in;
	char buff[2048];

	if (!(in = popen(cmd, "r"))) {
		return 1;
	}
	while (fgets(buff, sizeof(buff), in) != NULL) {
		cout << buff;
	}

	pclose(in);
	return 0;
}

/* Thread for scanning networks */
void* execthread(void*) {

	
	red->setval_gpio("1");
	init->getval_gpio(in_init);
	scan->getval_gpio(in_scan);

	while (in_init == "1") {
		init->getval_gpio(in_init);
		usleep(200000);
	}


	cout << "Initializing wireless network.. Please wait." << endl;
	execute("airmon-ng start wlan0");
	initialized = true;
	yellow->setval_gpio("1");


	while (in_scan == "1") {
		scan->getval_gpio(in_scan);
		usleep(200000);
	}

	usleep(150000);
	cout << "Scanning for networks... Press SCAN and INIT to stop." << endl;
	scanning = true;
	red->setval_gpio("0");
	green->setval_gpio("1");

	/* Fun happens here - Scinning begins */                                                                                                                                                                                
	execute("airodump-ng --write WIFIDATA mon0");
	scanning = false;
	initialized = false;
	pthread_exit(NULL);
}

/* Main */
int main(void)
{
	pthread_t t1;


	green->export_gpio();
	yellow->export_gpio();
	red->export_gpio();
	init->export_gpio();
	scan->export_gpio();

	cout << " GPIO pins exported" << endl;

	green->setdir_gpio("out");
	yellow->setdir_gpio("out");
	red->setdir_gpio("out");
	init->setdir_gpio("in");
	scan->setdir_gpio("in");

	cout << " Set GPIO pin directions" << endl;
	string value;
	cout << " Testing LED's " << endl;
	green->setval_gpio("1");
	yellow->setval_gpio("1");
	red->setval_gpio("1");
	usleep(1000000);
	green->setval_gpio("0");
	yellow->setval_gpio("0");
	red->setval_gpio("0");
	usleep(500000);



	red->setval_gpio("1");
	init->getval_gpio(in_init);
	scan->getval_gpio(in_scan);

	if (pthread_create(&t1, NULL, execthread, (void*)NULL) != 0) {
		cout << "Error in thread." << endl;
		return 0;
	}

	while (!scanning) {
		usleep(300000);
	}

	scan->getval_gpio(in_init);

	while (in_init == "1") {
		scan->getval_gpio(in_init);
		usleep(200000);
	}

	/* Cleanup */
	green->setval_gpio("0");
	execute("pkill airodump-ng");
	execute("airmon-ng stop mon0");
	yellow->setval_gpio("0");
	pthread_cancel(t1);
	red->setval_gpio("1");

	/* Compression of data and cleanup */
	execute("zip DATA WIFI*");
	execute("mv DATA.zip /root/Documents/DATA.zip");
	execute("rm -f WIFI*");
	cout << "Exiting....." << endl;
	return 0;
}
