#ifndef TPU_2D_H
#define TPU_2D_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

struct Door {
    int gridX, gridY;
    bool isOpen = false;
    float openProgress = 0.0f;
};

struct NPC {
    int gridX, gridY;
    std::string id;
    std::string name;
};

struct FloorData {
    std::vector<std::string> map;
    std::vector<Door> doors;
    std::vector<NPC> npcs;
    std::vector<sf::Vector2i> stairsPositions;
    std::vector<sf::Vector2i> exitPositions;
    int width = 0;
    int height = 0;
};

struct Building {
    std::string name;
    std::string folder;
    int floorCount;
    std::vector<FloorData> floors;
};

class TPUGameModule {
public:
    TPUGameModule();
    bool init(const std::string& fontPath);
    void handleEvent(sf::Event& event, const sf::RenderWindow& window);
    void update(float dt);
    void draw(sf::RenderWindow& window);

private:
    FloorData loadFloorFromFile(const std::string& path, const std::string& bName, int floorIdx);
    void resolveCollisions();
    void interact();
    void drawUI(sf::RenderWindow& window);
    void startDialogue(const NPC& npc);

    bool isDialogueActive = false;
    std::string currentDialogueText = "";
    std::string currentSpeakerName = "";

    std::vector<Building> buildings;
    int currentBuildingIdx = 0;
    int currentFloorIdx = 0;

    sf::CircleShape player;
    sf::View view;
    sf::Font font;

    bool showFloorMenu = false;
    bool showBuildingMenu = false;
    int selectedOption = 0;

    const float TILE_SIZE = 40.0f;
    const float PLAYER_SPEED = 220.0f;
    const float INTERACT_RANGE = 70.0f;
};

#endif