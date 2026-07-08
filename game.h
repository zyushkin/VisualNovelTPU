#pragma once

#include <SFML/Graphics.hpp>

#include "menu.h"
#include "GameState.h"

class Game
{
private:
    sf::RenderWindow window;

    Menu menu;

    GameState currentState;

    void processEvents();
    void update();
    void render();

public:
    Game();

    void run();
};