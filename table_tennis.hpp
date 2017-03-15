/**
 * A class that encapsulates the gameplay logic for table tennis.
 *
 * @author Aaron Jones <aaron@jonesinator.com>
 * @license GPLv3
 */

#ifndef __TABLE_TENNIS_HPP__
#define __TABLE_TENNIS_HPP__

/**
 * Encapsulates the data and logic for a game of table tennis. This class
 * contains just game data and logic and does not care about how the game is
 * controlled or displayed.
 */
struct table_tennis {
    /**
     * Used to select whether games should be played to eleven or twenty one
     * points. A simple bool could be used, but makes the code less readable.
     */
    enum class game_mode {
        /**
         * Games go to eleven points.
         */
        to_11,

        /**
         * Games go to twenty one points.
         */
        to_21
    };

    /**
     * Used to determine who is serving. A simple bool could be used, but makes
     * the code less readable.
     */
    enum class serve_player {
        /**
         * Player one serves first or is serving.
         */
        p1,

        /**
         * Player two serves first or is serving.
         */
        p2
    };

    /**
     * Get how many games player one has won.
     *
     * @returns An integer representing the number of games player one has won.
     */
    int get_p1_games_won() const {
        return _state.p1_games_won;
    }

    /**
     * Get how many games player two has won.
     *
     * @returns An integer representing the number of games player two has won.
     */
    int get_p2_games_won() const {
        return _state.p2_games_won;
    }

    /**
     * Gets how many points player one has scored in the current game.
     *
     * @returns An integer representing the number of points player one has
     *          scored.
     */
    int get_p1_score() const {
        return _state.p1_score;
    }

    /**
     * Gets how many points player two has scored in the current game.
     *
     * @returns An integer representing the number of points player two has
     *          scored.
     */
    int get_p2_score() const {
        return _state.p2_score;
    }

    /**
     * Determine which player is currently serving.
     *
     * @returns A serve_player enum indicating which player is serving.
     */
    serve_player serve() const {
	/**
         * In deuce the serve alternates every point. In normal play the serve
	 * alternates every every two points for eleven point mode and every
	 * five points for twenty one point mode.
         */
        int serve_interval = deuce()
                             ? 1
                             : (_state.mode == game_mode::to_11 ? 2 : 5);
        int num_intervals = (get_p1_score() + get_p2_score()) / serve_interval;
        if (num_intervals % 2 == 0) {
           return _state.first_serve == serve_player::p1
                  ? serve_player::p1
                  : serve_player::p2;
        } else {
           return _state.first_serve == serve_player::p1
                  ? serve_player::p2
                  : serve_player::p1;
        }
    }

    /**
     * Adds a point to player one's score. If player one wins then the
     * necessary game state adjustments are made automatically.
     */
    void p1_score() {
        save_state();
        _state.p1_score++;
        check_for_win();
    }

    /**
     * Adds a point to player two's score. If player one wins then the
     * necessary game state adjustments are made automatically.
     */
    void p2_score() {
        save_state();
        _state.p2_score++;
        check_for_win();
    }

    /**
     * Undoes the last point scored in the history. If there are no game states
     * in the history then this function does nothing.
     */
    void undo() {
        if (_history_index >= 0) {
            _state = _history[_history_index--];
        }
    }

    /**
     * Sets the game mode to eleven or twenty one point mode. This can only be
     * changed between matches. If this function is called in the middle of a
     * match then nothing is done.
     *
     * @param mode The desired game mode.
     */
    void set_game_mode(game_mode mode) {
        if (get_p1_score() == 0 && get_p2_score() == 0) {
            _state.mode = mode;
        }
    }

    /**
     * Sets which player serves first. This can only be changed at the start of
     * a game.  If this function is called in the middle of a match then nothing
     * is done.
     *
     * @param serve_player Which player should serve first.
     */
    void set_first_serve(serve_player first_serve_player) {
        if (get_p1_score() == 0 && get_p2_score() == 0) {
            _state.first_serve = first_serve_player;
        }
    }

private:
    /**
     * Helper structure storing all relevant information for a table tennis
     * match. This is used so it's easy to copy and store the entire game state
     * so we can maintain an array of game states to enable the undo function.
     */
    struct game_state {
        /**
         *  How many games player 1 has won.
         */
        int p1_games_won = 0;

        /**
         * How many points player 1 has scored in the current round.
         */
        int p1_score = 0;

        /**
         * How many games player 2 has won.
         */
        int p2_games_won = 0;

        /**
         * How many points player 2 has scored in the current round.
         */
        int p2_score = 0;

        /**
         * Which player served first in the game.
         */
        serve_player first_serve = serve_player::p1;

        /**
         * Whether games should be played to eleven or twenty one points.
         */
        game_mode mode = game_mode::to_11;
    };

    /**
     * Maximum number of undo levels.
     */
    static const int MAX_UNDO = 32;

    /**
     *  The current game state.
     */
    game_state _state;

    /**
     * A history of all previous game states, used to enable the undo
     * feature.
     */
    game_state _history[MAX_UNDO];

    /**
     * The current location in the history index of the latest valid game
     * state.
     */
    int _history_index = -1;

    /**
     * Saves the current game state to the history array. If the history array
     * is full then the oldest game state is discarded.
     */
    void save_state() {
        /**
         * The history array is full. Copy all of the state objects back one
         * index to make room for the new state.
         */
        if (_history_index == MAX_UNDO - 1) {
            for (int i = 0; i < MAX_UNDO - 1; ++i) {
                _history[i] = _history[i + 1];
            }
            --_history_index;
        }
        _history[++_history_index] = _state;
    }

    /**
     * Determines whether or not the deuce condition has been reached. For
     * eleven point games this happens when the score reaches 10/10. For
     * twenty one point games this happens when the score reaches 20/20.
     *
     * @returns True if the deuce condition has been reached, false otherwise.
     */
    bool deuce() const {
        int deuce_points = _state.mode == game_mode::to_11 ? 10 : 20;
        return get_p1_score() >= deuce_points && get_p2_score() >= deuce_points;
    }

    /**
     * Determines if the game has been won by either player. If the game has
     * been won then this function does the necessary updates to the game state.
     */
    void check_for_win() {
        /**
         * In deuce the game is won if one player has scored two points more
         * than the other player.
         */
        if (deuce()) {
            if (get_p1_score() >= get_p2_score() + 2) {
                _state.p1_games_won++;
                _state.p1_score = _state.p2_score = 0;
            } else if (get_p2_score() >= get_p1_score() + 2) {
                _state.p2_games_won++;
                _state.p1_score = _state.p2_score = 0;
            }

        /**
         * In non-deuce state the game is won if one player has reached eleven
         * or twenty one points depending on the game mode.
         */
        } else {
            int win = _state.mode == game_mode::to_11 ? 11 : 21;
            if (get_p1_score() >= win) {
                _state.p1_games_won++;
                _state.p1_score = _state.p2_score = 0;
            } else if (get_p2_score() >= win) {
                _state.p2_games_won++;
                _state.p1_score = _state.p2_score = 0;
            }
        }
    }
};

#endif /* __TABLE_TENNIS_HPP__ */
