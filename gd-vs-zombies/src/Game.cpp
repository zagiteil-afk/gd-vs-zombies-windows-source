#include "Game.hpp"
#include <algorithm>
#include <cmath>

namespace gvz {
PlantSpec spec(PlantKind kind) {
    switch (kind) {
        case PlantKind::Shooter:   return {100, 140.f, 2.f, 1.35f};
        case PlantKind::Generator: return {50, 110.f, 3.f, 9.f};
        case PlantKind::Wall:      return {75, 950.f, 7.f, 0.f};
        case PlantKind::Ice:       return {150, 140.f, 4.f, 2.1f};
        case PlantKind::Bomb:      return {125, 200.f, 16.f, 1.f};
    }
    return {0, 0.f, 0.f, 0.f};
}
Game::Game(std::uint32_t seed, Config config)
  : m_config(config), m_rng(seed), m_energy(config.startingEnergy) {
    m_config.waves = std::max(1, m_config.waves);
}
float Game::cooldown(PlantKind kind) const {
    auto i = static_cast<unsigned>(kind);
    return i < m_cooldowns.size() ? m_cooldowns[i] : 0.f;
}
const Plant* Game::plantAt(int row, int col) const {
    for (const auto& p : m_plants) if (p.row == row && p.col == col && p.hp > 0.f) return &p;
    return nullptr;
}
PlaceResult Game::place(PlantKind kind, int row, int col) {
    if (m_phase != Phase::Playing || m_paused || row < 0 || row >= Rows || col < 0 || col >= Cols ||
        static_cast<unsigned>(kind) >= m_cooldowns.size()) return PlaceResult::Invalid;
    if (plantAt(row, col)) return PlaceResult::Occupied;
    auto s = spec(kind);
    if (m_energy < s.cost) return PlaceResult::TooExpensive;
    if (cooldown(kind) > 0.f) return PlaceResult::CoolingDown;
    m_energy -= s.cost;
    m_cooldowns[static_cast<unsigned>(kind)] = s.cooldown;
    float first = kind == PlantKind::Generator ? 6.f : (kind == PlantKind::Bomb ? 1.f : .25f);
    m_plants.push_back({m_nextId++, kind, row, col, s.hp, s.hp, first});
    return PlaceResult::Placed;
}
bool Game::shovel(int row, int col) {
    if (m_phase != Phase::Playing || m_paused) return false;
    auto it = std::find_if(m_plants.begin(), m_plants.end(), [=](const Plant& p) {
        return p.row == row && p.col == col;
    });
    if (it == m_plants.end()) return false;
    m_plants.erase(it); // Intentional: no energy refund, like the original shovel.
    return true;
}
std::uint32_t Game::spawnEnemy(EnemyKind kind, int row, float x) {
    if (row < 0 || row >= Rows || !std::isfinite(x) || m_phase != Phase::Playing) return 0;
    float hp = 100.f, speed = .20f, damage = 28.f;
    if (kind == EnemyKind::Armored) { hp = 280.f; speed = .14f; damage = 34.f; }
    if (kind == EnemyKind::Runner) { hp = 70.f; speed = .34f; damage = 22.f; }
    hp += std::max(0, m_wave - 1) * 8.f;
    auto id = m_nextId++;
    m_enemies.push_back({id, kind, row, x, hp, hp, speed, damage, 0.f});
    return id;
}
void Game::advance(float seconds) {
    if (m_paused || m_phase != Phase::Playing || !std::isfinite(seconds) || seconds <= 0.f) return;
    // Avoid a fatal jump when GD returns from a stalled frame or an inactive window.
    m_accumulator += std::min(seconds, .25f);
    while (m_accumulator >= Tick && m_phase == Phase::Playing) {
        m_accumulator -= Tick;
        step();
    }
}
bool Game::enemyAhead(const Plant& p) const {
    return std::any_of(m_enemies.begin(), m_enemies.end(), [&](const Enemy& e) {
        return e.row == p.row && e.hp > 0.f && e.x >= p.col + .05f && e.x <= 9.6f;
    });
}
void Game::removeDead() {
    for (const auto& e : m_enemies) if (e.hp <= 0.f) {
        ++m_kills;
        m_energy = std::min(9999, m_energy + 5);
        m_effects.push_back({e.row, e.x, .3f, false});
    }
    m_enemies.erase(std::remove_if(m_enemies.begin(), m_enemies.end(), [](const Enemy& e) {
        return e.hp <= 0.f;
    }), m_enemies.end());
    m_plants.erase(std::remove_if(m_plants.begin(), m_plants.end(), [](const Plant& p) {
        return p.hp <= 0.f;
    }), m_plants.end());
}
void Game::step() {
    for (auto& cd : m_cooldowns) cd = std::max(0.f, cd - Tick);
    for (auto& fx : m_effects) fx.remaining -= Tick;
    m_effects.erase(std::remove_if(m_effects.begin(), m_effects.end(), [](const Effect& fx) {
        return fx.remaining <= 0.f;
    }), m_effects.end());
    m_skyTimer -= Tick;
    if (m_skyTimer <= 0.f) { m_energy = std::min(9999, m_energy + 25); m_skyTimer += 7.f; }

    if (m_config.automaticSpawns) {
        if (m_toSpawn == 0 && m_enemies.empty()) {
            if (m_wave >= m_config.waves) { m_phase = Phase::Won; return; }
            m_waveWait -= Tick;
            if (m_waveWait <= 0.f) { ++m_wave; m_toSpawn = 6 + m_wave * 3; m_spawnTimer = .3f; }
        }
        if (m_toSpawn > 0) {
            m_spawnTimer -= Tick;
            if (m_spawnTimer <= 0.f) {
                int row = static_cast<int>(m_rng() % Rows);
                if (row == m_lastRow) row = (row + 1) % Rows;
                m_lastRow = row;
                unsigned roll = m_rng() % 10;
                auto kind = m_wave >= 3 && roll < 3 ? EnemyKind::Armored :
                            m_wave >= 2 && roll >= 7 ? EnemyKind::Runner : EnemyKind::Normal;
                spawnEnemy(kind, row);
                --m_toSpawn;
                m_spawnTimer = std::max(1.2f, 3.6f - m_wave * .35f);
                if (m_toSpawn == 0) m_waveWait = 9.f;
            }
        }
    }

    for (auto& p : m_plants) {
        if (p.hp <= 0.f || p.kind == PlantKind::Wall) continue;
        p.timer -= Tick;
        if (p.timer > 0.f) continue;
        if (p.kind == PlantKind::Generator) {
            m_energy = std::min(9999, m_energy + 25);
            m_effects.push_back({p.row, p.col + .5f, .3f, false});
            p.timer += spec(p.kind).period;
        } else if (p.kind == PlantKind::Bomb) {
            for (auto& e : m_enemies) {
                if (std::abs(e.row - p.row) <= 1 && std::abs(e.x - (p.col + .5f)) <= 1.6f) e.hp -= 500.f;
            }
            p.hp = 0.f;
            m_effects.push_back({p.row, p.col + .5f, .55f, true});
        } else if (enemyAhead(p)) {
            bool ice = p.kind == PlantKind::Ice;
            m_shots.push_back({m_nextId++, p.row, p.col + .78f, ice ? 20.f : 25.f, ice});
            p.timer = spec(p.kind).period;
        } else p.timer = 0.f;
    }
    removeDead();

    for (auto& shot : m_shots) {
        float next = shot.x + Tick * 6.5f;
        Enemy* target = nullptr;
        for (auto& e : m_enemies) {
            // Swept collision: hit the closest live enemy, regardless of vector order.
            if (e.hp <= 0.f || e.row != shot.row || e.x + .28f < shot.x || e.x - .28f > next) continue;
            if (!target || e.x < target->x) target = &e;
        }
        if (target) {
            target->hp -= shot.damage;
            if (shot.ice) target->slow = 3.5f;
            shot.x = 100.f;
        } else shot.x = next;
    }
    m_shots.erase(std::remove_if(m_shots.begin(), m_shots.end(), [](const Shot& s) {
        return s.x > 10.f;
    }), m_shots.end());
    removeDead();

    for (auto& e : m_enemies) {
        e.slow = std::max(0.f, e.slow - Tick);
        float next = e.x - e.speed * (e.slow > 0.f ? .45f : 1.f) * Tick;
        Plant* blocking = nullptr;
        for (auto& p : m_plants) {
            float stop = p.col + .98f;
            if (p.hp <= 0.f || p.row != e.row || e.x < p.col + .05f || next > stop) continue;
            if (!blocking || p.col > blocking->col) blocking = &p;
        }
        if (blocking) {
            blocking->hp -= e.damage * Tick;
        } else e.x = next;
        auto& mower = m_mowers[e.row];
        if (e.x < .05f && mower.available) {
            mower.available = false;
            mower.active = true;
            mower.x = -.65f;
        }
        if (e.x < -.75f && !mower.active) m_phase = Phase::Lost;
    }
    for (int row = 0; row < Rows; ++row) {
        auto& mower = m_mowers[row];
        if (!mower.active) continue;
        float next = mower.x + Tick * 12.f;
        for (auto& e : m_enemies) if (e.row == row && e.x >= mower.x - .4f && e.x <= next + .4f) e.hp = 0.f;
        mower.x = next;
        if (mower.x > 10.f) mower.active = false;
    }
    removeDead();
}
}
