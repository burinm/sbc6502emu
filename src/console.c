#include <stdio.h>
#include <sys/stat.h>
#include <SDL2/SDL.h>
#include <unistd.h>
#include <pthread.h>
#include "6522.h"

extern uint8_t green_led_array_portb;
extern uint8_t green_led_array_porta;
extern pthread_mutex_t mem_lock;

void clrScr(SDL_Renderer *r);
void renderScr(SDL_Renderer *r, SDL_Texture *t);
void drawFrame();
void drawBox(uint8_t x, uint8_t y, Uint32 c);

Uint32 *screen;
#define SCREEN_X 320
#define SCREEN_Y 200
#define SCREEN_SCALE 3

SDL_Renderer *r=NULL;
SDL_Texture *t= NULL;

int console_main()
{
SDL_Window *w=NULL;
SDL_Surface *s=NULL;

if (SDL_Init(SDL_INIT_VIDEO) < 0) { goto bail;}

if ( (w = SDL_CreateWindow("Emu 6502", SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED, SCREEN_X * SCREEN_SCALE, SCREEN_Y *SCREEN_SCALE, SDL_WINDOW_SHOWN))==NULL){goto bail;}


if ((r = SDL_CreateRenderer(w, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED )) == NULL) {goto bail;}

clrScr(r);

if ((t = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_X, SCREEN_Y)) == NULL) {goto bail;} 

if ((SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND)) !=0) {goto bail;}

if ((screen = calloc(SCREEN_X*SCREEN_Y * sizeof(Uint32),1)) == NULL) {goto oom;}


drawFrame();
renderScr(r,t);
return 0;

//remember to free all mallocs....
bail:
printf("Exiting: %s\n",SDL_GetError());
sleep(5);
SDL_Quit();
return 1;

oom:
SDL_Quit();
printf("Boo...oom!\n");
return 2;

}

int console_quit()
{


SDL_Quit();
return 0;
}

void consoleRefresh()
{
drawFrame();
renderScr(r,t);
}

void clrScr(SDL_Renderer *r)
{
SDL_SetRenderDrawColor(r, 0, 0, 0, 0xFF);
SDL_RenderClear(r);
SDL_RenderPresent(r);
}

void renderScr(SDL_Renderer *r, SDL_Texture *t)
{
SDL_UpdateTexture(t, NULL, screen, SCREEN_X*sizeof(Uint32));
SDL_RenderClear(r);
SDL_RenderCopy(r, t, NULL, NULL);
SDL_RenderPresent(r);
}

void drawBox(uint8_t x, uint8_t y,Uint32 c) {
    for(int row=0;row<8;row++) {
        for(int bit=0;bit<8;bit++)
        {
            Uint32* location =   screen + (8*x + bit) + (8*y*SCREEN_X + row*SCREEN_X) ;
            *location = c;
        }
    }
}

void drawFrame() {
uint8_t i;

        pthread_mutex_lock(&mem_lock);
    for(i=0;i<8;i++) {    
        if (green_led_array_portb & 1<<i) {
            drawBox(0,i, 0xFF00FF00);
        } else {
            drawBox(0,i, 0xFF0000FF);
        }
    }

    for(i=0;i<8;i++) {    
        if (green_led_array_porta & 1<<i) {
            drawBox(1,i, 0xFF00FF00);
        } else {
            drawBox(1,i, 0xFF0000FF);
        }
    }
        pthread_mutex_unlock(&mem_lock);
}

