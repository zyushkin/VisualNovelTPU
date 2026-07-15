#include "DialogPanel.h"

sf::String fromUtf8(const std::string& str) {
    if (str.empty()) return sf::String();
    return sf::String::fromUtf8(str.begin(), str.end());
}

Button::Button(const sf::Font& font, const sf::String& text,
    const sf::Vector2f& position, const sf::Vector2f& size,
    const sf::Color& normalColor,
    const sf::Color& hoverColor,
    const sf::Color& textColor)
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

void Button::setPosition(const sf::Vector2f& pos) {
    m_rect.setPosition(pos);
    sf::FloatRect textRect = m_text.getLocalBounds();
    m_text.setPosition(pos.x + m_rect.getSize().x / 2.f,
        pos.y + m_rect.getSize().y / 2.f);
}

void Button::setSize(const sf::Vector2f& size) {
    m_rect.setSize(size);
    sf::FloatRect textRect = m_text.getLocalBounds();
    m_text.setPosition(m_rect.getPosition().x + size.x / 2.f,
        m_rect.getPosition().y + size.y / 2.f);
}

void Button::setText(const sf::String& text) {
    m_text.setString(text);
    sf::FloatRect textRect = m_text.getLocalBounds();
    m_text.setOrigin(textRect.left + textRect.width / 2.f,
        textRect.top + textRect.height / 2.f);
}

bool Button::contains(const sf::Vector2f& point) const {
    return m_rect.getGlobalBounds().contains(point);
}

bool Button::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    bool mouseOver = contains(mouse);

    if (event.type == sf::Event::MouseMoved) {
        m_rect.setFillColor(mouseOver ? m_hoverColor : m_normalColor);
        return false;
    }

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left &&
        mouseOver && m_callback) {
        m_callback();
        return true;
    }
    return false;
}

void Button::setCallback(std::function<void()> callback) {
    m_callback = callback;
}

void Button::draw(sf::RenderWindow& window) const {
    window.draw(m_rect);
    window.draw(m_text);
}

DialogueManager::DialogueManager(const sf::Font& font, sf::RenderWindow& window)
    : m_font(font), m_window(window),
    m_dialogueVisible(true), m_showHistory(false),
    m_currentNodeId(0), m_charIndex(0), m_textSpeed(0.04f),
    m_accumulatedTime(0.f), m_panelOpacity(0.f),
    m_isPanelVisible(false), m_hasOptions(false),
    m_soundEnabled(true)
{
    if (!m_letterSoundBuffer.loadFromFile("click.wav")) {
        std::cerr << "Предупреждение: не удалось загрузить звук 'click.wav'\n";
    }
    else {
        m_letterSound.setBuffer(m_letterSoundBuffer);
        m_letterSound.setVolume(100.f); // громкость
    }

    setupUI();
    resize(window.getSize());
}

bool DialogueManager::loadFromFile(const std::string& filename) {
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
        try {
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
        catch (const std::exception& e) {
            std::cerr << "Ошибка в узле " << key << ": " << e.what() << std::endl;
            return false;
        }
    }
    loadDialogue(nodes);
    return true;
}

void DialogueManager::loadDialogue(const std::unordered_map<int, DialogueNode>& nodes) {
    m_nodes = nodes;
    if (!m_nodes.empty()) {
        goToNode(0);
    }
}

void DialogueManager::goToNode(int id) {
    if (m_nodes.find(id) == m_nodes.end()) {
        std::cerr << "Ошибка: узел " << id << " не существует!" << std::endl;
        return;
    }

    if (!m_history.empty() && m_history.back() != m_currentNodeId) {
        m_history.push_back(m_currentNodeId);
    }
    else if (m_history.empty()) {
        m_history.push_back(m_currentNodeId);
    }

    m_currentNodeId = id;
    const auto& node = m_nodes.at(id);

    m_currentSpeaker = node.speaker;
    m_fullString = fromUtf8(node.text);
    m_displayedString = sf::String();
    m_charIndex = 0;
    m_accumulatedTime = 0.f;

    m_isPanelVisible = true;
    m_panelOpacity = 0.f;

    m_optionButtons.clear();

    m_hasOptions = !node.options.empty();
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

void DialogueManager::endDialogue() {
    m_dialogueVisible = false;
    std::cout << "Диалог завершён.\n";
}

void DialogueManager::update(float deltaTime) {
    if (!m_dialogueVisible) return;

    if (m_isPanelVisible && m_panelOpacity < 220.f) {
        m_panelOpacity += 300.f * deltaTime;
        if (m_panelOpacity > 220.f) m_panelOpacity = 220.f;
        m_panel.setFillColor(sf::Color(0, 0, 0, static_cast<sf::Uint8>(m_panelOpacity)));
    }

    if (m_charIndex < m_fullString.getSize()) {
        m_accumulatedTime += deltaTime;
        if (m_accumulatedTime >= m_textSpeed) {
            m_displayedString += m_fullString[m_charIndex];
            ++m_charIndex;
            m_accumulatedTime = 0.f;

            if (m_soundEnabled && m_letterSound.getBuffer() != nullptr) {
                m_letterSound.play();
            }
        }
    }
}

void DialogueManager::handleEvent(const sf::Event& event) {
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
        if (btn->handleEvent(event, m_window)) {
            return;
        }
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
            if (m_charIndex < m_fullString.getSize()) {
                m_charIndex = m_fullString.getSize();
                m_displayedString = m_fullString;
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

void DialogueManager::resize(const sf::Vector2u& newSize) {
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

    float optionStartY = h - panelH - 10 + 120;
    float optionX = w * 0.05f + 20;
    float optionStep = 45.f;
    for (size_t i = 0; i < m_optionButtons.size(); ++i) {
        m_optionButtons[i]->setPosition(sf::Vector2f(optionX, optionStartY + i * optionStep));
        m_optionButtons[i]->setSize(sf::Vector2f(panelW * 0.8f, 35.f));
    }

    m_windowSize = newSize;
}

void DialogueManager::draw() {
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

    sf::String displayString = m_displayedString;
    if (displayString.isEmpty()) {
        displayString = sf::String(" ");
    }
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

    if (m_hasOptions && !m_fullString.isEmpty()) {
        sf::Text hint(fromUtf8("Выберите вариант"), m_font, 14);
        hint.setPosition(m_panel.getPosition().x + 20, m_panel.getPosition().y + m_panel.getSize().y - 30);
        hint.setFillColor(sf::Color(200, 200, 200, 150));
        m_window.draw(hint);
    }
    else if (!m_hasOptions && !m_fullString.isEmpty()) {
        sf::Text hint(fromUtf8("Нажмите для продолжения"), m_font, 14);
        hint.setPosition(m_panel.getPosition().x + 20, m_panel.getPosition().y + m_panel.getSize().y - 30);
        hint.setFillColor(sf::Color(200, 200, 200, 150));
        m_window.draw(hint);
    }
}

void DialogueManager::drawHistory() {
    m_window.draw(m_historyPanel);

    sf::Text title(fromUtf8("История диалога"), m_font, 24);
    title.setPosition(m_historyPanel.getPosition().x + 20, m_historyPanel.getPosition().y + 10);
    title.setFillColor(sf::Color::White);
    m_window.draw(title);

    m_historyCloseBtn->draw(m_window);

    float yOffset = 200;
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

void DialogueManager::toggleDialogue() {
    m_dialogueVisible = !m_dialogueVisible;
    if (m_dialogueVisible) {
        m_isPanelVisible = true;
        m_panelOpacity = 0.f;
    }
}

void DialogueManager::showHistoryWindow() {
    m_showHistory = true;
}

void DialogueManager::closeHistoryWindow() {
    m_showHistory = false;
}

void DialogueManager::skipDialogue() {
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

void DialogueManager::toggleSound() {
    m_soundEnabled = !m_soundEnabled;
    std::cout << "Звук " << (m_soundEnabled ? "включён" : "выключен") << std::endl;
}

void DialogueManager::setupUI() {
    m_panel.setFillColor(sf::Color(0, 0, 0, 0));
    m_panel.setOutlineThickness(2);
    m_panel.setOutlineColor(sf::Color(80, 80, 150));

    m_buttons["hide"] = std::make_unique<Button>(m_font, fromUtf8("Скрыть"), sf::Vector2f(0, 0), sf::Vector2f(80, 30));
    m_buttons["history"] = std::make_unique<Button>(m_font, fromUtf8("История"), sf::Vector2f(0, 0), sf::Vector2f(80, 30));
    m_buttons["skip"] = std::make_unique<Button>(m_font, fromUtf8("Пропуск"), sf::Vector2f(0, 0), sf::Vector2f(80, 30));

    m_buttons["hide"]->setCallback([this]() { toggleDialogue(); });
    m_buttons["history"]->setCallback([this]() { showHistoryWindow(); });
    m_buttons["skip"]->setCallback([this]() { skipDialogue(); });

    m_showBtn = std::make_unique<Button>(m_font, fromUtf8("Показать"), sf::Vector2f(0, 0), sf::Vector2f(90, 30));
    m_showBtn->setCallback([this]() { toggleDialogue(); });

    m_historyCloseBtn = std::make_unique<Button>(m_font, fromUtf8("Закрыть"), sf::Vector2f(0, 0), sf::Vector2f(90, 30));
    m_historyCloseBtn->setCallback([this]() { closeHistoryWindow(); });

    m_historyPanel.setFillColor(sf::Color(20, 20, 40, 230));
    m_historyPanel.setOutlineThickness(2);
    m_historyPanel.setOutlineColor(sf::Color(150, 150, 200));
}

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "Тест");
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

            //Клавиша "M" для включения/выключения звука
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::M) {
                manager.toggleSound();
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