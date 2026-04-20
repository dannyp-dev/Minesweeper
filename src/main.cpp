#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <fstream>
#include <string>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <algorithm>

class Tile {
public:
    bool hasMine = false;
    bool isRevealed = false;
    bool isFlagged = false;
    int adjacentMines = 0;
};

class PlayerScore {
public:
    int minutes;
    int seconds;
    std::string name;

    int getTotalSeconds() const {
        return minutes * 60 + seconds;
    }
};

bool compareScores(const PlayerScore &a, const PlayerScore &b) {
    return a.getTotalSeconds() < b.getTotalSeconds();
}

void setText(sf::Text &text, float x, float y) {
    sf::FloatRect textRect = text.getLocalBounds();
    text.setOrigin({textRect.position.x + textRect.size.x / 2.0f, (float)text.getCharacterSize() / 2.0f});
    text.setPosition({x, y});
}

void placeMines(std::vector<std::vector<Tile> > &tiles, int rowCount, int colCount, int mineCount) {
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

void calculateAdjacentMines(std::vector<std::vector<Tile> > &tiles, int rowCount, int colCount) {
    for (int row = 0; row < rowCount; row++) {
        for (int col = 0; col < colCount; col++) {
            if (!tiles[row][col].hasMine) {
                int mineCount = 0;

                for (int leftRightCol = -1; leftRightCol <= 1; leftRightCol++) {
                    for (int upDownRow = -1; upDownRow <= 1; upDownRow++) {
                        int checkRow = row + upDownRow;
                        int checkCol = col + leftRightCol;

                        if (checkRow >= 0 && checkRow < rowCount && checkCol >= 0 && checkCol < colCount) {
                            if (tiles[checkRow][checkCol].hasMine) {
                                mineCount++;
                            }
                        }
                    }
                }
                tiles[row][col].adjacentMines = mineCount;
            }
        }
    }
}

void revealTile(std::vector<std::vector<Tile> > &tiles, int row, int col, int rowCount, int colCount) {
    if (row < 0 || row >= rowCount || col < 0 || col >= colCount) {
        return;
    }
    if (tiles[row][col].isRevealed || tiles[row][col].hasMine || tiles[row][col].isFlagged) {
        return;
    }
    tiles[row][col].isRevealed = true;
    if (tiles[row][col].adjacentMines == 0) {
        for (int row_ = -1; row_ <= 1; row_++) {
            for (int col_ = -1; col_ <= 1; col_++) {
                revealTile(tiles, row + row_, col + col_, rowCount, colCount);
            }
        }
    }
}

bool checkVictory(std::vector<std::vector<Tile> > &tiles, int rowCount, int colCount) {
    for (int row = 0; row < rowCount; row++) {
        for (int col = 0; col < colCount; col++) {
            if (!tiles[row][col].hasMine && !tiles[row][col].isRevealed) {
                return false;
            }
        }
    }
    return true;
}

void showLeaderboard(int rowCount, int colCount, sf::Font &font, std::string highlightStr = "") {
    unsigned int leaderWidth = colCount * 16;
    unsigned int leaderHeight = (rowCount * 16) + 50;

    sf::RenderWindow leaderWindow(sf::VideoMode({leaderWidth, leaderHeight}), "Minesweeper",
                                  sf::Style::Titlebar | sf::Style::Close);

    sf::Text titleText(font, "LEADERBOARD", 20);
    titleText.setStyle(sf::Text::Bold | sf::Text::Underlined);
    titleText.setFillColor(sf::Color::White);
    setText(titleText, (float) leaderWidth / 2.0f, ((float) leaderHeight / 2.0f) - 120.0f);

    std::string leaderboardContent = "";
    std::ifstream file("files/leaderboard.txt");
    if (file.is_open()) {
        std::string line;
        int rank = 1;
        while (std::getline(file, line) && rank <= 5) {
            if (!highlightStr.empty() && line == highlightStr) {
                line += "*";
                highlightStr = "";
            }
            size_t commaPos = line.find(',');
            if (commaPos != std::string::npos) {
                line.replace(commaPos, 1, "\t");
            }
            leaderboardContent += std::to_string(rank) + ".\t" + line + "\n\n";
            rank++;
        }
        file.close();
    }

    sf::Text contentText(font, leaderboardContent, 18);
    contentText.setStyle(sf::Text::Bold);
    contentText.setFillColor(sf::Color::White);
    setText(contentText, (float) leaderWidth / 2.0f, ((float) leaderHeight / 2.0f) + 20.0f);

    while (leaderWindow.isOpen()) {
        while (const std::optional event = leaderWindow.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                leaderWindow.close();
            }
        }
        leaderWindow.clear(sf::Color::Blue);
        leaderWindow.draw(titleText);
        leaderWindow.draw(contentText);
        leaderWindow.display();
    }
}

void updateLeaderboard(int elapsedSeconds, std::string playerName) {
    std::vector<PlayerScore> scores;
    std::ifstream inFile("files/leaderboard.txt");
    std::string line;

    if (inFile.is_open()) {
        while (std::getline(inFile, line)) {
            std::stringstream ss(line);
            std::string timeStr, name;
            if (std::getline(ss, timeStr, ',') && std::getline(ss, name)) {
                PlayerScore score;
                size_t colonPos = timeStr.find(':');
                if (colonPos != std::string::npos) {
                    score.minutes = std::stoi(timeStr.substr(0, colonPos));
                    score.seconds = std::stoi(timeStr.substr(colonPos + 1));
                    score.name = name + " ";
                    scores.push_back(score);
                }
            }
        }
        inFile.close();
    }

    PlayerScore newScore;
    newScore.minutes = elapsedSeconds / 60;
    newScore.seconds = elapsedSeconds % 60;
    newScore.name = playerName;
    scores.push_back(newScore);

    std::sort(scores.begin(), scores.end(), compareScores);

    if (scores.size() > 5) {
        scores.pop_back();
    }

    std::ofstream outFile("files/leaderboard.txt");
    if (outFile.is_open()) {
        for (int i = 0; i < scores.size(); i++) {
            std::string minStr = (scores[i].minutes < 10)
                                     ? "0" + std::to_string(scores[i].minutes)
                                     : std::to_string(scores[i].minutes);
            std::string secStr = (scores[i].seconds < 10)
                                     ? "0" + std::to_string(scores[i].seconds)
                                     : std::to_string(scores[i].seconds);
            outFile << minStr << ":" << secStr << "," << scores[i].name << "\n";
        }
        outFile.close();
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

    std::vector<std::vector<Tile> > tiles(rowCount, std::vector<Tile>(colCount));

    srand(time(0));
    placeMines(tiles, rowCount, colCount, mineCount);
    calculateAdjacentMines(tiles, rowCount, colCount);

    unsigned int windowWidth = colCount * 32;
    unsigned int windowHeight = (rowCount * 32) + 100;

    sf::RenderWindow welcomeWindow(sf::VideoMode({windowWidth, windowHeight}), "Minesweeper",
                                   sf::Style::Titlebar | sf::Style::Close);

    sf::Font font;
    if (!font.openFromFile("files/font.ttf")) {
        return -1;
    }

    sf::Text titleName(font, "WELCOME TO MINESWEEPER!", 24);
    titleName.setStyle(sf::Text::Bold | sf::Text::Underlined);
    titleName.setFillColor(sf::Color::White);
    setText(titleName, (float) windowWidth / 2.0f, ((float) windowHeight / 2.0f) - 150.0f);

    sf::Text enterNameText(font, "Enter your name:", 20);
    enterNameText.setStyle(sf::Text::Bold);
    enterNameText.setFillColor(sf::Color::White);
    setText(enterNameText, (float) windowWidth / 2.0f, ((float) windowHeight / 2.0f) - 75.0f);

    std::string nameInput = "";
    sf::Text nameDisplay(font, "|", 18);
    nameDisplay.setStyle(sf::Text::Bold);
    nameDisplay.setFillColor(sf::Color::Yellow);
    setText(nameDisplay, (float) windowWidth / 2.0f, ((float) windowHeight / 2.0f) - 45.0f);

    bool startGame = false;
    while (welcomeWindow.isOpen()) {
        while (const std::optional event = welcomeWindow.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                welcomeWindow.close();
            }

            if (const auto *textEvent = event->getIf<sf::Event::TextEntered>()) {
                unsigned int unicode = textEvent->unicode;

                if (std::isalpha(unicode) && nameInput.length() < 10) {
                    char entered = (char) unicode;

                    if (nameInput.empty()) {
                        entered = std::toupper(entered);
                    } else {
                        entered = std::tolower(entered);
                    }

                    nameInput += entered;
                } else if (unicode == '\b' && !nameInput.empty()) {
                    nameInput.pop_back();
                }

                nameDisplay.setString(nameInput + "|");
                setText(nameDisplay, (float) windowWidth / 2.0f, ((float) windowHeight / 2.0f) - 45.0f);
            }

            if (const auto *keyEvent = event->getIf<sf::Event::KeyPressed>()) {
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
        sf::RenderWindow gameWindow(sf::VideoMode({windowWidth, windowHeight}), "Minesweeper",
                                    sf::Style::Titlebar | sf::Style::Close);

        sf::Texture happyFaceTexture, debugTexture, pauseTexture, leaderboardTexture, titleHiddenTexture,
                titleRevealedTexture, flag, mineTexture, numberTextures[8], faceLoseTexture, digitsTexture,
                faceWinTexture, playTexture;
        happyFaceTexture.loadFromFile("files/images/face_happy.png");
        debugTexture.loadFromFile("files/images/debug.png");
        pauseTexture.loadFromFile("files/images/pause.png");
        leaderboardTexture.loadFromFile("files/images/leaderboard.png");
        titleHiddenTexture.loadFromFile("files/images/tile_hidden.png");
        titleRevealedTexture.loadFromFile("files/images/tile_revealed.png");
        flag.loadFromFile("files/images/flag.png");
        mineTexture.loadFromFile("files/images/mine.png");
        faceLoseTexture.loadFromFile("files/images/face_lose.png");
        digitsTexture.loadFromFile("files/images/digits.png");
        faceWinTexture.loadFromFile("files/images/face_win.png");
        playTexture.loadFromFile("files/images/play.png");

        sf::Sprite faceBtn(happyFaceTexture);
        sf::Sprite debugTextureBtn(debugTexture);
        sf::Sprite pauseBtn(pauseTexture);
        sf::Sprite leaderBtn(leaderboardTexture);
        sf::Sprite hiddenTile(titleHiddenTexture);
        sf::Sprite revealedTile(titleRevealedTexture);
        sf::Sprite flagImg(flag);
        sf::Sprite mineImg(mineTexture);
        std::vector<sf::Sprite> numberSprites;
        sf::Sprite digitSprite(digitsTexture);

        sf::Clock gameClock;
        sf::Clock pauseClock;
        int pauseTimeOffset = 0;
        int elapsedSeconds = 0;
        bool isPaused = false;


        for (int i = 0; i < 8; i++) {
            numberTextures[i].loadFromFile("files/images/number_" + std::to_string(i + 1) + ".png");
            sf::Sprite newSprite(numberTextures[i]);
            numberSprites.push_back(newSprite);
        }

        int flagsPlaced = 0;


        float uiY = 32.0f * ((float) rowCount + 0.5f);
        faceBtn.setPosition({((float) colCount / 2.0f) * 32.0f, uiY});
        debugTextureBtn.setPosition({((float) colCount * 32.0f) - 304.0f, uiY});
        pauseBtn.setPosition({((float) colCount * 32.0f) - 240.0f, uiY});
        leaderBtn.setPosition({((float) colCount * 32.0f) - 176.0f, uiY});

        bool debugMode = false;
        bool gameLost = false;
        bool gameWon = false;

        while (gameWindow.isOpen()) {
            while (const std::optional event = gameWindow.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    gameWindow.close();
                }
                if (const auto *mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                    float mouseX = mousePressed->position.x;
                    float mouseY = mousePressed->position.y;

                    if (mousePressed->button == sf::Mouse::Button::Left) {
                        if (faceBtn.getGlobalBounds().contains({mouseX, mouseY})) {
                            gameLost = false;
                            gameWon = false;
                            isPaused = false;
                            debugMode = false;
                            flagsPlaced = 0;
                            pauseTimeOffset = 0;

                            tiles.assign(rowCount, std::vector<Tile>(colCount));
                            placeMines(tiles, rowCount, colCount, mineCount);
                            calculateAdjacentMines(tiles, rowCount, colCount);

                            faceBtn.setTexture(happyFaceTexture);
                            pauseBtn.setTexture(pauseTexture);
                            gameClock.restart();
                        }

                        if (pauseBtn.getGlobalBounds().contains({mouseX, mouseY}) && !gameLost && !gameWon) {
                            isPaused = !isPaused;
                            if (isPaused) {
                                pauseBtn.setTexture(playTexture);
                                pauseClock.restart();
                            } else {
                                pauseBtn.setTexture(pauseTexture);
                                pauseTimeOffset += pauseClock.getElapsedTime().asSeconds();
                            }
                        }
                        if (debugTextureBtn.getGlobalBounds().contains({mouseX, mouseY})) {
                            debugMode = !debugMode;
                        }
                    }
                    if (leaderBtn.getGlobalBounds().contains({mouseX, mouseY})) {
                        bool wasPaused = isPaused;
                        isPaused = true;

                        gameWindow.clear(sf::Color::White);
                        for (int r = 0; r < rowCount; r++) {
                            for (int c = 0; c < colCount; c++) {
                                revealedTile.setPosition({(float) c * 32.0f, (float) r * 32.0f});
                                gameWindow.draw(revealedTile);
                            }
                        }
                        gameWindow.draw(faceBtn);
                        gameWindow.draw(debugTextureBtn);
                        gameWindow.draw(pauseBtn);
                        gameWindow.draw(leaderBtn);
                        gameWindow.display();
                        pauseClock.restart();
                        showLeaderboard(rowCount, colCount, font);
                        if (!wasPaused && !gameLost && !gameWon) {
                            isPaused = false;
                            pauseTimeOffset += pauseClock.getElapsedTime().asSeconds();
                        }
                    }

                    int col = (int) (mouseX / 32.0f);
                    int row = (int) (mouseY / 32.0f);

                    if (!gameLost && !gameWon && !isPaused) {
                        if (row >= 0 && row < rowCount && col >= 0 && col < colCount) {
                            if (mousePressed->button == sf::Mouse::Button::Right) {
                                if (!tiles[row][col].isRevealed) {
                                    tiles[row][col].isFlagged = !tiles[row][col].isFlagged;
                                    if (tiles[row][col].isFlagged) {
                                        flagsPlaced++;
                                    } else {
                                        flagsPlaced--;
                                    }
                                }
                            } else if (mousePressed->button == sf::Mouse::Button::Left) {
                                if (!tiles[row][col].isFlagged && !tiles[row][col].isRevealed) {
                                    if (tiles[row][col].hasMine) {
                                        gameLost = true;
                                        tiles[row][col].isRevealed = true;
                                        faceBtn.setTexture(faceLoseTexture);
                                    } else {
                                        revealTile(tiles, row, col, rowCount, colCount);

                                        if (checkVictory(tiles, rowCount, colCount)) {
                                            gameWon = true;
                                            faceBtn.setTexture(faceWinTexture);

                                            for (int r = 0; r < rowCount; r++) {
                                                for (int c = 0; c < colCount; c++) {
                                                    if (tiles[r][c].hasMine && !tiles[r][c].isFlagged) {
                                                        tiles[r][c].isFlagged = true;
                                                    }
                                                }
                                            }
                                            flagsPlaced = mineCount;

                                            updateLeaderboard(elapsedSeconds, nameInput);

                                            int winMin = elapsedSeconds / 60;
                                            int winSec = elapsedSeconds % 60;
                                            std::string winMinStr = (winMin < 10) ? "0" + std::to_string(winMin) : std::to_string(winMin);
                                            std::string winSecStr = (winSec < 10) ? "0" + std::to_string(winSec) : std::to_string(winSec);
                                            std::string highlightStr = winMinStr + ":" + winSecStr + "," + nameInput;

                                            showLeaderboard(rowCount, colCount, font, highlightStr);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (!gameWon && !gameLost && !isPaused) {
                elapsedSeconds = gameClock.getElapsedTime().asSeconds() - pauseTimeOffset;
            }
            gameWindow.clear(sf::Color::White);
            for (int row = 0; row < rowCount; row++) {
                for (int col = 0; col < colCount; col++) {
                    float xPos = (float) col * 32.0f;
                    float yPos = (float) row * 32.0f;

                    if (isPaused) {
                        revealedTile.setPosition({xPos, yPos});
                        gameWindow.draw(revealedTile);
                        continue;
                    }

                    if (tiles[row][col].isRevealed) {
                        revealedTile.setPosition({xPos, yPos});
                        gameWindow.draw(revealedTile);

                        if (tiles[row][col].hasMine) {
                            mineImg.setPosition({xPos, yPos});
                            gameWindow.draw(mineImg);
                        } else if (tiles[row][col].adjacentMines > 0) {
                            int numIndex = tiles[row][col].adjacentMines - 1;
                            numberSprites[numIndex].setPosition({xPos, yPos});
                            gameWindow.draw(numberSprites[numIndex]);
                        }
                    } else {
                        hiddenTile.setPosition({xPos, yPos});
                        gameWindow.draw(hiddenTile);

                        if (tiles[row][col].isFlagged) {
                            flagImg.setPosition({xPos, yPos});
                            gameWindow.draw(flagImg);
                        }
                        if ((debugMode || gameLost) && tiles[row][col].hasMine) {
                            mineImg.setPosition({xPos, yPos});
                            gameWindow.draw(mineImg);
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

            int counter = mineCount - flagsPlaced;
            int absCounter = std::abs(counter);

            int hundreds = (absCounter / 100) % 10;
            int tens = (absCounter / 10) % 10;
            int ones = absCounter % 10;

            float counterY = 32.0f * ((float) rowCount + 0.5f) + 16.0f;

            if (counter < 0) {
                digitSprite.setTextureRect(sf::IntRect({10 * 21, 0}, {21, 32}));
                digitSprite.setPosition({12.0f, counterY});
                gameWindow.draw(digitSprite);
            }

            digitSprite.setTextureRect(sf::IntRect({hundreds * 21, 0}, {21, 32}));
            digitSprite.setPosition({33.0f, counterY});
            gameWindow.draw(digitSprite);

            digitSprite.setTextureRect(sf::IntRect({tens * 21, 0}, {21, 32}));
            digitSprite.setPosition({33.0f + 21.0f, counterY});
            gameWindow.draw(digitSprite);

            digitSprite.setTextureRect(sf::IntRect({ones * 21, 0}, {21, 32}));
            digitSprite.setPosition({33.0f + 42.0f, counterY});
            gameWindow.draw(digitSprite);

            int minutes = elapsedSeconds / 60;
            int seconds = elapsedSeconds % 60;

            int minTens = (minutes / 10) % 10;
            int minOnes = minutes % 10;
            int secTens = (seconds / 10) % 10;
            int secOnes = seconds % 10;

            float timerX = (float) (colCount * 32);

            digitSprite.setTextureRect(sf::IntRect({minTens * 21, 0}, {21, 32}));
            digitSprite.setPosition({timerX - 97.0f, counterY});
            gameWindow.draw(digitSprite);

            digitSprite.setTextureRect(sf::IntRect({minOnes * 21, 0}, {21, 32}));
            digitSprite.setPosition({timerX - 76.0f, counterY});
            gameWindow.draw(digitSprite);

            digitSprite.setTextureRect(sf::IntRect({secTens * 21, 0}, {21, 32}));
            digitSprite.setPosition({timerX - 54.0f, counterY});
            gameWindow.draw(digitSprite);

            digitSprite.setTextureRect(sf::IntRect({secOnes * 21, 0}, {21, 32}));
            digitSprite.setPosition({timerX - 33.0f, counterY});
            gameWindow.draw(digitSprite);
            gameWindow.display();
        }
        return 0;
    }
}
