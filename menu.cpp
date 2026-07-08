#include "menu.h"

#include <iostream>

Menu::Menu()
{
    try
    {
        font.emplace("assets/fonts/arial.ttf");
    }
    catch (const std::exception&)
    {
        std::cout << "Failed to load font!" << std::endl;
    }

    items =
    {
        "Start Game",
        "Settings",
        "About",
        "Exit"
    };
}

void Menu::draw(sf::RenderWindow& window)
{
    if (!font)
        return;

    sf::Text title(*font, "Anime Girl at TPU", 48);
    title.setPosition({300.f, 80.f});
    title.setFillColor(sf::Color::Cyan);

    window.draw(title);

    float y = 220.f;

    for (std::size_t i = 0; i < items.size(); ++i)
    {
        sf::Text text(*font, items[i], 36);

        text.setPosition({470.f, y});

        if (static_cast<int>(i) == selectedIndex)
            text.setFillColor(sf::Color::Yellow);
        else
            text.setFillColor(sf::Color::White);

        window.draw(text);

        y += 70.f;
    }
}

void Menu::moveUp()
{
    if (selectedIndex > 0)
        selectedIndex--;
}

void Menu::moveDown()
{
    if (selectedIndex < static_cast<int>(items.size()) - 1)
        selectedIndex++;
}

int Menu::getSelectedIndex() const
{
    return selectedIndex;
}