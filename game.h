#pragma once

#include <SFML/Graphics.hpp>
#include "menu.h"

class Game
{
private:
    sf::RenderWindow window;

    Menu menu;

    void processEvents();
    void update();
    void render();

public:
    Game();

    void run();
};