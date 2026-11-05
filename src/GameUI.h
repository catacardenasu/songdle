//
// Created by catac_07fjpav on 11/4/2026.
//

#ifndef SONGDLE_GAMEUI_H
#define SONGDLE_GAMEUI_H

#endif //SONGDLE_GAMEUI_H
#pragma once
#include "Benchmark.h"
#include "Game.h"

#include <SFML/Graphics.hpp>

#include <functional>
#include <random>
#include <string>
#include <utility>
#include <vector>

// SFML window with four screens: Menu, Play, Results and Benchmark.
// Drawing is "immediate mode": every frame each screen draws itself and
// registers the areas you can click, so there are no button objects to manage.
class GameUI {
public:
    GameUI(const std::vector<Song>& songs, const std::vector<CategorySets>& sets);
    void run();  // opens the window and loops until it is closed

private:
    enum class Screen { Menu, Play, Results, Benchmark };
    enum class Align { Left, Center, Right };

    // input
    void handleEvent(const sf::Event& event);
    void onText(char32_t c);
    void onKey(sf::Keyboard::Key key);

    // screens
    void drawMenu();
    void drawPlay();
    void drawResults();
    void drawBenchmark();

    // actions
    void startGame();
    void guessSong(int songId);
    void refreshSuggestions();
    void startBenchmark();

    // drawing helper functions
    sf::String toSf(const std::string& utf8) const;
    float textWidth(const sf::String& s, unsigned size, bool bold = false) const;
    sf::String fitText(const std::string& utf8, float maxWidth, unsigned size, bool bold = false) const;
    void label(const sf::String& s, sf::FloatRect box, unsigned size, sf::Color color, Align align = Align::Left, bool bold = false, float padding = 14.f);
    void box(sf::FloatRect r, sf::Color fill);
    void button(const std::string& text, sf::FloatRect r, std::function<void()> onClick, bool primary = false);
    void arrow(sf::Vector2f center, bool up, sf::Color color, float size = 7.f);
    void clueCell(sf::FloatRect r, const std::string& text, bool match, int direction, bool showArrow);

    const std::vector<Song>& songs_;
    const std::vector<CategorySets>& sets_;
    sf::RenderWindow window_;
    sf::Font font_;
    sf::Clock clock_;
    std::mt19937 rng_;

    Screen screen_ = Screen::Menu;
    std::vector<std::pair<sf::FloatRect, std::function<void()>>> clickables_;

    // play
    Game game_;
    std::string input_;  // player input
    std::vector<int> suggestions_;
    int selected_ = 0;

    // benchmark
    std::string benchGamesInput_ = "200";
    bool haveReport_ = false;
    BenchmarkReport report_;
};