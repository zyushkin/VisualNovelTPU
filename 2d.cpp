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


const std::vector<std::string> MAP_DATA = {
    "#####################",
    "#.........#.........#",
    "#.........#.........#",
    "#.........#.........#",
    "###D########D########",
    "#...................#",
    "#...................#",
    "#...................#",
    "###D########D########",
    "#.........#.........#",
    "#.........#.........#",
    "#.........#.........#",
    "#####################"
};


struct Door {
    int gridX;
    int gridY;
    bool isOpen = false;
    float openProgress = 0.0f;
};

void resolveCollisions(sf::CircleShape& player, float radius, std::vector<Door>& doors, float dx, float dy) {
    sf::Vector2f pos = player.getPosition();
    
    int startX = std::max(0, static_cast<int>((pos.x - radius) / TILE_SIZE));
    int endX = std::min(MAP_WIDTH - 1, static_cast<int>((pos.x + radius) / TILE_SIZE));
    int startY = std::max(0, static_cast<int>((pos.y - radius) / TILE_SIZE));
    int endY = std::min(MAP_HEIGHT - 1, static_cast<int>((pos.y + radius) / TILE_SIZE));

    for (int y = startY; y <= endY; ++y) {
        for (int x = startX; x <= endX; ++x) {
            char tile = MAP_DATA[y][x];
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
                    if (distance == 0.0f) continue; // Защита от деления на 0

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
    sf::RenderWindow window(sf::VideoMode(1920, 1080), "a", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    sf::View view(sf::FloatRect(0.f, 0.f, 1920.f, 1080.f));

    float playerRadius = 12.0f;
    sf::CircleShape player(playerRadius);
    player.setFillColor(sf::Color(1, 1, 255));
    player.setOrigin(playerRadius, playerRadius);
    player.setPosition(2.0f * TILE_SIZE + TILE_SIZE / 2.0f, 5.0f * TILE_SIZE + TILE_SIZE / 2.0f);

    std::vector<Door> doors;
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            if (MAP_DATA[y][x] == 'D') {
                Door door;
                door.gridX = x;
                door.gridY = y;
                doors.push_back(door);
            }
        }
    }

    sf::RectangleShape wallShape(sf::Vector2f(TILE_SIZE, TILE_SIZE));
    wallShape.setFillColor(sf::Color(30, 41, 59));

    sf::RectangleShape floorShape(sf::Vector2f(TILE_SIZE, TILE_SIZE));
    floorShape.setFillColor(sf::Color(15, 23, 42));

    sf::RectangleShape doorShape(sf::Vector2f(TILE_SIZE - 4.0f, TILE_SIZE));
    doorShape.setFillColor(sf::Color(245, 158, 11));

    sf::Clock clock;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::E) {
                    sf::Vector2f playerPos = player.getPosition();

                    for (size_t i = 0; i < doors.size(); ++i) {
                        sf::Vector2f doorCenter((doors[i].gridX + 0.5f) * TILE_SIZE, (doors[i].gridY + 0.5f) * TILE_SIZE);
                        float dx = playerPos.x - doorCenter.x;
                        float dy = playerPos.y - doorCenter.y;
                        float dist = std::sqrt(dx * dx + dy * dy);

                        if (dist <= INTERACT_RANGE) {
                            doors[i].isOpen = !doors[i].isOpen;
                        }
                    }
                }
            }
        }

        for (size_t i = 0; i < doors.size(); ++i) {
            float target = doors[i].isOpen ? 1.0f : 0.0f;
            if (doors[i].openProgress != target) {
                float slideSpeed = 6.0f;
                if (doors[i].isOpen) {
                    doors[i].openProgress = std::min(1.0f, doors[i].openProgress + dt * slideSpeed);
                } else {
                    doors[i].openProgress = std::max(0.0f, doors[i].openProgress - dt * slideSpeed);
                }
            }
        }

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
            resolveCollisions(player, playerRadius, doors, 1.f, 0.f);
        }
        if (velocity.y != 0.f) {
            player.move(0.f, velocity.y);
            resolveCollisions(player, playerRadius, doors, 0.f, 1.f);
        }

        view.setCenter(player.getPosition());
        window.setView(view);

        window.clear();

        for (int y = 0; y < MAP_HEIGHT; ++y) {
            for (int x = 0; x < MAP_WIDTH; ++x) {
                if (MAP_DATA[y][x] == '#') {
                    wallShape.setPosition(x * TILE_SIZE, y * TILE_SIZE);
                    window.draw(wallShape);
                } else {
                    floorShape.setPosition(x * TILE_SIZE, y * TILE_SIZE);
                    window.draw(floorShape);
                }
            }
        }

        for (size_t i = 0; i < doors.size(); ++i) {
            if (doors[i].openProgress < 1.0f) {
                float currentWidth = (TILE_SIZE - 4.f) * (1.0f - doors[i].openProgress);
                doorShape.setSize(sf::Vector2f(currentWidth, TILE_SIZE));
                doorShape.setPosition(doors[i].gridX * TILE_SIZE + 2.f + (TILE_SIZE - 4.f - currentWidth) / 2.f, doors[i].gridY * TILE_SIZE);
                doorShape.setFillColor(doors[i].isOpen ? sf::Color(34, 197, 94) : sf::Color(245, 158, 11));
                window.draw(doorShape);
            }
        }

        window.draw(player);

        window.display();
    }

    return 0;
}