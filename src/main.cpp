#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <fstream>
#include <string>
#include <cctype>
#include <cstdlib>
#include <ctime>

class Tile {
    public:
        bool hasMine = false;
        bool isRevealed = false;
        bool isFlagged = false;
        int adjacentMines = 0;
};

void setText(sf::Text &text, float x, float y) {
    sf::FloatRect textRect = text.getLocalBounds();
    text.setOrigin({textRect.position.x + textRect.size.x / 2.0f, textRect.position.y + textRect.size.y / 2.0f});
    text.setPosition({x, y});
}

void placeMines(std::vector<std::vector<Tile>>& tiles, int rowCount, int colCount, int mineCount) {
    int minesPlaced = 0;

    while (minesPlaced < mineCount) {
        int randomRow = rand() % rowCount;
        int randomCol = rand() % colCount;

        if (!tiles[randomRow][randomCol].hasMine) {
            tiles[randomRow][randomCol].hasMine = true;
            minesPlaced++;
        }
    }
}


int main() {
    int colCount = 25;
    int rowCount = 16;
    int mineCount = 50;

    std::ifstream configFile("files/config.cfg");
    if (configFile.is_open()) {
        configFile >> colCount >> rowCount >> mineCount;
    }

    std::vector<std::vector<Tile>> tiles(rowCount, std::vector<Tile>(colCount));

    srand(time(0));
    placeMines(tiles, rowCount, colCount, mineCount);

    unsigned int windowWidth = colCount * 32;
    unsigned int windowHeight = (rowCount * 32) + 100;

    sf::RenderWindow welcomeWindow(sf::VideoMode({windowWidth, windowHeight}), "Minesweeper", sf::Style::Titlebar | sf::Style::Close);

    sf::Font font;
    if (!font.openFromFile("files/font.ttf")) {
        return -1;
    }

    sf::Text titleName(font, "WELCOME TO MINESWEEPER!", 24);
    titleName.setStyle(sf::Text::Bold | sf::Text::Underlined);
    titleName.setFillColor(sf::Color::White);
    setText(titleName, (float)windowWidth / 2.0f, ((float)windowHeight / 2.0f) - 150.0f);

    sf::Text enterNameText(font, "Enter your name:", 20);
    enterNameText.setStyle(sf::Text::Bold);
    enterNameText.setFillColor(sf::Color::White);
    setText(enterNameText, (float)windowWidth / 2.0f, ((float)windowHeight / 2.0f) - 75.0f);

    std::string nameInput = "";
    sf::Text nameDisplay(font, "|", 18);
    nameDisplay.setStyle(sf::Text::Bold);
    nameDisplay.setFillColor(sf::Color::Yellow);
    setText(nameDisplay, (float)windowWidth / 2.0f, ((float)windowHeight / 2.0f) - 45.0f);

    bool startGame = false;
    while (welcomeWindow.isOpen()) {
        while (const std::optional event = welcomeWindow.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                welcomeWindow.close();
            }

            if (const auto* textEvent = event->getIf<sf::Event::TextEntered>()) {
                unsigned int unicode = textEvent->unicode;

                if (std::isalpha(unicode) && nameInput.length() < 10) {
                    char entered = (char)unicode;

                    if (nameInput.empty()) {
                        entered = std::toupper(entered);
                    } else {
                        entered = std::tolower(entered);
                    }

                    nameInput += entered;
                }

                else if (unicode == '\b' && !nameInput.empty()) {
                    nameInput.pop_back();
                }

                nameDisplay.setString(nameInput + "|");
                setText(nameDisplay, (float)windowWidth / 2.0f, ((float)windowHeight / 2.0f) - 45.0f);
            }

            if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                if (keyEvent->code == sf::Keyboard::Key::Enter && !nameInput.empty()) {
                    startGame = true;
                    welcomeWindow.close();
                }
            }
        }

        welcomeWindow.clear(sf::Color::Blue);
        welcomeWindow.draw(titleName);
        welcomeWindow.draw(enterNameText);
        welcomeWindow.draw(nameDisplay);
        welcomeWindow.display();
    }

    if (startGame) {
        sf::RenderWindow gameWindow(sf::VideoMode({windowWidth, windowHeight}), "Minesweeper", sf::Style::Titlebar | sf::Style::Close);

        sf::Texture happyFaceTexture, debugTexture, pauseTexture, leaderboardTexture, titleHiddenTexture, titleRevealedTexture, flag, mineTexture;
        happyFaceTexture.loadFromFile("files/images/face_happy.png");
        debugTexture.loadFromFile("files/images/debug.png");
        pauseTexture.loadFromFile("files/images/pause.png");
        leaderboardTexture.loadFromFile("files/images/leaderboard.png");
        titleHiddenTexture.loadFromFile("files/images/tile_hidden.png");
        titleRevealedTexture.loadFromFile("files/images/tile_revealed.png");
        flag.loadFromFile("files/images/flag.png");
        mineTexture.loadFromFile("files/images/mine.png");

        sf::Sprite faceBtn(happyFaceTexture);
        sf::Sprite debugTextureBtn(debugTexture);
        sf::Sprite pauseBtn(pauseTexture);
        sf::Sprite leaderBtn(leaderboardTexture);
        sf::Sprite hiddenTile(titleHiddenTexture);
        sf::Sprite revealedTile(titleRevealedTexture);
        sf::Sprite flagImg(flag);
        sf::Sprite mineImg(mineTexture);

        float uiY = 32.0f * ((float)rowCount + 0.5f);
        faceBtn.setPosition({((float)colCount / 2.0f) * 32.0f, uiY});
        debugTextureBtn.setPosition({((float)colCount * 32.0f) - 304.0f, uiY});
        pauseBtn.setPosition({((float)colCount * 32.0f) - 240.0f, uiY});
        leaderBtn.setPosition({((float)colCount * 32.0f) - 176.0f, uiY});

        bool debugMode = false;

        while (gameWindow.isOpen()) {
            while (const std::optional event = gameWindow.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    gameWindow.close();
                }
                if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                    float mouseX = mousePressed->position.x;
                    float mouseY = mousePressed->position.y;

                    if (mousePressed->button == sf::Mouse::Button::Left) {
                        if (debugTextureBtn.getGlobalBounds().contains({mouseX, mouseY})) {
                            debugMode = !debugMode;
                        }
                    }

                    int col = (int)(mouseX / 32.0f);
                    int row = (int)(mouseY / 32.0f);

                    if (row >= 0 && row < rowCount && col >= 0 && col < colCount) {
                        if (mousePressed->button == sf::Mouse::Button::Right) {
                            if (!tiles[row][col].isRevealed) {
                                tiles[row][col].isFlagged = !tiles[row][col].isFlagged;
                            }
                        } else if (mousePressed->button == sf::Mouse::Button::Left) {
                            if (!tiles[row][col].isFlagged && !tiles[row][col].isRevealed) {
                                tiles[row][col].isRevealed = true;
                            }
                        }
                    }
                }
            }
            gameWindow.clear(sf::Color::White);
            for (int row = 0; row < rowCount; ++row) {
                for (int col = 0; col < colCount; ++col) {
                    float xPos = (float)col * 32.0f;
                    float yPos = (float)row * 32.0f;

                    if (tiles[row][col].isRevealed) {
                        revealedTile.setPosition({xPos, yPos});
                        gameWindow.draw(revealedTile);
                    } else {
                        hiddenTile.setPosition({xPos, yPos});
                        gameWindow.draw(hiddenTile);

                        if (tiles[row][col].isFlagged) {
                            flagImg.setPosition({xPos, yPos});
                            gameWindow.draw(flagImg);
                        }
                        if (debugMode && tiles[row][col].hasMine) {
                            mineImg.setPosition({xPos, yPos});
                            gameWindow.draw(mineImg);
                        }
                    }
                }
            }
            gameWindow.draw(faceBtn);
            gameWindow.draw(debugTextureBtn);
            gameWindow.draw(pauseBtn);
            gameWindow.draw(leaderBtn);
            gameWindow.display();
        }
        return 0;
    }
}