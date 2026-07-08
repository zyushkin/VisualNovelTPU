#include "game.h"

Game::Game()
{
    window.create(
        sf::VideoMode({1280, 720}),
        "Anime Girl at TPU"
    );

    window.setFramerateLimit(60);

    currentState = GameState::MainMenu;
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
        // Закрытие окна
        if (event->is<sf::Event::Closed>())
        {
            window.close();
        }

        // Нажатие клавиш
        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
        {
            if (currentState == GameState::MainMenu)
            {
                // Вверх
                if (keyPressed->code == sf::Keyboard::Key::Up)
                {
                    menu.moveUp();
                }

                // Вниз
                if (keyPressed->code == sf::Keyboard::Key::Down)
                {
                    menu.moveDown();
                }

                // Enter
                if (keyPressed->code == sf::Keyboard::Key::Return)
                {
                    switch (menu.getSelectedIndex())
                    {
                        case 0:
                            currentState = GameState::Playing;
                            break;

                        case 1:
                            currentState = GameState::Settings;
                            break;

                        case 2:
                            currentState = GameState::About;
                            break;

                        case 3:
                            window.close();
                            break;
                    }
                }
            }
        }
    }
}

void Game::update()
{
    // Пока пусто
}

void Game::render()
{
    switch (currentState)
    {
        case GameState::MainMenu:
            window.clear(sf::Color(45, 45, 65));
            menu.draw(window);
            break;

        case GameState::Playing:
            window.clear(sf::Color::Green);
            break;

        case GameState::Settings:
            window.clear(sf::Color::Blue);
            break;

        case GameState::About:
            window.clear(sf::Color::Magenta);
            break;

        case GameState::Exit:
            window.close();
            return;
    }

    window.display();
}