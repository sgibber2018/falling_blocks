#ifndef FALLINGBLOCKS_HPP
#define FALLINGBLOCKS_HPP

#define MAX_LEVEL 13
#define BG_ANIM_WAIT 3ms
#define FRAME_WAIT 33ms
#define CLEAR_WAIT 50ms
#define DEFAULT_GRAVITY 1000
#define GAME_HEIGHT 24
#define GAME_WIDTH 80
#define LINE_SCORE 100
#define BONUS_SCORE 1000
#define LINES_TO_LEVEL 20
#define MS_DECREMENT 75
#define MS_FLOOR 45
#define BACKGROUND_TETROMINOS (term_height * term_width) / 10
#define BORDER_WIDTH 10
#define HUD_LINES 3

#include <iostream>
#include <chrono>
#include <thread>
#include <ncurses.h>
#include "Playfield.hpp"
#include "Tetromino.hpp"
#include "TetrominoGenerator.hpp"

class FallingBlocks
{
    public:
        FallingBlocks(int starting_level, bool solid_color, bool animate);
        ~FallingBlocks();

        unsigned long int game_loop();

    private:
        void init_curses(bool solid_color);
        void uninit_curses();
        void draw_hud(Coord origin);
        void draw_game();
        void draw_playfield_border(Coord origin);
        void line_clear_animation(std::vector<int> cleared);
        void level_up();
        void pause();
        void generate_background(bool game_over = false);
        void termsize_check();
        void adjust_shadow();       
 
        int convert_input(int input);
        bool is_resized();

        int level, term_height, term_width;
        unsigned long int lines_cleared, score, lines_this_level;
        bool animate;
        Playfield playfield = Playfield(PLAYFIELD_HEIGHT, PLAYFIELD_WIDTH);
        Playfield background = Playfield(GAME_HEIGHT, GAME_WIDTH);
        TetrominoGenerator generator = TetrominoGenerator();
        Tetromino tetromino = generator.next(playfield);
        Tetromino shadow = generator.next(playfield);
        std::chrono::milliseconds gravity_threshold{DEFAULT_GRAVITY};
        std::chrono::time_point<std::chrono::high_resolution_clock> gravity_clock = std::chrono::high_resolution_clock::now();
};

#endif // FALLINGBLOCKS_HPP
