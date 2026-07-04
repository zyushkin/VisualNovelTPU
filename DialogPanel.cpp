#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <string>
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;

sf::String fromUtf8(const std::string& str) {
    if (str.empty()) return sf::String();
    return sf::String::fromUtf8(str.begin(), str.end());
}

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
        const sf::Color& textColor = sf::Color::White)
        : m_rect(size), m_text(text, font, 20),
        m_normalColor(normalColor), m_hoverColor(hoverColor)
    {
        m_rect.setPosition(position);
        m_rect.setFillColor(normalColor);
        m_rect.setOutlineThickness(2);
        m_rect.setOutlineColor(sf::Color(150, 150, 200));

        sf::FloatRect textRect = m_text.getLocalBounds();
        m_text.setOrigin(textRect.left + textRect.width / 2.f,
            textRect.top + textRect.height / 2.f);
        m_text.setPosition(position.x + size.x / 2.f,
            position.y + size.y / 2.f);
        m_text.setFillColor(textColor);
    }

    void setPosition(const sf::Vector2f& pos) {
        m_rect.setPosition(pos);
        sf::FloatRect textRect = m_text.getLocalBounds();
        m_text.setPosition(pos.x + m_rect.getSize().x / 2.f,
            pos.y + m_rect.getSize().y / 2.f);
    }

    void setSize(const sf::Vector2f& size) {
        m_rect.setSize(size);
        sf::FloatRect textRect = m_text.getLocalBounds();
        m_text.setPosition(m_rect.getPosition().x + size.x / 2.f,
            m_rect.getPosition().y + size.y / 2.f);
    }

    void setText(const sf::String& text) {
        m_text.setString(text);
        sf::FloatRect textRect = m_text.getLocalBounds();
        m_text.setOrigin(textRect.left + textRect.width / 2.f,
            textRect.top + textRect.height / 2.f);
    }

    bool contains(const sf::Vector2f& point) const {
        return m_rect.getGlobalBounds().contains(point);
    }

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
        sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        bool mouseOver = contains(mouse);

        if (event.type == sf::Event::MouseMoved) {
            m_rect.setFillColor(mouseOver ? m_hoverColor : m_normalColor);
        }

        if (event.type == sf::Event::MouseButtonPressed &&
            event.mouseButton.button == sf::Mouse::Left &&
            mouseOver && m_callback) {
            m_callback();
        }
    }

    void setCallback(std::function<void()> callback) { m_callback = callback; }

    void draw(sf::RenderWindow& window) const {
        window.draw(m_rect);
        window.draw(m_text);
    }

private:
    sf::RectangleShape m_rect;
    sf::Text m_text;
    sf::Color m_normalColor;
    sf::Color m_hoverColor;
    std::function<void()> m_callback;
};

class DialogueManager {
public:
    DialogueManager(const sf::Font& font, sf::RenderWindow& window)
        : m_font(font), m_window(window),
        m_dialogueVisible(true), m_showHistory(false),
        m_currentNodeId(0), m_charIndex(0), m_textSpeed(0.04f),
        m_accumulatedTime(0.f), m_panelOpacity(0.f),
        m_isPanelVisible(false), m_hasOptions(false)
    {
        setupUI();
        resize(window.getSize());
    }

    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Не удалось открыть файл " << filename << std::endl;
            return false;
        }
        json data;
        try {
            file >> data;
        }
        catch (const std::exception& e) {
            std::cerr << "Ошибка парсинга JSON: " << e.what() << std::endl;
            return false;
        }

        if (!data.contains("nodes")) {
            std::cerr << "В JSON отсутствует поле 'nodes'" << std::endl;
            return false;
        }

        std::unordered_map<int, DialogueNode> nodes;
        for (auto& [key, nodeData] : data["nodes"].items()) {
            int id = std::stoi(key);
            DialogueNode node;
            node.id = id;
            node.speaker = nodeData.value("speaker", "");
            node.text = nodeData.value("text", "");
            node.nextNodeId = nodeData.value("next", -1);
            for (auto& opt : nodeData["options"]) {
                DialogueOption option;
                option.text = opt.value("text", "");
                option.nextNodeId = opt.value("next", -1);
                node.options.push_back(option);
            }
            nodes[id] = node;
        }
        loadDialogue(nodes);
        return true;
    }

    void loadDialogue(const std::unordered_map<int, DialogueNode>& nodes) {
        m_nodes = nodes;
        if (!m_nodes.empty()) {
            goToNode(0);
        }
    }

    void goToNode(int id) {
        if (m_nodes.find(id) == m_nodes.end()) return;

        if (!m_history.empty() && m_history.back() != m_currentNodeId) {
            m_history.push_back(m_currentNodeId);
        }
        else if (m_history.empty()) {
            m_history.push_back(m_currentNodeId);
        }

        m_currentNodeId = id;
        const auto& node = m_nodes.at(id);

        m_currentSpeaker = node.speaker;
        m_fullText = node.text;
        m_displayedText = "";
        m_charIndex = 0;
        m_accumulatedTime = 0.f;

        m_isPanelVisible = true;
        m_panelOpacity = 0.f;

        m_optionButtons.clear();

        m_hasOptions = !node.options.empty();
        float spacing = 40.f;
        for (size_t i = 0; i < node.options.size(); ++i) {
            const auto& opt = node.options[i];
            auto btn = std::make_unique<Button>(m_font, fromUtf8(opt.text),
                sf::Vector2f(0, 0), sf::Vector2f(300, 35));
            btn->setCallback([this, opt]() {
                if (opt.nextNodeId == -1) {
                    endDialogue();
                }
                else {
                    goToNode(opt.nextNodeId);
                }
                });
            m_optionButtons.push_back(std::move(btn));
        }

        resize(m_window.getSize());
    }

    void endDialogue() {
        m_dialogueVisible = false;
        std::cout << "Диалог завершён.\n";
    }

    void update(float deltaTime) {
        if (!m_dialogueVisible) return;

        if (m_isPanelVisible && m_panelOpacity < 220.f) {
            m_panelOpacity += 300.f * deltaTime;
            if (m_panelOpacity > 220.f) m_panelOpacity = 220.f;
            m_panel.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(m_panelOpacity)));
        }

        if (!m_fullText.empty() && m_charIndex < static_cast<int>(m_fullText.length())) {
            m_accumulatedTime += deltaTime;
            if (m_accumulatedTime >= m_textSpeed) {
                ++m_charIndex;
                if (m_charIndex > static_cast<int>(m_fullText.length()))
                    m_charIndex = static_cast<int>(m_fullText.length());
                m_displayedText = m_fullText.substr(0, static_cast<size_t>(m_charIndex));
                m_accumulatedTime = 0.f;
            }
        }
    }

    void handleEvent(const sf::Event& event) {
        if (m_showHistory) {
            if (event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mouse = m_window.mapPixelToCoords(
                    sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                if (m_historyCloseBtn->contains(mouse)) {
                    m_showHistory = false;
                }
            }
            return;
        }

        if (!m_dialogueVisible) {
            m_showBtn->handleEvent(event, m_window);
            return;
        }

        for (auto& [name, btn] : m_buttons) {
            btn->handleEvent(event, m_window);
        }

        for (auto& btn : m_optionButtons) {
            btn->handleEvent(event, m_window);
        }

        if (event.type == sf::Event::MouseButtonPressed &&
            event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mouse = m_window.mapPixelToCoords(
                sf::Vector2i(event.mouseButton.x, event.mouseButton.y));

            bool clickedOnButton = false;
            for (auto& [name, btn] : m_buttons) {
                if (btn->contains(mouse)) { clickedOnButton = true; break; }
            }
            if (!clickedOnButton) {
                for (auto& btn : m_optionButtons) {
                    if (btn->contains(mouse)) { clickedOnButton = true; break; }
                }
            }

            if (!clickedOnButton) {
                if (!m_fullText.empty() && m_charIndex < static_cast<int>(m_fullText.length())) {
                    m_charIndex = static_cast<int>(m_fullText.length());
                    m_displayedText = m_fullText;
                }
                else if (!m_hasOptions) {
                    const auto& node = m_nodes.at(m_currentNodeId);
                    if (node.nextNodeId != -1) {
                        goToNode(node.nextNodeId);
                    }
                    else {
                        endDialogue();
                    }
                }
            }
        }
    }

    void resize(const sf::Vector2u& newSize) {
        float w = static_cast<float>(newSize.x);
        float h = static_cast<float>(newSize.y);

        float panelW = w * 0.9f;
        float panelH = h * 0.3f;
        m_panel.setSize(sf::Vector2f(panelW, panelH));
        m_panel.setPosition(w * 0.05f, h - panelH - 10);
        m_panel.setOutlineColor(sf::Color(80, 80, 150));

        float btnSizeX = 80.f, btnSizeY = 30.f;
        float margin = 10.f;
        float x = w - margin - btnSizeX;
        float y = margin;
        if (m_buttons.find("hide") != m_buttons.end()) {
            m_buttons["hide"]->setPosition(sf::Vector2f(x, y));
            m_buttons["hide"]->setSize(sf::Vector2f(btnSizeX, btnSizeY));
            x -= btnSizeX + 5;
            m_buttons["history"]->setPosition(sf::Vector2f(x, y));
            m_buttons["history"]->setSize(sf::Vector2f(btnSizeX, btnSizeY));
            x -= btnSizeX + 5;
            m_buttons["skip"]->setPosition(sf::Vector2f(x, y));
            m_buttons["skip"]->setSize(sf::Vector2f(btnSizeX, btnSizeY));
        }

        m_showBtn->setPosition(sf::Vector2f(w - 100, 10));
        m_showBtn->setSize(sf::Vector2f(90, 30));

        m_historyCloseBtn->setPosition(sf::Vector2f(w - 100, 10));
        m_historyCloseBtn->setSize(sf::Vector2f(90, 30));

        float historyW = w * 0.7f;
        float historyH = h * 0.7f;
        m_historyPanel.setSize(sf::Vector2f(historyW, historyH));
        m_historyPanel.setPosition((w - historyW) / 2.f, (h - historyH) / 2.f);
        m_historyPanel.setFillColor(sf::Color(20, 20, 40, 230));
        m_historyPanel.setOutlineThickness(2);
        m_historyPanel.setOutlineColor(sf::Color(150, 150, 200));

        m_nameBoxSize = sf::Vector2f(200.f, 30.f);

        float optionStartY = h - panelH - 10 + 100;
        float optionX = w * 0.05f + 20;
        for (size_t i = 0; i < m_optionButtons.size(); ++i) {
            m_optionButtons[i]->setPosition(sf::Vector2f(optionX, optionStartY + i * 40.f));
            m_optionButtons[i]->setSize(sf::Vector2f(panelW * 0.8f, 35.f));
        }

        m_windowSize = newSize;
    }

    void draw() {
        if (m_showHistory) {
            drawHistory();
            return;
        }

        if (!m_dialogueVisible) {
            m_showBtn->draw(m_window);
            return;
        }

        m_window.draw(m_panel);

        if (!m_currentSpeaker.empty()) {
            sf::RectangleShape nameBox(m_nameBoxSize);
            nameBox.setPosition(m_panel.getPosition().x + 20, m_panel.getPosition().y + 10);
            nameBox.setFillColor(sf::Color(40, 40, 80, 200));
            nameBox.setOutlineThickness(1);
            nameBox.setOutlineColor(sf::Color(100, 100, 200));
            m_window.draw(nameBox);

            sf::Text nameText(fromUtf8(m_currentSpeaker), m_font, 18);
            nameText.setPosition(m_panel.getPosition().x + 30, m_panel.getPosition().y + 12);
            nameText.setFillColor(sf::Color(150, 200, 255));
            m_window.draw(nameText);
        }

        sf::String displayString = m_displayedText.empty() ? sf::String(" ") : fromUtf8(m_displayedText);
        sf::Text text(displayString, m_font, 22);
        text.setPosition(m_panel.getPosition().x + 20, m_panel.getPosition().y + 50);
        text.setFillColor(sf::Color::White);
        text.setLineSpacing(1.3f);
        m_window.draw(text);

        for (auto& btn : m_optionButtons) {
            btn->draw(m_window);
        }

        for (auto& [name, btn] : m_buttons) {
            btn->draw(m_window);
        }

        if (!m_hasOptions && !m_fullText.empty() && m_charIndex >= static_cast<int>(m_fullText.length())) {
            sf::Text hint(fromUtf8("Нажмите для продолжения"), m_font, 14);
            hint.setPosition(m_panel.getPosition().x + 20, m_panel.getPosition().y + m_panel.getSize().y - 30);
            hint.setFillColor(sf::Color(200, 200, 200, 150));
            m_window.draw(hint);
        }
    }

    void drawHistory() {
        m_window.draw(m_historyPanel);

        sf::Text title(L"История диалога", m_font, 24);
        title.setPosition(m_historyPanel.getPosition().x + 20, m_historyPanel.getPosition().y + 10);
        title.setFillColor(sf::Color::White);
        m_window.draw(title);

        m_historyCloseBtn->draw(m_window);

        float yOffset = 60;
        float xOffset = m_historyPanel.getPosition().x + 20;
        float maxHeight = m_historyPanel.getPosition().y + m_historyPanel.getSize().y - 20;

        for (int id : m_history) {
            if (m_nodes.find(id) != m_nodes.end()) {
                const auto& node = m_nodes.at(id);
                std::string lineStr = node.speaker + ": " + node.text;
                sf::Text line(fromUtf8(lineStr), m_font, 18);
                line.setPosition(xOffset, yOffset);
                line.setFillColor(sf::Color(200, 200, 220));

                sf::RectangleShape lineBg(sf::Vector2f(m_historyPanel.getSize().x - 40, 25));
                lineBg.setPosition(xOffset - 5, yOffset - 2);
                lineBg.setFillColor(sf::Color(0, 0, 0, 120));
                m_window.draw(lineBg);

                m_window.draw(line);
                yOffset += 30;
                if (yOffset > maxHeight) break;
            }
        }
    }

    void toggleDialogue() {
        m_dialogueVisible = !m_dialogueVisible;
        if (m_dialogueVisible) {
            m_isPanelVisible = true;
            m_panelOpacity = 0.f;
        }
    }

    void showHistoryWindow() {
        m_showHistory = true;
    }

    void closeHistoryWindow() {
        m_showHistory = false;
    }

    void skipDialogue() {
        const auto& node = m_nodes.at(m_currentNodeId);
        if (!node.options.empty()) {
            goToNode(node.options[0].nextNodeId);
        }
        else if (node.nextNodeId != -1) {
            goToNode(node.nextNodeId);
        }
        else {
            endDialogue();
        }
    }

private:
    void setupUI() {
        m_panel.setFillColor(sf::Color(0, 0, 0, 0));
        m_panel.setOutlineThickness(2);
        m_panel.setOutlineColor(sf::Color(80, 80, 150));

        m_buttons["hide"] = std::make_unique<Button>(m_font, L"Скрыть", sf::Vector2f(0, 0), sf::Vector2f(80, 30));
        m_buttons["history"] = std::make_unique<Button>(m_font, L"История", sf::Vector2f(0, 0), sf::Vector2f(80, 30));
        m_buttons["skip"] = std::make_unique<Button>(m_font, L"Пропуск", sf::Vector2f(0, 0), sf::Vector2f(80, 30));

        m_buttons["hide"]->setCallback([this]() { toggleDialogue(); });
        m_buttons["history"]->setCallback([this]() { showHistoryWindow(); });
        m_buttons["skip"]->setCallback([this]() { skipDialogue(); });

        m_showBtn = std::make_unique<Button>(m_font, L"Показать", sf::Vector2f(0, 0), sf::Vector2f(90, 30));
        m_showBtn->setCallback([this]() { toggleDialogue(); });

        m_historyCloseBtn = std::make_unique<Button>(m_font, L"Закрыть", sf::Vector2f(0, 0), sf::Vector2f(90, 30));
        m_historyCloseBtn->setCallback([this]() { closeHistoryWindow(); });

        m_historyPanel.setFillColor(sf::Color(20, 20, 40, 230));
        m_historyPanel.setOutlineThickness(2);
        m_historyPanel.setOutlineColor(sf::Color(150, 150, 200));
    }

private:
    sf::Font m_font;
    sf::RenderWindow& m_window;
    sf::Vector2u m_windowSize;

    bool m_dialogueVisible;
    bool m_showHistory;

    std::unordered_map<int, DialogueNode> m_nodes;
    int m_currentNodeId;
    std::string m_currentSpeaker;
    std::string m_fullText;
    std::string m_displayedText;
    int m_charIndex;
    float m_textSpeed;
    float m_accumulatedTime;
    bool m_hasOptions;
    bool m_isPanelVisible;
    float m_panelOpacity;

    sf::RectangleShape m_panel;
    sf::Vector2f m_nameBoxSize;

    std::vector<int> m_history;

    std::vector<std::unique_ptr<Button>> m_optionButtons;
    std::unordered_map<std::string, std::unique_ptr<Button>> m_buttons;
    std::unique_ptr<Button> m_showBtn;
    std::unique_ptr<Button> m_historyCloseBtn;
    sf::RectangleShape m_historyPanel;
};

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), L"Тест");
    window.setVerticalSyncEnabled(true);

    sf::Font font;
    if (!font.loadFromFile("Lora-VariableFont_wght.ttf")) {
        std::cerr << "Ошибка загрузки шрифта Lora-VariableFont_wght.ttf\n";
        return -1;
    }

    DialogueManager manager(font, window);
    if (!manager.loadFromFile("dialogue.json")) {
        std::cerr << "Не удалось загрузить диалог. Проверьте файл dialogue.json\n";
        return -1;
    }

    sf::Clock clock;
    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::Resized) {
                window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
                manager.resize({ event.size.width, event.size.height });
            }

            manager.handleEvent(event);
        }

        manager.update(deltaTime);

        window.clear(sf::Color(30, 30, 60));
        manager.draw();
        window.display();
    }

    return 0;
}