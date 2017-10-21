#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <3ds.h>

#include "pp2d/pp2d.h"
#include "filestuff.h"
#include "ui.h"
#include "backup.h"

#define WHITE    RGBA8(255, 255, 255, 255)
#define GREY     RGBA8(255, 120, 120, 120)
#define GREYFG   RGBA8(120, 120, 120, 255)
#define BLACK    RGBA8( 50,  50,  50, 255)

float noWidth;

// thanks, Kingy    :^)
void cls() {
    printf("\e[1;1H\e[2J");
}

// initializes ui
void uiInit(uistruct* us) {
    us->entries = listAllFiles("/3ds/data/PayloadSpinner3DS/", &us->entryCount);
    us->entryIndex = 0;
    us->holdTimer = 0;
    us->indexPos = 0;
    
    noWidth = pp2d_get_text_width("No \uE001", 0.5f, 0.5f);
}

// prompts user for input
int uiPrompt(const char* prompt) {
    int result = -1;
    pp2d_set_screen_color(GFX_TOP, GREY);
    
    while (aptMainLoop()) {
        
        // update
        hidScanInput();
        u32 kDown = hidKeysDown();
        
        if (kDown & KEY_B) {
            result = -1;
            break;
        } else if (kDown & KEY_A) {
            result = 0;
            break;
        }
        
        // display
        pp2d_begin_draw(GFX_TOP);
            pp2d_draw_rectangle(0, 100, 400, 15, GREYFG);
            pp2d_draw_text_center(GFX_TOP, 100, 0.5f, 0.5f, WHITE, prompt);
            pp2d_draw_rectangle(0, 200, 400, 15, GREYFG);
            pp2d_draw_text(40,  200, 0.5f, 0.5f, WHITE, "Yes \uE000");
            pp2d_draw_text(360 - noWidth, 200, 0.5f, 0.5f, WHITE, "No \uE001");
        pp2d_end_draw();
    }
    
    return result;
}

void uiError(const char* error) {
    pp2d_set_screen_color(GFX_TOP, GREY);
    
    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        
        if (kDown)
            break;
        
        pp2d_begin_draw(GFX_TOP);
            pp2d_draw_rectangle(0, 100, 400, 15, GREY);
            pp2d_draw_text_center(GFX_TOP, 100, 0.5f, 0.5f, WHITE, error);
            pp2d_draw_rectangle(0, 200, 400, 15, GREY);
            pp2d_draw_text_center(GFX_TOP, 200, 0.5f, 0.5f, WHITE, "Press any key to continue.");
        pp2d_end_draw();
    }
}

// really, it's just the main application loop
void uiRun(uistruct* us) {
    int max;
    int response;
    short add;
    
    char prompt[255];
    char path[255];
    
    pp2d_set_screen_color(GFX_TOP, GREY);
    
    while (aptMainLoop()) {
        // update
        add = 0;
        
        hidScanInput();
        u32 kDown = hidKeysDown();
        
        switch (kDown) {
            case KEY_UP:
                us->entryIndex--;
                break;
            case KEY_DOWN:
                us->entryIndex++;
                add = 1;
                break;
            case KEY_RIGHT:
                us->entryIndex = us->entryIndex + 13;
                if ((us->entryIndex >= us->entryCount) && us->indexPos != ((int)ceil((float)us->entryCount / 13.0) - 1) * 13) {
                    us->entryIndex = us->entryCount - 1;
                }
                us->indexPos = us->indexPos + 13;
                break;
            case KEY_LEFT:
                us->entryIndex = us->entryIndex - 13;
                us->indexPos = us->indexPos - 13;
                break;
            case KEY_A:
                memset(prompt, 0, sizeof(prompt));
                snprintf(prompt, 255, "Replace boot.firm with %s?", us->entries[us->entryIndex]);
                response = uiPrompt(prompt);
                if (response == 0) {
                    memset(path, 0, sizeof(path));
                    snprintf(path, 255, "/3ds/data/PayloadSpinner3DS/%s", us->entries[us->entryIndex]);
                    response = backup(path);
                    if (response == 0) {
                        return;
                        break;
                    }
                }
                
                break;
            case KEY_START:
                rebootSystem();
                return;
                break;
            default:
                break;
        }
        
        if (us->entryIndex >= us->entryCount) {
            us->entryIndex = 0;
            us->indexPos = 0;
        } else if (us->entryIndex < 0) {
            us->entryIndex = us->entryCount - 1;
            us->indexPos = ((int)ceil((float)us->entryCount / 13.0) - 1) * 13;
        }
        
        if (kDown) {
            
            if (us->entryIndex < us->indexPos)
                us->indexPos = us->indexPos - 13;
            else if ((us->entryIndex % 13) == 0 && us->entryIndex != 0 && add == 1)
                us->indexPos = us->indexPos + 13;
            
            if (us->indexPos < 0)
                us->indexPos = 0;
        }
        
        printf("current index pos: %d\n", us->entryIndex);
        printf("index pos: %d\n", us->indexPos);
        
        // display
        pp2d_begin_draw(GFX_TOP);
            // draw top
            pp2d_draw_rectangle(0, 0, 400, 15, GREYFG);
            pp2d_draw_text_center(GFX_TOP, 0, 0.5f, 0.5f, WHITE, "Make a selection.");
            
            // draw bottom
            pp2d_draw_rectangle(0, 220, 400, 20, GREYFG);
            pp2d_draw_text_center(GFX_TOP, 223, 0.5f, 0.5f, WHITE, "\uE000 and \uE006 to select firm. \uE073 to exit. START to reboot.");
        
            // entry drawing
            max = ((us->indexPos + 13) < us->entryCount) ? us->indexPos + 13 : us->entryCount;
            printf("max index: %d\n", max);
            for (int i = us->indexPos; i < max; i++) {
                if (i == us->entryIndex)
                    pp2d_draw_rectangle(0, ((i % 13) * 15) + 20, 400, 15, GREYFG);
                pp2d_draw_text(0, ((i % 13) * 15) + 20, 0.5f, 0.5f, WHITE, us->entries[i]);
            }            
        pp2d_end_draw();
    }
}