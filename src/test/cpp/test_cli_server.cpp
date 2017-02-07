//
// Created by crem_ja on 2/3/17.
//


#include "cli_server.h"
#include "CliBridge.h"
#include <csignal>
#include <iostream>
#include <unistd.h>

using namespace std;
using namespace robotkernel;

bool stopRequested = false;

void cliServerTest();

void signalHandler(int signum ) {
   cout << "Interrupt signal (" << signum << ") received.\n";
   stopRequested=true;
}


void cliServerTest() {
    CliServer server;
    server.start();

    while(!stopRequested){
        cout << "Running...." << endl;
        sleep(2);
    }
    cout << "shutdown ..." << endl;
    server.stop();
    cout << "STOP!" << endl;
    return 0;
}

void cliBridgeTest() {
    
}

int main(int argc, char** argv){
    signal(SIGINT, signalHandler);

    cliServerTest();
    cliBridgeTest();

    return 0;
}

