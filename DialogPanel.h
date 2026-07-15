#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <string>
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;

sf::String fromUtf8(const std::string& str);

struct DialogueOption {
    std::string text;
    int nextNodeId;
};

struct DialogueNode {
    int id = 0;
    std::string speaker;
    std::string text;
    int nextNodeId = -1;
    std::vector<DialogueOption> options;
};

class Button {
public:
    Button(const sf::Font& font, const sf::String& text,
        const sf::Vector2f& position, const sf::Vector2f& size,
        const sf::Color& normalColor = sf::Color(60, 60, 120, 220),
        const sf::Color& hoverColor = sf::Color(100, 100, 180, 240),
        const sf::Color& textColor = sf::Color::White);

    void setPosition(const sf::Vector2f& pos);
    void setSize(const sf::Vector2f& size);
    void setText(const sf::String& text);
    bool contains(const sf::Vector2f& point) const;
    bool handleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void setCallback(std::function<void()> callback);
    void draw(sf::RenderWindow& window) const;

private:
    sf::RectangleShape m_rect;
    sf::Text m_text;
    sf::Color m_normalColor;
    sf::Color m_hoverColor;
    std::function<void()> m_callback;
};

class DialogueManager {
public:
    DialogueManager(const sf::Font& font, sf::RenderWindow& window);

    bool loadFromFile(const std::string& filename);
    void loadDialogue(const std::unordered_map<int, DialogueNode>& nodes);
    void goToNode(int id);
    void endDialogue();

    void update(float deltaTime);
    void handleEvent(const sf::Event& event);
    void resize(const sf::Vector2u& newSize);
    void draw();

    void toggleDialogue();
    void showHistoryWindow();
    void closeHistoryWindow();
    void skipDialogue();
    void toggleSound();

private:
    void setupUI();
    void drawHistory();

    sf::Font m_font;
    sf::RenderWindow& m_window;
    sf::Vector2u m_windowSize;

    bool m_dialogueVisible;
    bool m_showHistory;

    std::unordered_map<int, DialogueNode> m_nodes;
    int m_currentNodeId;
    std::string m_currentSpeaker;
    sf::String m_fullString;
    sf::String m_displayedString;
    int m_charIndex;
    float m_textSpeed;
    float m_accumulatedTime;
    bool m_hasOptions;
    bool m_isPanelVisible;
    float m_panelOpacity;

    sf::SoundBuffer m_letterSoundBuffer;
    sf::Sound m_letterSound;
    bool m_soundEnabled;

    sf::RectangleShape m_panel;
    sf::Vector2f m_nameBoxSize;

    std::vector<int> m_history;

    std::vector<std::unique_ptr<Button>> m_optionButtons;
    std::unordered_map<std::string, std::unique_ptr<Button>> m_buttons;
    std::unique_ptr<Button> m_showBtn;
    std::unique_ptr<Button> m_historyCloseBtn;
    sf::RectangleShape m_historyPanel;
};