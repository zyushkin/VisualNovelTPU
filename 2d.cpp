#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>


const int MAP_WIDTH = 21;
const int MAP_HEIGHT = 13;
const float TILE_SIZE = 36.0f;
const float PLAYER_SPEED = 200.0f;
const float INTERACT_RANGE = 60.0f;

const std::vector<std::string> MAP_DATA_FLOOR0 = {
    "#####################",
    "#.........#.........#",
    "#.........#.........#",
    "#....S....#....S....#",
    "###D########D########",
    "#...................#",
    "#...................#",
    "#...................#",
    "###D########D########",
    "#....S....#....S....#",
    "#.........#.........#",
    "#.........#.........#",
    "#####################"
};

const std::vector<std::string> MAP_DATA_FLOOR1 = {
    "#####################",
    "#.........#.........#",
    "#....S....#....S....#",
    "#.........#.........#",
    "###D########D########",
    "#...................#",
    "#...................#",
    "#...................#",
    "###D########D########",
    "#.........#.........#",
    "#....S....#....S....#",
    "#.........#.........#",
    "#####################"
};

const std::vector<std::string> MAP_DATA_FLOOR2 = {
    "#####################",
    "#....S....#....S....#",
    "#.........#.........#",
    "#.........#.........#",
    "###D########D########",
    "#...................#",
    "#...................#",
    "#...................#",
    "###D########D########",
    "#.........#.........#",
    "#.........#.........#",
    "#....S....#....S....#",
    "#####################"
};

struct Door {
    int gridX;
    int gridY;
    bool isOpen = false;
    float openProgress = 0.0f;
};

struct Staircase {
    int gridX;
    int gridY;
};

struct FloorData {
    std::vector<std::string> map;
    std::vector<Door> doors;
    std::vector<Staircase> stairs;
};

std::vector<FloorData> floors;
int currentFloor = 0;
bool showStairsDialog = false;
int selectedOption = 0;
sf::RectangleShape dialogBackground;
sf::Text dialogText;
sf::Text option1Text;
sf::Text option2Text;
sf::Text option3Text;
sf::Text option4Text;
sf::Font dialogFont;

void initializeFloors() {
    floors.resize(3);
    
    floors[0].map = MAP_DATA_FLOOR0;
    floors[1].map = MAP_DATA_FLOOR1;
    floors[2].map = MAP_DATA_FLOOR2;
    
    for (int f = 0; f < 3; f++) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                if (floors[f].map[y][x] == 'D') {
                    Door door;
                    door.gridX = x;
                    door.gridY = y;
                    floors[f].doors.push_back(door);
                }
                if (floors[f].map[y][x] == 'S') {
                    Staircase stairs;
                    stairs.gridX = x;
                    stairs.gridY = y;
                    floors[f].stairs.push_back(stairs);
                }
            }
        }
    }
}

void resolveCollisions(sf::CircleShape& player, float radius, const std::vector<Door>& doors, float dx, float dy, const std::vector<std::string>& mapData) {
    sf::Vector2f pos = player.getPosition();
    
    int startX = std::max(0, static_cast<int>((pos.x - radius) / TILE_SIZE));
    int endX = std::min(MAP_WIDTH - 1, static_cast<int>((pos.x + radius) / TILE_SIZE));
    int startY = std::max(0, static_cast<int>((pos.y - radius) / TILE_SIZE));
    int endY = std::min(MAP_HEIGHT - 1, static_cast<int>((pos.y + radius) / TILE_SIZE));

    for (int y = startY; y <= endY; ++y) {
        for (int x = startX; x <= endX; ++x) {
            char tile = mapData[y][x];
            bool isSolid = (tile == '#');

            if (tile == 'D') {
                for (const auto& door : doors) {
                    if (door.gridX == x && door.gridY == y && door.openProgress < 0.6f) {
                        isSolid = true;
                        break;
                    }
                }
            }

            if (isSolid) {
                float tileLeft = x * TILE_SIZE;
                float tileRight = (x + 1) * TILE_SIZE;
                float tileTop = y * TILE_SIZE;
                float tileBottom = (y + 1) * TILE_SIZE;

                float closestX = std::max(tileLeft, std::min(pos.x, tileRight));
                float closestY = std::max(tileTop, std::min(pos.y, tileBottom));

                float distX = pos.x - closestX;
                float distY = pos.y - closestY;
                float distance = std::sqrt(distX * distX + distY * distY);

                if (distance < radius) {
                    float overlap = radius - distance;
                    if (distance == 0.0f) continue;

                    if (dx != 0.0f) {
                        player.move((distX / distance) * overlap, 0.0f);
                    }
                    if (dy != 0.0f) {
                        player.move(0.0f, (distY / distance) * overlap);
                    }
                    pos = player.getPosition();
                }
            }
        }
    }
}

int main() {
    sf::RenderWindow window(sf::VideoMode(1920, 1080), "Multi-floor Building - Floor 1", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    initializeFloors();
    
    dialogFont.loadFromFile("arial.ttf");
    
    dialogBackground.setSize(sf::Vector2f(400, 250));
    dialogBackground.setFillColor(sf::Color(30, 30, 30, 230));
    dialogBackground.setOutlineColor(sf::Color::White);
    dialogBackground.setOutlineThickness(2);
    
    dialogText.setFont(dialogFont);
    dialogText.setCharacterSize(20);
    dialogText.setFillColor(sf::Color::White);
    
    option1Text.setFont(dialogFont);
    option1Text.setCharacterSize(18);
    
    option2Text.setFont(dialogFont);
    option2Text.setCharacterSize(18);
    
    option3Text.setFont(dialogFont);
    option3Text.setCharacterSize(18);
    
    option4Text.setFont(dialogFont);
    option4Text.setCharacterSize(18);

    sf::View view(sf::FloatRect(0.f, 0.f, 1920.f, 1080.f));

    float playerRadius = 12.0f;
    sf::CircleShape player(playerRadius);
    player.setFillColor(sf::Color(1, 1, 255));
    player.setOrigin(playerRadius, playerRadius);
    player.setPosition(2.0f * TILE_SIZE + TILE_SIZE / 2.0f, 5.0f * TILE_SIZE + TILE_SIZE / 2.0f);

    sf::RectangleShape wallShape(sf::Vector2f(TILE_SIZE, TILE_SIZE));
    wallShape.setFillColor(sf::Color(30, 41, 59));

    sf::RectangleShape floorShape(sf::Vector2f(TILE_SIZE, TILE_SIZE));
    floorShape.setFillColor(sf::Color(15, 23, 42));

    sf::RectangleShape doorShape(sf::Vector2f(TILE_SIZE - 4.0f, TILE_SIZE));
    doorShape.setFillColor(sf::Color(245, 158, 11));
    
    sf::RectangleShape stairsShape(sf::Vector2f(TILE_SIZE, TILE_SIZE));
    stairsShape.setFillColor(sf::Color(139, 69, 19));

    sf::Clock clock;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (showStairsDialog) {
                    if (event.key.code == sf::Keyboard::Up || event.key.code == sf::Keyboard::W) {
                        selectedOption = (selectedOption - 1 + 4) % 4;
                    } else if (event.key.code == sf::Keyboard::Down || event.key.code == sf::Keyboard::S) {
                        selectedOption = (selectedOption + 1) % 4;
                    } else if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::E) {
                        if (selectedOption == 0 && currentFloor != 0) {
                            currentFloor = 0;
                            player.setPosition(2.0f * TILE_SIZE + TILE_SIZE / 2.0f, 5.0f * TILE_SIZE + TILE_SIZE / 2.0f);
                            window.setTitle("Multi-floor Building - Floor 1");
                        } else if (selectedOption == 1 && currentFloor != 1) {
                            currentFloor = 1;
                            player.setPosition(2.0f * TILE_SIZE + TILE_SIZE / 2.0f, 5.0f * TILE_SIZE + TILE_SIZE / 2.0f);
                            window.setTitle("Multi-floor Building - Floor 2");
                        } else if (selectedOption == 2 && currentFloor != 2) {
                            currentFloor = 2;
                            player.setPosition(2.0f * TILE_SIZE + TILE_SIZE / 2.0f, 5.0f * TILE_SIZE + TILE_SIZE / 2.0f);
                            window.setTitle("Multi-floor Building - Floor 3");
                        }
                        showStairsDialog = false;
                        selectedOption = 0;
                    }
                    continue;
                }
                
                if (event.key.code == sf::Keyboard::E) {
                    sf::Vector2f playerPos = player.getPosition();

                    bool interactedWithDoor = false;
                    for (size_t i = 0; i < floors[currentFloor].doors.size(); ++i) {
                        sf::Vector2f doorCenter((floors[currentFloor].doors[i].gridX + 0.5f) * TILE_SIZE, 
                                               (floors[currentFloor].doors[i].gridY + 0.5f) * TILE_SIZE);
                        float dx = playerPos.x - doorCenter.x;
                        float dy = playerPos.y - doorCenter.y;
                        float dist = std::sqrt(dx * dx + dy * dy);

                        if (dist <= INTERACT_RANGE) {
                            floors[currentFloor].doors[i].isOpen = !floors[currentFloor].doors[i].isOpen;
                            interactedWithDoor = true;
                        }
                    }
                    
                    if (!interactedWithDoor) {
                        for (const auto& stairs : floors[currentFloor].stairs) {
                            sf::Vector2f stairsCenter((stairs.gridX + 0.5f) * TILE_SIZE, 
                                                     (stairs.gridY + 0.5f) * TILE_SIZE);
                            float dx = playerPos.x - stairsCenter.x;
                            float dy = playerPos.y - stairsCenter.y;
                            float dist = std::sqrt(dx * dx + dy * dy);

                            if (dist <= INTERACT_RANGE) {
                                showStairsDialog = true;
                                selectedOption = currentFloor;
                                break;
                            }
                        }
                    }
                }
            }
        }

        for (size_t i = 0; i < floors[currentFloor].doors.size(); ++i) {
            float target = floors[currentFloor].doors[i].isOpen ? 1.0f : 0.0f;
            if (floors[currentFloor].doors[i].openProgress != target) {
                float slideSpeed = 6.0f;
                if (floors[currentFloor].doors[i].isOpen) {
                    floors[currentFloor].doors[i].openProgress = std::min(1.0f, floors[currentFloor].doors[i].openProgress + dt * slideSpeed);
                } else {
                    floors[currentFloor].doors[i].openProgress = std::max(0.0f, floors[currentFloor].doors[i].openProgress - dt * slideSpeed);
                }
            }
        }

        if (!showStairsDialog) {
            sf::Vector2f movement(0.f, 0.f);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) movement.y -= 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) movement.y += 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) movement.x -= 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) movement.x += 1.f;

            float length = std::sqrt(movement.x * movement.x + movement.y * movement.y);
            if (length != 0.f) {
                movement /= length;
            }

            sf::Vector2f velocity = movement * PLAYER_SPEED * dt;

            if (velocity.x != 0.f) {
                player.move(velocity.x, 0.f);
                resolveCollisions(player, playerRadius, floors[currentFloor].doors, 1.f, 0.f, floors[currentFloor].map);
            }
            if (velocity.y != 0.f) {
                player.move(0.f, velocity.y);
                resolveCollisions(player, playerRadius, floors[currentFloor].doors, 0.f, 1.f, floors[currentFloor].map);
            }
        }

        view.setCenter(player.getPosition());
        window.setView(view);

        window.clear();

        for (int y = 0; y < MAP_HEIGHT; ++y) {
            for (int x = 0; x < MAP_WIDTH; ++x) {
                if (floors[currentFloor].map[y][x] == '#') {
                    wallShape.setPosition(x * TILE_SIZE, y * TILE_SIZE);
                    window.draw(wallShape);
                } else {
                    floorShape.setPosition(x * TILE_SIZE, y * TILE_SIZE);
                    window.draw(floorShape);
                }
                
                if (floors[currentFloor].map[y][x] == 'S') {
                    stairsShape.setPosition(x * TILE_SIZE, y * TILE_SIZE);
                    window.draw(stairsShape);
                }
            }
        }

        for (size_t i = 0; i < floors[currentFloor].doors.size(); ++i) {
            if (floors[currentFloor].doors[i].openProgress < 1.0f) {
                float currentWidth = (TILE_SIZE - 4.f) * (1.0f - floors[currentFloor].doors[i].openProgress);
                doorShape.setSize(sf::Vector2f(currentWidth, TILE_SIZE));
                doorShape.setPosition(floors[currentFloor].doors[i].gridX * TILE_SIZE + 2.f + (TILE_SIZE - 4.f - currentWidth) / 2.f, 
                                    floors[currentFloor].doors[i].gridY * TILE_SIZE);
                doorShape.setFillColor(floors[currentFloor].doors[i].isOpen ? sf::Color(34, 197, 94) : sf::Color(245, 158, 11));
                window.draw(doorShape);
            }
        }

        window.draw(player);
        
        if (showStairsDialog) {
            sf::Vector2f viewCenter = view.getCenter();
            
            dialogBackground.setPosition(viewCenter.x - 200, viewCenter.y - 125);
            window.draw(dialogBackground);
            
            dialogText.setString("Select Floor");
            dialogText.setPosition(viewCenter.x - 180, viewCenter.y - 105);
            window.draw(dialogText);
            
            option1Text.setString("Floor 1" + std::string(currentFloor == 0 ? " (current)" : ""));
            option1Text.setFillColor(selectedOption == 0 ? sf::Color::Yellow : sf::Color::White);
            option1Text.setPosition(viewCenter.x - 180, viewCenter.y - 65);
            window.draw(option1Text);
            
            option2Text.setString("Floor 2" + std::string(currentFloor == 1 ? " (current)" : ""));
            option2Text.setFillColor(selectedOption == 1 ? sf::Color::Yellow : sf::Color::White);
            option2Text.setPosition(viewCenter.x - 180, viewCenter.y - 35);
            window.draw(option2Text);
            
            option3Text.setString("Floor 3" + std::string(currentFloor == 2 ? " (current)" : ""));
            option3Text.setFillColor(selectedOption == 2 ? sf::Color::Yellow : sf::Color::White);
            option3Text.setPosition(viewCenter.x - 180, viewCenter.y - 5);
            window.draw(option3Text);
            
            option4Text.setString("Cancel");
            option4Text.setFillColor(selectedOption == 3 ? sf::Color::Yellow : sf::Color::White);
            option4Text.setPosition(viewCenter.x - 180, viewCenter.y + 25);
            window.draw(option4Text);
        }

        window.display();
    }

    return 0;
}