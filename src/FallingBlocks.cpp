#include "FallingBlocks.hpp"

using std::this_thread::sleep_for;
using std::chrono::high_resolution_clock;
using std::chrono::duration;
using std::chrono::milliseconds;
using std::vector;
using std::min;

using namespace std::chrono_literals;

FallingBlocks::FallingBlocks(int starting_level, bool solid_color, bool animate) : level(1), lines_cleared(0), score(0), lines_this_level(0), animate(animate)
{
    init_curses(solid_color);

    for (auto levels = 1; levels < min(MAX_LEVEL, starting_level); levels++)
    {
        this->animate = false;
        level_up();
        this->animate = animate;
    }

    getmaxyx(stdscr, term_height, term_width);
    generate_background();
}

FallingBlocks::~FallingBlocks()
{
    uninit_curses();
}

int FallingBlocks::convert_input(int input)
{
    int return_value = MOVE_NONE;
    switch (input) {
        case 32:
            return_value = MOVE_DROP;
        break;
        case 83:
        case 115:
            return_value = MOVE_DOWN;
        break;
        case 65:
        case 97:
            return_value = MOVE_LEFT;
        break;
        case 68:
        case 100:
            return_value = MOVE_RIGHT;
        break;
        case 70:
        case 74:
        case 102:
        case 106:
            return_value = MOVE_ROTATE;
        break;
        case 80:
        case 112:
            return_value = MOVE_PAUSE;
        break;
        case 81:
            return_value = MOVE_QUIT;
        break;
    }
    return return_value;
}

void FallingBlocks::init_curses(bool solid_color)
{
    initscr();
    noecho();
    cbreak();
    curs_set(0);
    nodelay(stdscr, true);
    if (has_colors())
    {
        start_color();
        for (auto color = 1; color <= NUM_TETROMINOS; color++)
        {
            if (solid_color)
            {
                init_pair(color, color, color);
            }
            else
            {
                init_pair(color, color, 0);
            }
        }
    }

    termsize_check();
}

void FallingBlocks::termsize_check()
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    if (rows < GAME_HEIGHT || cols < GAME_WIDTH)
    {
        printw("Please ensure your terminal window is %d rows by %d columns or greater and try again. Any key to exit.", GAME_HEIGHT, GAME_WIDTH);
        nodelay(stdscr, false);
        getch();
        endwin();
        exit(0);
    }
}

void FallingBlocks::uninit_curses()
{
    echo();
    nocbreak();
    curs_set(1);
    nodelay(stdscr, false);
    endwin();
}

void FallingBlocks::generate_background(bool game_over)
{
    background = Playfield(term_height, term_width);

    if (animate)
    {
        clear();
        if (game_over)
        {
            mvprintw(term_height / 2, term_width / 2 - 5, "GAME OVER!");
        }
        else 
        {    
            mvprintw(term_height / 2, term_width / 2 - 9, "WELCOME TO LEVEL %d", level);
        }
        refresh();
        sleep_for(1000ms);
    }

    for (auto tetrominos = 0; tetrominos < BACKGROUND_TETROMINOS; tetrominos++)
    {
        Tetromino bg_tetromino = generator.next(background);

        while (!bg_tetromino.resting(background))
        {
            bg_tetromino.attempt_move(background, MOVE_DOWN);
        }

        bg_tetromino.freeze(background);

        if (animate)
        {
            background.draw(Coord(0, 0));
            refresh();
            sleep_for(BG_ANIM_WAIT);
        }
    }
}

void FallingBlocks::draw_playfield_border(Coord origin)
{
    for (auto row = origin.get_y(); row < origin.get_y() + playfield.get_rows(); row++)
    {
        for (auto col = origin.get_x() - BORDER_WIDTH; col < origin.get_x(); col++)
        {
            char symbol = col == origin.get_x() - BORDER_WIDTH ? '|' : '>';
            mvaddch(row, col, symbol);
        }

        int start_hudside = origin.get_x() + playfield.get_cols();
        int limit_hudside = row < origin.get_y() + HUD_LINES ? term_width : start_hudside + BORDER_WIDTH;

        for (auto col = start_hudside; col < limit_hudside; col++)
        {
            char symbol = row < origin.get_y() + HUD_LINES ? ' ' : col == limit_hudside - 1 ? '|' : '<';
            mvaddch(row, col, symbol);
        }
    }

    if (term_height > playfield.get_rows())
    {
        for (auto col = origin.get_x() - BORDER_WIDTH; col < origin.get_x() + playfield.get_cols() + BORDER_WIDTH; col++)
            mvaddch(origin.get_y() + playfield.get_rows(), col, '#');
    }
}

bool FallingBlocks::is_resized()
{
    int max_height, max_width;
    getmaxyx(stdscr, max_height, max_width);

    if (max_height != term_height || max_width != term_width)
    {
        term_height = max_height;
        term_width = max_width;
        return true;
    }
    return false;
}

void FallingBlocks::adjust_shadow()
{
    int orientation = tetromino.get_orientation();
    int type = tetromino.get_type();
    Coord origin = tetromino.get_origin();

    shadow = Tetromino(origin, type, true);
    
    while (shadow.get_orientation() != orientation) 
        shadow.clockwise_rotation(playfield);

    while (!shadow.resting(playfield))
        shadow.attempt_move(playfield, MOVE_DOWN);
}

void FallingBlocks::draw_game()
{
    erase();

    if (is_resized())
    {
        bool animate = this->animate;
        this->animate = false;
        generate_background();
        this->animate = animate;
    }

    background.draw(Coord(0, 0), level);

    Coord playfield_origin = Coord(term_height / 2 - playfield.get_rows() / 2, term_width / 2 - playfield.get_cols() / 2);
    playfield.set_origin(playfield_origin);

    draw_playfield_border(playfield_origin);

    playfield.draw(playfield_origin, level);

    shadow.draw(playfield_origin, level);
    tetromino.draw(playfield_origin, level);

    draw_hud(playfield_origin);

    Tetromino next = Tetromino(Coord(4, PLAYFIELD_WIDTH + 1), generator.preview());
    next.draw(playfield_origin, level);

    refresh();
}

void FallingBlocks::draw_hud(Coord origin)
{
    int hud_y = origin.get_y(), hud_x = origin.get_x() + PLAYFIELD_WIDTH;
    mvprintw(hud_y, hud_x, "Level: %d", level);
    mvprintw(hud_y + 1, hud_x, "Lines: %d", lines_cleared);
    mvprintw(hud_y + 2, hud_x, "Score: %d", score);
    mvprintw(hud_y + 3, hud_x, "Next Up: ");
}

void FallingBlocks::line_clear_animation(vector<int> cleared)
{
    Coord origin = playfield.get_origin();


    for (auto row : cleared)
    {
        for (auto col = 0; col < playfield.get_cols(); col++)
        {
            if (has_colors())
            {
                attron(COLOR_PAIR(col % 7 + 1));
            }

            mvaddch(origin.get_y() + row, origin.get_x() + col, '#');

            if (has_colors())
            {
                attroff(COLOR_PAIR(col % 7 + 1));

            }
        }
    }
    refresh();
    sleep_for(CLEAR_WAIT);
}

void FallingBlocks::level_up()
{
    level++;

    lines_this_level %= LINES_TO_LEVEL;

    if (gravity_threshold.count() - MS_DECREMENT >= MS_FLOOR)
        gravity_threshold -= milliseconds{MS_DECREMENT};

    generate_background();
}

void FallingBlocks::pause()
{
    nodelay(stdscr, false);
    draw_game();
    mvprintw(0, 0, "GAME PAUSED -- PRESS 'P' AGAIN TO CONTINUE");
    int input = convert_input(getch());
    while (input != MOVE_PAUSE)
    {
        input = convert_input(getch());
    }
    nodelay(stdscr, true);
    draw_game();
}

unsigned long int FallingBlocks::game_loop()
{
    generator.new_batch();
    adjust_shadow();
    while (!playfield.game_over())
    {
        if (tetromino.resting(playfield))
        {
            tetromino.freeze(playfield);
            tetromino = generator.next(playfield);
            vector<int> cleared = playfield.clear_lines();
            int num_cleared = cleared.size();
            lines_cleared += num_cleared;
            lines_this_level += num_cleared;
            score = num_cleared == 4 ? score + BONUS_SCORE : score + num_cleared * LINE_SCORE;
            if (lines_this_level >= LINES_TO_LEVEL)
                level_up();
            if (num_cleared > 0)
                line_clear_animation(cleared);
            adjust_shadow();
        }

        duration<double, std::milli> elapsed = high_resolution_clock::now() - gravity_clock;
        if (elapsed > gravity_threshold)
        {
            tetromino.attempt_move(playfield, MOVE_DOWN);
            gravity_clock = high_resolution_clock::now();
        }

        int input = convert_input(getch());
        switch (input)
        {
            case MOVE_ROTATE:
                tetromino.clockwise_rotation(playfield);
                adjust_shadow();
            break;
            case MOVE_LEFT:
            case MOVE_RIGHT:
            case MOVE_DOWN:
                tetromino.attempt_move(playfield, input);
                adjust_shadow();
            break;
            case MOVE_DROP:
                while (!tetromino.resting(playfield))
                    tetromino.attempt_move(playfield, MOVE_DOWN);
            break;
            break;
            case MOVE_PAUSE:
                pause();
            break;
            case MOVE_QUIT:
                return score;
            break;
        }
        draw_game();

        sleep_for(FRAME_WAIT);
    }

    if (playfield.game_over())
        generate_background(true);

    return score;
}

