#include "game.h"

Game::Game()
{
    window.create(
        sf::VideoMode({1280, 720}),
        "Anime Girl at TPU"
    );

    window.setFramerateLimit(60);
}

void Game::run()
{
    while (window.isOpen())
    {
        processEvents();
        update();
        render();
    }
}

void Game::processEvents()
{
    while (const std::optional event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            window.close();
        }
    }
}

void Game::update()
{
    // Пока ничего 
}

void Game::render()
{
    window.clear(sf::Color(45, 45, 65));

    menu.draw(window);

    window.display();
}