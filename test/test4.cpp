#include "RovControl.h"
#include <unistd.h>
bool running = true;

int main() {
    long lastTime = 0;
    long nowTime = 0;
    int state = 0;
    RovControl ctrl;

    if(ctrl.open("localhost:8080") == true) {
        while (ctrl.processMessage()) {
            nowTime = getTimeInMs();
            //printf("[lasttime=%d nowtime=%d]\n",lastTime,nowTime);
            if(lastTime + 10000 < nowTime) {
                switch (state) {
                    case 0:
                        ctrl.setThrottle(FORWARD_VALUE);
                        break;
                    case 1:
                        ctrl.setThrottle(BACK_VALUE);
                        break;
                    case 2:
                        ctrl.setYaw(LEFT_VALUE);
                        break;
                    case 3:
                        ctrl.setYaw(RIGHT_VALUE);
                        break;
                    case 4:
                        ctrl.setLift(UP_VALUE);
                        break;
                    case 5:
                        ctrl.setLift(DOWN_VALUE);
                        break;
                    case 6:
                        ctrl.stopAll();
                        break;
                    default:
                        break;
                }
                state += 1;
                if(state > 6)
                    state = 0;
                lastTime = nowTime;
            }
            usleep(1);
        }
        ctrl.close();

    }

    return 0;
}
