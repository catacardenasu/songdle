#include "DataLoader.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <unordered_map>

static std::string trim(const std::string& s) {
    int b = 0, e = static_cast<int>(s.size());
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

static std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// Reads one CSV record. A quoted field can contain a newline, so keep
// reading lines while the number of quote characters is odd.
static bool readRecord(std::istream& in, std::string& record) {
    std::string line;
    if (!std::getline(in, line)) return false;
    record = line;
    while (std::count(record.begin(), record.end(), '"') % 2 != 0 && std::getline(in, line)) {
        record += '\n';
        record += line;
    }
    if (!record.empty() && record.back() == '\r') record.pop_back();
    return true;
}

static bool toLong(const std::string& s, long& out) {
    if (s.empty()) return false;
    char* end = nullptr;
    out = std::strtol(s.c_str(), &end, 10);
    return end != s.c_str();
}

static bool toDouble(const std::string& s, double& out) {
    if (s.empty()) return false;
    char* end = nullptr;
    out = std::strtod(s.c_str(), &end);
    return end != s.c_str();
}

  // namespace

std::vector<std::string> DataLoader::parseCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string cur;
    bool inQuotes = false;
    for (int i = 0; i < static_cast<int>(line.size()); ++i) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < static_cast<int>(line.size()) && line[i + 1] == '"') {  // "" -> "
                    cur += '"';
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                cur += c;
            }
        } else if (c == '"') {
            inQuotes = true;
        } else if (c == ',') {
            fields.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    fields.push_back(cur);
    return fields;
}

std::vector<std::string> DataLoader::parseArtistList(const std::string& raw) {
    std::string s = trim(raw);
    std::vector<std::string> out;
    if (s.size() < 2 || s.front() != '[' || s.back() != ']') {  // cleaned file: plain name
        if (!s.empty()) out.push_back(s);
        return out;
    }
    // Python list text, e.g. ['Uli', "Guns N' Roses"]
    const int end = static_cast<int>(s.size()) - 1;  // index of ']'
    int i = 1;
    while (i < end) {
        char q = s[i];
        if (q == '\'' || q == '"') {
            std::string name;
            ++i;
            while (i < end && s[i] != q) {
                if (s[i] == '\\' && i + 1 < end) ++i;  // \' escape
                name += s[i];
                ++i;
            }
            ++i;  // skip closing quote
            name = trim(name);
            if (!name.empty()) out.push_back(name);
        } else {
            ++i;  // commas and spaces between items
        }
    }
    return out;
}

std::vector<Song> DataLoader::load(const std::string& path, LoadStats* stats, int limit) {
    auto start = std::chrono::steady_clock::now();

    std::ifstream in(path);
    if (!in) throw std::runtime_error("Could not open " + path);

    std::string record;
    if (!readRecord(in, record)) throw std::runtime_error(path + " is empty");

    std::vector<std::string> header = parseCsvLine(record);
    if (!header.empty() && header[0].rfind("\xEF\xBB\xBF", 0) == 0) header[0].erase(0, 3);  // BOM

    std::unordered_map<std::string, int> col;
    for (int i = 0; i < static_cast<int>(header.size()); ++i) col[lower(trim(header[i]))] = i;

    auto findCol = [&](std::initializer_list<const char*> names) {
        for (const char* n : names) {
            auto it = col.find(n);
            if (it != col.end()) return it->second;
        }
        return -1;
    };
    const int cName = findCol({"name", "title"});
    const int cArtist = findCol({"artists", "artist"});
    const int cYear = findCol({"year", "release_year", "release_date"});
    const int cPop = findCol({"popularity"});
    const int cTempo = findCol({"tempo"});

    if (cName < 0 || cArtist < 0 || cYear < 0 || cPop < 0 || cTempo < 0) {
        std::string found;
        for (const auto& h : header) found += "'" + h + "' ";
        throw std::runtime_error(
            "Missing a required column (name, artists, release_date/year, popularity, tempo). "
            "Columns found: " + found);
    }
    const int maxCol = std::max({cName, cArtist, cYear, cPop, cTempo});

    LoadStats local;
    std::vector<Song> songs;
    songs.reserve(600000);

    while (readRecord(in, record)) {
        if (record.empty()) continue;
        ++local.rowsRead;

        std::vector<std::string> f = parseCsvLine(record);
        if (static_cast<int>(f.size()) <= maxCol) { ++local.skipped; continue; }

        Song s;
        s.name = trim(f[cName]);
        std::vector<std::string> artists = parseArtistList(f[cArtist]);
        if (s.name.empty() || artists.empty()) { ++local.skipped; continue; }

        s.primaryArtist = artists[0];
        for (int i = 0; i < static_cast<int>(artists.size()); ++i) {
            if (i) s.artists += ", ";
            s.artists += artists[i];
        }

        long year = 0, pop = 0;
        double tempo = 0.0;
        const std::string yearText = trim(f[cYear]).substr(0, 4);  // "1998-04-12" -> "1998"
        if (!toLong(yearText, year) || year < 1900 || year > 2100 ||
            !toLong(trim(f[cPop]), pop) || pop < 0 || pop > 100 ||
            !toDouble(trim(f[cTempo]), tempo) || tempo <= 0.0) {  // tempo 0 = unknown
            ++local.skipped;
            continue;
        }
        s.year = static_cast<int>(year);
        s.popularity = static_cast<int>(pop);
        s.tempo = tempo;

        songs.push_back(std::move(s));
        if (limit > 0 && static_cast<int>(songs.size()) >= limit) break;
    }

    local.loaded = static_cast<int>(songs.size());
    local.time = std::chrono::duration<double, std::milli>(
                       std::chrono::steady_clock::now() - start).count();
    if (stats) *stats = local;
    return songs;
}