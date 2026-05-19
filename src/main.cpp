#include <switch.h>
#include <stdio.h>

int main() {
    consoleInit(NULL);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    PadState pad;
    padInitializeDefault(&pad);

    printf("Hello Switch\n");
    printf("Greenlight extract preserved for later\n");
    printf("Press + to exit\n");

    while (appletMainLoop()) {
        padUpdate(&pad);

        if (padGetButtonsDown(&pad) & HidNpadButton_Plus) {
            break;
        }

        consoleUpdate(NULL);
        svcSleepThread(16666666);
    }

    consoleExit(NULL);
    return 0;
}
