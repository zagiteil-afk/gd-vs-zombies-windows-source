#pragma once

#include <array>
#include <cstdint>
#include <random>
#include <vector>

namespace gvz {
constexpr int Rows = 5;
constexpr int Cols = 9;
constexpr float Tick = 1.f / 60.f;
enum class PlantKind { Shooter, Generator, Wall, Ice, Bomb };
enum class EnemyKind { Normal, Armored, Runner };
enum class Phase { Playing, Won, Lost };
enum class PlaceResult { Placed, Occupied, TooExpensive, CoolingDown, Invalid };
struct PlantSpec { int cost; float hp; float cooldown; float period; };
PlantSpec spec(PlantKind kind);
struct Plant {
    std::uint32_t id;
    PlantKind kind;
    int row, col;
    float hp, maxHp, timer;
};
struct Enemy {
    std::uint32_t id;
    EnemyKind kind;
    int row;
    float x, hp, maxHp, speed, damage, slow = 0.f;
};
struct Shot { std::uint32_t id; int row; float x, damage; bool ice; };
struct Mower { bool available = true, active = false; float x = -.65f; };
struct Effect { int row; float x, remaining; bool blast; };
struct Config { int waves = 5; bool automaticSpawns = true; int startingEnergy = 275; };

// Pure C++ model. Coordinates are measured in cells; row 0 is the top lane.
class Game {
public:
    explicit Game(std::uint32_t seed = 1, Config config = {});
    void advance(float seconds);
    PlaceResult place(PlantKind kind, int row, int col);
    bool shovel(int row, int col);
    void setPaused(bool paused) { m_paused = paused; }
    bool paused() const { return m_paused; }
    Phase phase() const { return m_phase; }
    int energy() const { return m_energy; }
    int wave() const { return m_wave; }
    int totalWaves() const { return m_config.waves; }
    int kills() const { return m_kills; }
    float nextWaveIn() const { return m_waveWait; }
    float cooldown(PlantKind kind) const;
    const auto& plants() const { return m_plants; }
    const auto& enemies() const { return m_enemies; }
    const auto& shots() const { return m_shots; }
    const auto& mowers() const { return m_mowers; }
    const auto& effects() const { return m_effects; }
    const Plant* plantAt(int row, int col) const;
    // Also useful for deterministic scenarios and balance tests.
    std::uint32_t spawnEnemy(EnemyKind kind, int row, float x = 9.4f);
private:
    void step();
    bool enemyAhead(const Plant& plant) const;
    void removeDead();
    Config m_config;
    std::mt19937 m_rng;
    std::uint32_t m_nextId = 1;
    Phase m_phase = Phase::Playing;
    bool m_paused = false;
    int m_energy, m_wave = 0, m_toSpawn = 0, m_kills = 0, m_lastRow = -1;
    float m_accumulator = 0.f, m_skyTimer = 6.f, m_waveWait = 15.f, m_spawnTimer = 0.f;
    std::array<float, 5> m_cooldowns{};
    std::array<Mower, Rows> m_mowers{};
    std::vector<Plant> m_plants;
    std::vector<Enemy> m_enemies;
    std::vector<Shot> m_shots;
    std::vector<Effect> m_effects;
};
}
