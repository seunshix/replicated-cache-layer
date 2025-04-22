#include <iostream>
#include <cstdlib>
#include <unistd.h>

using namespace std;

int main(){
    const char* name = getenv("NODE_NAME");
    if (name == nullptr){
        name = "unknown";
    }
    cout << "Node " << name << " is running." << endl;

    while(true){
        sleep(10);
    }
    return 0;
}