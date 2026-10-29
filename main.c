#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_BRIGHTNESS 100
#define MAX_BRIGHTNESS 1000
#define MIN_BRIGHTNESS 0

SDL_Texture* create_bright_texture(SDL_Renderer* renderer, SDL_Surface* original_surface, int brightness) {
    if (!original_surface) return NULL;

    SDL_Surface* fmt_surface = SDL_ConvertSurfaceFormat(original_surface, SDL_PIXELFORMAT_RGBA32, 0);
    if (!fmt_surface) return NULL;

    if (SDL_LockSurface(fmt_surface) == 0) {
        Uint32* pixels = fmt_surface->pixels;
        int total_pixels = fmt_surface->w * fmt_surface->h;

        float factor = (float)brightness / 100.0f;

        for (int i = 0; i < total_pixels; i++) {
            Uint32 pixel = pixels[i];
            Uint8 r, g, b, a;

            SDL_GetRGBA(pixel, fmt_surface->format, &r, &g, &b, &a);

            int new_r = (int)(r * factor);
            int new_g = (int)(g * factor);
            int new_b = (int)(b * factor);

            if (new_r > 255) new_r = 255;
            if (new_g > 255) new_g = 255;
            if (new_b > 255) new_b = 255;

            pixels[i] = SDL_MapRGBA(fmt_surface->format, (Uint8)new_r, (Uint8)new_g, (Uint8)new_b, a);
        }
        SDL_UnlockSurface(fmt_surface);
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, fmt_surface);
    SDL_FreeSurface(fmt_surface);
    return texture;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Use: %s <img_path> [brightness 0-1000]\n", argv[0]);
        printf("Example use: %s photo.jpg 250\n", argv[0]);
        return 1;
    }

    const char* filepath = argv[1];
    int current_brightness = DEFAULT_BRIGHTNESS;

    if (argc >= 3) {
        current_brightness = atoi(argv[2]);
        if (current_brightness < MIN_BRIGHTNESS) current_brightness = MIN_BRIGHTNESS;
        if (current_brightness > MAX_BRIGHTNESS) current_brightness = MAX_BRIGHTNESS;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL init error: %s\n", SDL_GetError());
        return 1;
    }

    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(img_flags) & img_flags)) {
        printf("SDL_image error: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Surface* loaded_surface = IMG_Load(filepath);
    if (!loaded_surface) {
        printf("Unable to load %s! SDL_image Error: %s\n", filepath, IMG_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    int img_width = loaded_surface->w;
    int img_height = loaded_surface->h;

    SDL_Window* window = SDL_CreateWindow(
        "Bright img viewer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        img_width, img_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        printf("Window creation error: %s\n", SDL_GetError());
        SDL_FreeSurface(loaded_surface);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer creation error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_FreeSurface(loaded_surface);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Texture* texture = create_bright_texture(renderer, loaded_surface, current_brightness);

    printf("=== Controls ===\n");
    printf(" Arrows UP / DOWN : Increase / Reduce brightness (+/- 25)\n");
    printf(" Current brightness: %d (100 - default)\n", current_brightness);

    int running = 1;
    SDL_Event e;
    int need_update = 0;

    while (running) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                running = 0;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        running = 0;
                        break;
                    case SDLK_UP:
                        current_brightness += 25;
                        if (current_brightness > MAX_BRIGHTNESS) current_brightness = MAX_BRIGHTNESS;
                        need_update = 1;
                        break;
                    case SDLK_DOWN:
                        current_brightness -= 25;
                        if (current_brightness < MIN_BRIGHTNESS) current_brightness = MIN_BRIGHTNESS;
                        need_update = 1;
                        break;
                    default:
                        break;
                }
            }
        }

        if (need_update) {
            if (texture) SDL_DestroyTexture(texture);
            texture = create_bright_texture(renderer, loaded_surface, current_brightness);
            printf("Brightness changed: %d\n", current_brightness);
            need_update = 0;
        }

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        if (texture) {
            SDL_RenderCopy(renderer, texture, NULL, NULL);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(10);
    }

    if (texture) SDL_DestroyTexture(texture);
    SDL_FreeSurface(loaded_surface);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}