#include "10_data.h"

#include "20_init.h"
#include "30_control.h"
#include "40_process.h"
#include "50_render.h"

//###############################################
//####               RUN ZONE                ####
//###############################################
int run(){
if(!init()){ return 0; }

for(;;){
if(!control()){ break; }
process();
render();
}

return 1;
}


//###############################################
//####               MAIN ZONE               ####
//###############################################
// Entry point
int main(int argc, char** argv){
(void)argc; (void)argv; // suppress unused warnings
return !run();
}
//###############################################








