//
// Created by catac_07fjpav on 11/4/2026.
//
#include "GameUI.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>

// Layout and colors
static const unsigned kWidth = 1100;
static const unsigned kHeight = 720;
static const int kPlayMinPopularity = 60;

static const sf::Color kBackground(18, 18, 26);
static const sf::Color kPanel(32, 32, 44);
static const sf::Color kText(236, 236, 242);
static const sf::Color kMuted(150, 150, 170);
static const sf::Color kAccent(124, 92, 255);
static const sf::Color kMatch(46, 160, 92);

// Formats a number with a fixed number of decimals.
static std::string fmt(double value, int decimals) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", decimals, value);
    return buf;
}

// Appends one Unicode character to a UTF-8 string.
static void appendUtf8(std::string& s, char32_t c) {
    if (c < 0x80) {
        s += static_cast<char>(c);
    } else if (c < 0x800) {
        s += static_cast<char>(0xC0 | (c >> 6));
        s += static_cast<char>(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
        s += static_cast<char>(0xE0 | (c >> 12));
        s += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (c & 0x3F));
    } else {
        s += static_cast<char>(0xF0 | (c >> 18));
        s += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        s += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (c & 0x3F));
    }
}

// Removes the last character (not just the last byte) of a UTF-8 string.
static void popUtf8(std::string& s) {
    while (!s.empty() && (static_cast<unsigned char>(s.back()) & 0xC0) == 0x80) s.pop_back();
    if (!s.empty()) s.pop_back();
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
GameUI::GameUI(const std::vector<Song>& songs, const std::vector<CategorySets>& sets)
    : songs_(songs),
      sets_(sets),
      window_(sf::VideoMode({kWidth, kHeight}), "Songdle", sf::Style::Titlebar | sf::Style::Close),
      rng_(std::random_device{}()),
      game_(songs, sets) {
    window_.setFramerateLimit(60);

    // The one font the whole UI uses. Any .ttf works; it lives at assets/font.ttf.
    if (!font_.openFromFile("assets/font.ttf"))
        throw std::runtime_error("Could not load assets/font.ttf");
}


void GameUI::run() {
    while (window_.isOpen()) {
        while (const std::optional event = window_.pollEvent()) handleEvent(*event);

        window_.clear(kBackground);
        clickables_.clear();
        switch (screen_) {
            case Screen::Menu:      drawMenu(); break;
            case Screen::Play:      drawPlay(); break;
            case Screen::Results:   drawResults(); break;
            case Screen::Benchmark: drawBenchmark(); break;
        }
        window_.display();
    }
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------
void GameUI::handleEvent(const sf::Event& event) {
    if (event.is<sf::Event::Closed>()) {
        window_.close();
    } else if (const auto* press = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (press->button != sf::Mouse::Button::Left) return;
        sf::Vector2f p = window_.mapPixelToCoords(press->position);
        auto areas = clickables_;  // copy: a click can change screens
        for (auto it = areas.rbegin(); it != areas.rend(); ++it) {  // topmost first
            if (it->first.contains(p)) {
                it->second();
                break;
            }
        }
    } else if (const auto* typed = event.getIf<sf::Event::TextEntered>()) {
        onText(typed->unicode);
    } else if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        onKey(key->code);
    }
}

void GameUI::onText(char32_t c) {
    const bool backspace = (c == 8);
    if (!backspace && (c < 32 || c == 127)) return;  // ignore other control keys

    if (screen_ == Screen::Play) {
        if (backspace) popUtf8(input_);
        else if (input_.size() < 80) appendUtf8(input_, c);
        refreshSuggestions();
    }   else if (screen_ == Screen::Benchmark) {
        if (backspace) popUtf8(benchGamesInput_);
        else if (c >= '0' && c <= '9' && benchGamesInput_.size() < 5) appendUtf8(benchGamesInput_, c);
    }
}

void GameUI::onKey(sf::Keyboard::Key key) {
    using K = sf::Keyboard::Key;
    if (screen_ == Screen::Play) {
        const int n = static_cast<int>(suggestions_.size());
        if (key == K::Down && n > 0) selected_ = (selected_ + 1) % n;
        else if (key == K::Up && n > 0) selected_ = (selected_ + n - 1) % n;
        else if (key == K::Enter && n > 0) guessSong(suggestions_[selected_]);
        else if (key == K::Escape) {
            input_.clear();
            suggestions_.clear();
        }
    } else if (screen_ == Screen::Results && key == K::Enter) {
        startGame();
    } else if (screen_ == Screen::Benchmark && key == K::Enter) {
        startBenchmark();
    }
}

// ---------------------------------------------------------------------------
// Actions
// ---------------------------------------------------------------------------
void GameUI::startGame() {
    game_.startRandom(rng_, kPlayMinPopularity);
    input_.clear();
    suggestions_.clear();
    selected_ = 0;
    screen_ = Screen::Play;
}

void GameUI::guessSong(int songId) {
    if (game_.isOver() || game_.hasGuessed(songId)) return;
    game_.guess(songId);
    input_.clear();
    suggestions_.clear();
    selected_ = 0;
    if (game_.isOver()) screen_ = Screen::Results;
}

void GameUI::refreshSuggestions() {
    suggestions_.clear();
    selected_ = 0;
    if (input_.size() < 2) return;
    for (int id : game_.findByTitle(input_, 20)) {
        if (songs_[id].popularity >= kPlayMinPopularity && !game_.hasGuessed(id))
            suggestions_.push_back(id);
        if (suggestions_.size() == 6)
            break;
    }
}

void GameUI::startBenchmark() {
    int games = std::atoi(benchGamesInput_.c_str());
    if (games < 1) games = 1;
    benchGamesInput_ = std::to_string(games);

    // Show a message first: the window pauses until the benchmark finishes.
    window_.clear(kBackground);
    label("Running benchmark... the window will pause until it finishes.",
          sf::FloatRect({0.f, 0.f}, {static_cast<float>(kWidth), static_cast<float>(kHeight)}), 22,
          kText, Align::Center);
    window_.display();

    BenchmarkConfig cfg;
    cfg.games = games;
    report_ = Benchmark::run(songs_, sets_, cfg);
    haveReport_ = true;
}

// ---------------------------------------------------------------------------
// Drawing helpers
// ---------------------------------------------------------------------------
sf::String GameUI::toSf(const std::string& utf8) const {
    return sf::String::fromUtf8(utf8.begin(), utf8.end());
}

float GameUI::textWidth(const sf::String& s, unsigned size, bool bold) const {
    sf::Text t(font_, s, size);
    if (bold) t.setStyle(sf::Text::Bold);
    return t.getLocalBounds().size.x;
}

// Shortens text with "..." so it fits in maxWidth pixels.
sf::String GameUI::fitText(const std::string& utf8, float maxWidth, unsigned size, bool bold) const {
    sf::String s = toSf(utf8);
    if (textWidth(s, size, bold) <= maxWidth) return s;
    while (!s.isEmpty() && textWidth(s + "...", size, bold) > maxWidth) s.erase(s.getSize() - 1);
    return s + "...";
}

// Draws text vertically centered in `box`, aligned left, center or right.
void GameUI::label(const sf::String& s, sf::FloatRect r, unsigned size, sf::Color color,
                   Align align, bool bold, float padding) {
    sf::Text t(font_, s, size);
    if (bold) t.setStyle(sf::Text::Bold);
    t.setFillColor(color);

    // Center on the height of a capital letter so every line sits at the same level.
    const sf::FloatRect cap = font_.getGlyph(U'H', size, bold).bounds;
    const float y = r.position.y + r.size.y / 2.f - size - cap.position.y - cap.size.y / 2.f;

    const float w = t.getLocalBounds().size.x;
    float x = r.position.x + padding;
    if (align == Align::Center) x = r.position.x + (r.size.x - w) / 2.f;
    if (align == Align::Right) x = r.position.x + r.size.x - padding - w;

    t.setPosition({std::round(x), std::round(y)});
    window_.draw(t);
}

void GameUI::box(sf::FloatRect r, sf::Color fill) {
    sf::RectangleShape shape(r.size);
    shape.setPosition(r.position);
    shape.setFillColor(fill);
    window_.draw(shape);
}

void GameUI::button(const std::string& text, sf::FloatRect r, std::function<void()> onClick, bool primary) {
    box(r, primary ? kAccent : kPanel);    label(toSf(text), r, 18, kText, Align::Center, true);
    clickables_.push_back({r, std::move(onClick)});
}

void GameUI::arrow(sf::Vector2f c, bool up, sf::Color color, float size) {
    sf::ConvexShape tri(3);
    if (up) {
        tri.setPoint(0, {c.x, c.y - size});
        tri.setPoint(1, {c.x + size, c.y + size * 0.6f});
        tri.setPoint(2, {c.x - size, c.y + size * 0.6f});
    } else {
        tri.setPoint(0, {c.x, c.y + size});
        tri.setPoint(1, {c.x + size, c.y - size * 0.6f});
        tri.setPoint(2, {c.x - size, c.y - size * 0.6f});
    }
    tri.setFillColor(color);
    window_.draw(tri);
}

// One square of clue: green when it matches; otherwise an arrow shows
// which way the mystery song's value is (up = higher, down = lower).
void GameUI::clueCell(sf::FloatRect r, const std::string& text, bool match, int direction, bool showArrow) {
    box(r, match ? kMatch : kPanel);
    const float arrowSpace = (showArrow && !match) ? 26.f : 0.f;
    sf::FloatRect textArea({r.position.x, r.position.y}, {r.size.x - arrowSpace, r.size.y});
    label(fitText(text, textArea.size.x - 24.f, 17), textArea, 17, kText, Align::Left);
    if (showArrow && !match) {
        arrow({r.position.x + r.size.x - 20.f, r.position.y + r.size.y / 2.f},
              direction == 1, kText);
    }
}

// ---------------------------------------------------------------------------
// Menu screen
// ---------------------------------------------------------------------------
void GameUI::drawMenu() {
    const float W = static_cast<float>(kWidth);
    label("SONGDLE", sf::FloatRect({0.f, 150.f}, {W, 90.f}), 76, kAccent, Align::Center, true);
    label("Guess the mystery song in 8 tries.", sf::FloatRect({0.f, 245.f}, {W, 30.f}), 22, kText,
          Align::Center);

    std::string structures;
    for (int i = 0; i < static_cast<int>(sets_.size()); ++i)
        structures += (i ? " vs " : "") + sets_[i].name;
    label(toSf(std::to_string(songs_.size()) + " songs loaded   |   " + structures),
          sf::FloatRect({0.f, 280.f}, {W, 30.f}), 16, kMuted, Align::Center);

    const float bx = (W - 320.f) / 2.f;
      button("Play", sf::FloatRect({bx, 350.f}, {320.f, 56.f}), [this] { startGame(); }, true);    button("Run benchmark", sf::FloatRect({bx, 420.f}, {320.f, 56.f}),
           [this] { screen_ = Screen::Benchmark; });
    button("Quit", sf::FloatRect({bx, 490.f}, {320.f, 56.f}), [this] { window_.close(); });

    label("After each guess you learn if the artist matches and whether the year, tempo and",
          sf::FloatRect({0.f, 600.f}, {W, 24.f}), 15, kMuted, Align::Center);
    label("popularity are higher or lower. Both trees filter the songs that still fit every clue.",
          sf::FloatRect({0.f, 624.f}, {W, 24.f}), 15, kMuted, Align::Center);
}

// ---------------------------------------------------------------------------
// Play screen
// ---------------------------------------------------------------------------
void GameUI::drawPlay() {
    // Header
    label("SONGDLE", sf::FloatRect({40.f, 20.f}, {300.f, 44.f}), 30, kAccent, Align::Left, true, 0.f);
    const int guessNumber = std::min(game_.guessesUsed() + 1, game_.maxGuesses());
    label(toSf("Guess " + std::to_string(guessNumber) + " of " + std::to_string(game_.maxGuesses())),
          sf::FloatRect({600.f, 20.f}, {330.f, 44.f}), 22, kText, Align::Right, true, 0.f);
    button("Give up", sf::FloatRect({950.f, 22.f}, {110.f, 40.f}), [this] { screen_ = Screen::Results; });

    // Search box
    const sf::FloatRect search({40.f, 84.f}, {1020.f, 50.f});
    box(search, kPanel);
    const bool caretOn = static_cast<int>(clock_.getElapsedTime().asSeconds() * 2.f) % 2 == 0;
    if (input_.empty()) {
        label("Type a song title...", search, 19, kMuted);
    } else {
        sf::String shown = toSf(input_);
        label(shown, search, 19, kText);
        if (caretOn) {
            const float caretX = search.position.x + 14.f + textWidth(shown, 19) + 3.f;
            box(sf::FloatRect({caretX, search.position.y + 13.f}, {2.f, 24.f}), kText);
        }
    }

    // Board
    struct Column { const char* title; float x; float w; };
    const Column cols[5] = {
        {"Song", 40.f, 330.f}, {"Artist", 378.f, 230.f}, {"Year", 616.f, 140.f},
        {"Tempo (BPM)", 764.f, 140.f}, {"Popularity", 912.f, 148.f},
    };
    for (const Column& c : cols)
        label(c.title, sf::FloatRect({c.x, 148.f}, {c.w, 24.f}), 14, kMuted, Align::Left, false, 4.f);

    const float rowH = 44.f, rowGap = 6.f, top = 176.f;
    const auto& history = game_.history();
    for (int row = 0; row < game_.maxGuesses(); ++row) {
        const float y = top + row * (rowH + rowGap);
        if (row >= static_cast<int>(history.size())) {
            for (const Column& c : cols)
                box(sf::FloatRect({c.x, y}, {c.w, rowH}), kPanel);
            continue;
        }
        const GuessResult& f = history[row];
        const Song& s = game_.song(f.songId);
        box(sf::FloatRect({cols[0].x, y}, {cols[0].w, rowH}), f.correct ? kMatch : kPanel);
        label(fitText(s.name, cols[0].w - 28.f, 17), sf::FloatRect({cols[0].x, y}, {cols[0].w, rowH}),
              17, kText);
        clueCell(sf::FloatRect({cols[1].x, y}, {cols[1].w, rowH}), s.artist,
                 f.artistMatch, 0, false);
        clueCell(sf::FloatRect({cols[2].x, y}, {cols[2].w, rowH}), std::to_string(s.year),
                 f.year == 0, f.year, true);
        clueCell(sf::FloatRect({cols[3].x, y}, {cols[3].w, rowH}), fmt(s.tempo, 0),
                 f.tempo == 0, f.tempo, true);
        clueCell(sf::FloatRect({cols[4].x, y}, {cols[4].w, rowH}), std::to_string(s.popularity),
                 f.popularity == 0, f.popularity, true);
    }

    // Bottom panel: matches left and filter time per structure
    const sf::FloatRect panel({40.f, 590.f}, {1020.f, 110.f});
    box(panel, kPanel);
    if (history.empty()) {
        label("Start typing a title, pick a song with the mouse or Up/Down + Enter.", sf::FloatRect({40.f, 590.f}, {1020.f, 55.f}), 17, kMuted, Align::Center);
        label("Green = match.   Arrow = the mystery song's value is higher or lower.", sf::FloatRect({40.f, 640.f}, {1020.f, 40.f}), 17, kMuted, Align::Center);
    } else {
        const GuessResult& last = history.back();
        label("Songs that still fit", sf::FloatRect({60.f, 602.f}, {260.f, 26.f}), 15, kMuted, Align::Left, false, 0.f);
        label(toSf(std::to_string(last.matches)), sf::FloatRect({60.f, 628.f}, {260.f, 56.f}), 44, kText, Align::Left, true, 0.f);

        double slowest = 1.0;
        for (const FilterTiming& t : last.timings) slowest = std::max(slowest, t.treeMicros);
        float y = 606.f;
        for (const FilterTiming& t : last.timings) {
            label(toSf(t.structure), sf::FloatRect({340.f, y}, {180.f, 26.f}), 16, kText, Align::Left, false, 0.f);
            const float barW = static_cast<float>(380.0 * t.treeMicros / slowest);
            box(sf::FloatRect({530.f, y + 5.f}, {380.f, 16.f}), kBackground);
            box(sf::FloatRect({530.f, y + 5.f}, {std::max(barW, 2.f), 16.f}), kAccent);
            label(toSf(fmt(t.treeMicros, 1) + " us"), sf::FloatRect({920.f, y}, {130.f, 26.f}), 16,
                  kText, Align::Right, false, 0.f);
            y += 34.f;
        }
        if (!last.structuresAgree)
            label("WARNING: the trees returned different songs!", sf::FloatRect({340.f, 670.f}, {700.f, 24.f}), 15, kText, Align::Left, true, 0.f);
    }

    // Suggestions dropdown (drawn last so it sits on top of the board)
    if (input_.size() >= 2) {
        const float itemH = 42.f;
        float y = search.position.y + search.size.y + 4.f;
        // Background-colored backing so the list stands apart from the board under it.
        const int rows = std::max(1, static_cast<int>(suggestions_.size()));
        box(sf::FloatRect({search.position.x - 6.f, y - 4.f}, {search.size.x + 12.f, rows * itemH + 6.f}),
            kBackground);
        if (suggestions_.empty()) {
            sf::FloatRect item({search.position.x, y}, {search.size.x, itemH - 2.f});
            box(item, kPanel);
            label(toSf("No match. Only songs with popularity " + std::to_string(kPlayMinPopularity) +
                       "+ can be guessed."),
                  item, 17, kMuted);
        }
        for (int i = 0; i < static_cast<int>(suggestions_.size()); ++i) {
            const int id = suggestions_[i];
            const Song& s = songs_[id];
            sf::FloatRect item({search.position.x, y}, {search.size.x, itemH - 2.f});
            const bool selected = (i == selected_);
            box(item, selected ? kAccent : kPanel);
            label(fitText(s.name, 520.f, 18, true), item, 18, kText, Align::Left, true);
            label(fitText(s.artist + "  (" + std::to_string(s.year) + ")", 440.f, 16), sf::FloatRect({item.position.x + 560.f, y}, {460.f, itemH - 2.f}), 16, selected ? kText : kMuted);
            clickables_.push_back({item, [this, id] { guessSong(id); }});
            y += itemH;
        }
    }
}

// Results screen
void GameUI::drawResults() {
    const float W = static_cast<float>(kWidth);
    const Song& m = game_.song(game_.mysteryId());

    if (game_.hasWon())
        label(toSf("Solved! in " + std::to_string(game_.guessesUsed()) + "/" + std::to_string(game_.maxGuesses())), sf::FloatRect({0.f, 180.f}, {W, 60.f}), 48, kMatch, Align::Center, true);
    else
        label("Not solved", sf::FloatRect({0.f, 180.f}, {W, 60.f}), 48, kText, Align::Center, true);
    label("The mystery song was", sf::FloatRect({0.f, 260.f}, {W, 26.f}), 16, kMuted, Align::Center);
    label(fitText(m.name + "  -  " + m.artist, W - 120.f, 28, true), sf::FloatRect({0.f, 290.f}, {W, 40.f}), 28, kText, Align::Center, true);
    label(toSf(std::to_string(m.year) + "   |   " + fmt(m.tempo, 0) + " BPM   |   popularity " +std::to_string(m.popularity)), sf::FloatRect({0.f, 334.f}, {W, 28.f}), 18, kMuted, Align::Center);
    button("Play again", sf::FloatRect({W / 2.f - 230.f, 420.f}, {220.f, 52.f}), [this] { startGame(); }, true);
    button("Menu", sf::FloatRect({W / 2.f + 10.f, 420.f}, {220.f, 52.f}), [this] { screen_ = Screen::Menu; });
}

// Benchmark screen
void GameUI::drawBenchmark() {
    label("Benchmark mode", sf::FloatRect({40.f, 24.f}, {600.f, 50.f}), 34, kAccent, Align::Left, true, 0.f);
    label("Plays games automatically and compares how fast each structure filters the songs.", sf::FloatRect({40.f, 74.f}, {1000.f, 26.f}), 17, kMuted, Align::Left, false, 0.f);

    // Games input + run
    label("Games to simulate", sf::FloatRect({40.f, 126.f}, {200.f, 50.f}), 18, kText, Align::Left, false, 0.f);
    const sf::FloatRect field({230.f, 126.f}, {150.f, 50.f});
    box(field, kPanel);
    label(toSf(benchGamesInput_), field, 20, kText);
    if (static_cast<int>(clock_.getElapsedTime().asSeconds() * 2.f) % 2 == 0) {
        const float caretX = field.position.x + 14.f + textWidth(toSf(benchGamesInput_), 20) + 3.f;
        box(sf::FloatRect({caretX, field.position.y + 13.f}, {2.f, 24.f}), kText);
    }
    button("Run", sf::FloatRect({400.f, 126.f}, {160.f, 50.f}), [this] { startBenchmark(); }, true);
    // Results table
    if (haveReport_) {
        const BenchmarkReport& r = report_;
        const int filteredGuesses = r.totalGuesses - r.solved;
        label(toSf("Solved " + std::to_string(r.solved) + " of " + std::to_string(r.games) + " games   |   avg " + fmt(r.games ? double(r.totalGuesses) / r.games : 0.0, 1) + " guesses   |   " + fmt(r.wallSeconds, 1) + " s total"), sf::FloatRect({40.f, 256.f}, {1020.f, 28.f}), 17, kText, Align::Left, false, 0.f);
        label(toSf("Results matched: " + std::to_string(filteredGuesses - r.mismatches) + " / " + std::to_string(filteredGuesses) + " filtered guesses"), sf::FloatRect({40.f, 284.f}, {1020.f, 28.f}), 17, r.mismatches ? kText : kMatch, Align::Left, true, 0.f);

        const int n = static_cast<int>(r.structures.size());
        struct Row { const char* name; std::vector<double> values; int decimals; const char* unit; };
        std::vector<Row> rows = {
            {"Build time", {}, 0, " ms"},
            {"Memory", {}, 1, " MB"},
            {"Avg search per guess", {}, 1, " us"},
            {"Slowest search", {}, 1, " us"},
        };
        for (const StructureStats& s : r.structures) {
            rows[0].values.push_back(s.buildMillis);
            rows[1].values.push_back(s.memoryBytes / (1024.0 * 1024.0));
            rows[2].values.push_back(s.avgTreeMicros());
            rows[3].values.push_back(s.slowestTreeMicros);
        }

        const float tx = 40.f, tw = 1020.f, nameW = 300.f;
        const float colW = (tw - nameW) / std::max(n, 1);
        for (int i = 0; i < n; ++i)
            label(toSf(r.structures[i].name), sf::FloatRect({tx + nameW + i * colW, 326.f}, {colW, 26.f}), 16, kMuted, Align::Right, false, 16.f);
        float y = 356.f;
        for (const Row& row : rows) {
            box(sf::FloatRect({tx, y}, {tw, 42.f}), kPanel);
            label(row.name, sf::FloatRect({tx, y}, {nameW, 42.f}), 17, kText);
            const double best = *std::min_element(row.values.begin(), row.values.end());
            for (int i = 0; i < n; ++i) {
                const bool isBest = n > 1 && row.values[i] == best;
                label(toSf(fmt(row.values[i], row.decimals) + row.unit),
                      sf::FloatRect({tx + nameW + i * colW, y}, {colW, 42.f}), 18,
                      isBest ? kMatch : kText, Align::Right, isBest, 16.f);
            }
            y += 48.f;
        }
        label("Green = lower (better). Per-guess timings saved to benchmark_results.csv",
              sf::FloatRect({40.f, y + 2.f}, {1020.f, 24.f}), 14, kMuted, Align::Left, false, 0.f);
    } else {
        label("Type how many games to play and press Run. 200 games takes a minute or two on the full dataset.",
              sf::FloatRect({40.f, 220.f}, {1020.f, 26.f}), 16, kMuted, Align::Left, false, 0.f);
    }
    button("Menu", sf::FloatRect({40.f, 648.f}, {160.f, 48.f}), [this] { screen_ = Screen::Menu; });
}