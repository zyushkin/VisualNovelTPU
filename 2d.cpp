#include "2d.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>

TPUGameModule::TPUGameModule() {
    player.setRadius(14.f);
    player.setFillColor(sf::Color(52, 152, 219));
    player.setOutlineThickness(2);
    player.setOutlineColor(sf::Color::White);
    player.setOrigin(14.f, 14.f);
}

bool TPUGameModule::init(const std::string& fontPath) {
    std::vector<std::string> fontPaths = { fontPath, "arial.ttf", "assets/arial.ttf", "C:/Windows/Fonts/arial.ttf" };
    bool fontLoaded = false;
    for (auto& p : fontPaths) { if (font.loadFromFile(p)) { fontLoaded = true; break; } }
    if (!fontLoaded) return false;

    struct BldgDef { std::string name, folder; int floors; };
    std::vector<BldgDef> defs = {
        {"Dormitory 14", "dorm14", 5},
        {"Building 19 (IT)", "bldg19", 5},
        {"Building 3 (Chem)", "bldg3", 3}
    };

    for (auto& d : defs) {
        Building b; b.name = d.name; b.folder = d.folder; b.floorCount = d.floors;
        for (int i = 0; i < d.floors; ++i) {
            std::string path = "assets/maps/" + d.folder + "/floor" + std::to_string(i) + ".txt";
            b.floors.push_back(loadFloorFromFile(path, d.name, i));
        }
        buildings.push_back(b);
    }

    if (!buildings.empty() && !buildings[0].floors.empty()) {
        auto& firstFloor = buildings[0].floors[0];
        if (!firstFloor.exitPositions.empty()) {
            sf::Vector2i ePos = firstFloor.exitPositions[0];
            player.setPosition(ePos.x * TILE_SIZE + TILE_SIZE/2.f, ePos.y * TILE_SIZE + TILE_SIZE/2.f);
        } else {
            player.setPosition(TILE_SIZE * 2.5f, TILE_SIZE * 2.5f);
        }
    }

    return true;
}

FloorData TPUGameModule::loadFloorFromFile(const std::string& path, const std::string& bName, int floorIdx) {
    FloorData fd;
    std::ifstream file(path);
    std::string line;
    int y = 0;
    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            fd.map.push_back(line);
            for (int x = 0; x < (int)line.size(); ++x) {
                if (line[x] == 'D') fd.doors.push_back({x, y});
                if (line[x] == 'S') fd.stairsPositions.push_back({x, y});
                if (line[x] == 'E') fd.exitPositions.push_back({x, y});
                if (line[x] == 'N') {
                    NPC n; n.gridX = x; n.gridY = y;
                    n.name = "Student TPU"; 
                    n.id = "dialog_" + bName + "_" + std::to_string(floorIdx) + "_" + std::to_string(x);
                    fd.npcs.push_back(n);
                }
            }
            y++;
        }
    } else {
        fd.map = {"##########","# NO MAP #","##########"};
    }
    fd.height = fd.map.size();
    fd.width = (fd.height > 0) ? fd.map[0].size() : 0;
    return fd;
}

void TPUGameModule::handleEvent(sf::Event& event, const sf::RenderWindow& window) {
    if (event.type == sf::Event::KeyPressed) {
        if (isDialogueActive) {
            if (event.key.code == sf::Keyboard::Space || event.key.code == sf::Keyboard::E) isDialogueActive = false;
            return;
        }

        if (showFloorMenu || showBuildingMenu) {
            int max = showFloorMenu ? buildings[currentBuildingIdx].floorCount : buildings.size();
            if (event.key.code == sf::Keyboard::Up) selectedOption = (selectedOption - 1 + (max + 1)) % (max + 1);
            if (event.key.code == sf::Keyboard::Down) selectedOption = (selectedOption + 1) % (max + 1);
            
            if (event.key.code == sf::Keyboard::Enter) {
                if (selectedOption < max) {
                    sf::Vector2i spawnGridPos(2, 2);

                    if (showFloorMenu) {
                        currentFloorIdx = selectedOption;
                        auto& f = buildings[currentBuildingIdx].floors[currentFloorIdx];
                        if (!f.stairsPositions.empty()) spawnGridPos = f.stairsPositions[0];
                    } else {
                        currentBuildingIdx = selectedOption;
                        currentFloorIdx = 0;
                        auto& f = buildings[currentBuildingIdx].floors[currentFloorIdx];
                        if (!f.exitPositions.empty()) spawnGridPos = f.exitPositions[0];
                    }
                    
                    player.setPosition(spawnGridPos.x * TILE_SIZE + TILE_SIZE/2.f, spawnGridPos.y * TILE_SIZE + TILE_SIZE/2.f);
                }
                showFloorMenu = false; showBuildingMenu = false;
            }
        } else {
            if (event.key.code == sf::Keyboard::E) interact();
        }
    }
}

void TPUGameModule::interact() {
    auto& fd = buildings[currentBuildingIdx].floors[currentFloorIdx];
    sf::Vector2f pPos = player.getPosition();

    for (const auto& npc : fd.npcs) {
        if (std::hypot(pPos.x - (npc.gridX*TILE_SIZE + TILE_SIZE/2), pPos.y - (npc.gridY*TILE_SIZE + TILE_SIZE/2)) < INTERACT_RANGE) {
            startDialogue(npc);
            return;
        }
    }

    for (auto& d : fd.doors) {
        if (std::hypot(pPos.x - (d.gridX*TILE_SIZE + TILE_SIZE/2), pPos.y - (d.gridY*TILE_SIZE + TILE_SIZE/2)) < INTERACT_RANGE) {
            d.isOpen = !d.isOpen;
            return;
        }
    }

    int gx = pPos.x / TILE_SIZE; int gy = pPos.y / TILE_SIZE;
    if (gy >= 0 && gy < fd.height && gx >= 0 && gx < fd.width) {
        if (fd.map[gy][gx] == 'S') { showFloorMenu = true; selectedOption = currentFloorIdx; }
        if (fd.map[gy][gx] == 'E') { showBuildingMenu = true; selectedOption = currentBuildingIdx; }
    }
}

void TPUGameModule::startDialogue(const NPC& npc) {
    isDialogueActive = true;
    currentSpeakerName = npc.name;
    currentDialogueText = "Hello! Welcome to TPU.\nYou are in " + buildings[currentBuildingIdx].name + ".\nPress SPACE to exit.";
}

void TPUGameModule::resolveCollisions() {
    auto& fd = buildings[currentBuildingIdx].floors[currentFloorIdx];
    sf::Vector2f pos = player.getPosition();
    float r = player.getRadius();

    for (int y = 0; y < fd.height; ++y) {
        for (int x = 0; x < fd.width; ++x) {
            bool solid = (fd.map[y][x] == '#');
            if (fd.map[y][x] == 'D') {
                for (auto& d : fd.doors) if (d.gridX == x && d.gridY == y && d.openProgress < 0.5f) solid = true;
            }
            if (solid) {
                sf::FloatRect tr(x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE);
                sf::Vector2f closest(std::clamp(pos.x, tr.left, tr.left + tr.width), std::clamp(pos.y, tr.top, tr.top + tr.height));
                sf::Vector2f diff = pos - closest;
                float dist = std::sqrt(diff.x*diff.x + diff.y*diff.y);
                if (dist < r && dist > 0.001f) player.move(diff / dist * (r - dist));
            }
        }
    }
}

void TPUGameModule::update(float dt) {
    if (isDialogueActive || showFloorMenu || showBuildingMenu) return;

    auto& fd = buildings[currentBuildingIdx].floors[currentFloorIdx];
    for (auto& d : fd.doors) {
        float target = d.isOpen ? 1.0f : 0.0f;
        d.openProgress += (target - d.openProgress) * dt * 5.f;
    }

    sf::Vector2f m(0, 0);
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) m.y -= 1;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) m.y += 1;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) m.x -= 1;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) m.x += 1;

    if (m.x != 0 || m.y != 0) {
        player.move(m / std::sqrt(m.x*m.x + m.y*m.y) * PLAYER_SPEED * dt);
        resolveCollisions();
    }
}

void TPUGameModule::draw(sf::RenderWindow& window) {
    auto& fd = buildings[currentBuildingIdx].floors[currentFloorIdx];
    view.setCenter(player.getPosition());
    view.setSize(window.getSize().x, window.getSize().y);
    window.setView(view);

    sf::RectangleShape box(sf::Vector2f(TILE_SIZE, TILE_SIZE));
    for (int y = 0; y < fd.height; ++y) {
        for (int x = 0; x < fd.width; ++x) {
            box.setPosition(x*TILE_SIZE, y*TILE_SIZE);
            char t = fd.map[y][x];
            if (t == '#') box.setFillColor(sf::Color(44, 62, 80));
            else if (t == 'S') box.setFillColor(sf::Color(230, 126, 34)); 
            else if (t == 'E') box.setFillColor(sf::Color(192, 57, 43));  
            else box.setFillColor(sf::Color(30, 30, 30));
            window.draw(box);
        }
    }

    sf::CircleShape nSh(14.f);
    nSh.setFillColor(sf::Color(46, 204, 113));
    for (auto& n : fd.npcs) {
        nSh.setPosition(n.gridX*TILE_SIZE + 6, n.gridY*TILE_SIZE + 6);
        window.draw(nSh);
    }

    box.setFillColor(sf::Color(241, 196, 15));
    for (auto& d : fd.doors) {
        box.setSize(sf::Vector2f(TILE_SIZE * (1.0f - d.openProgress), TILE_SIZE));
        box.setPosition(d.gridX * TILE_SIZE, d.gridY * TILE_SIZE);
        window.draw(box);
    }

    window.draw(player);
    drawUI(window);
}

void TPUGameModule::drawUI(sf::RenderWindow& window) {
    window.setView(window.getDefaultView());
    sf::Vector2u wSize = window.getSize();

    sf::Text info(buildings[currentBuildingIdx].name + " - Floor " + std::to_string(currentFloorIdx+1), font, 20);
    info.setPosition(20, 20);
    window.draw(info);

    if (isDialogueActive) {
        sf::RectangleShape db(sf::Vector2f(wSize.x - 40, 140));
        db.setFillColor(sf::Color(0,0,0,230));
        db.setOutlineThickness(2);
        db.setPosition(20, wSize.y - 160);
        window.draw(db);

        sf::Text n(currentSpeakerName, font, 18);
        n.setFillColor(sf::Color::Green);
        n.setPosition(40, wSize.y - 150);
        window.draw(n);

        sf::Text t(currentDialogueText, font, 16);
        t.setPosition(40, wSize.y - 120);
        window.draw(t);
    }

    if (showFloorMenu || showBuildingMenu) {
        sf::RectangleShape mb(sf::Vector2f(300, 300));
        mb.setFillColor(sf::Color(0,0,0,240));
        mb.setPosition(wSize.x/2.f - 150, wSize.y/2.f - 150);
        window.draw(mb);

        sf::Text title(showFloorMenu ? "Select Floor:" : "Travel to:", font, 22);
        title.setPosition(mb.getPosition().x + 20, mb.getPosition().y + 20);
        window.draw(title);

        int max = showFloorMenu ? buildings[currentBuildingIdx].floorCount : buildings.size();
        for (int i = 0; i <= max; ++i) {
            std::string s = (i == max) ? "Cancel" : (showFloorMenu ? "Floor " + std::to_string(i+1) : buildings[i].name);
            sf::Text opt(s, font, 18);
            opt.setPosition(mb.getPosition().x + 40, mb.getPosition().y + 70 + i*30);
            opt.setFillColor(selectedOption == i ? sf::Color::Yellow : sf::Color::White);
            window.draw(opt);
        }
    }
}