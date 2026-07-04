#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class Menu
{
private:
    sf::Font font;

    std::vector<std::string> items;

    int selectedIndex;

public:
    Menu();

    void draw(sf::RenderWindow& window);

    void moveUp();
    void moveDown();

    int getSelectedIndex() const;
};