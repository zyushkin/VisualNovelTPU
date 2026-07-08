#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <optional>

class Menu
{
private:
    std::optional<sf::Font> font;

    std::vector<std::string> items;

    int selectedIndex = 0;

public:
    Menu();

    void draw(sf::RenderWindow& window);

    void moveUp();
    void moveDown();

    int getSelectedIndex() const;
};