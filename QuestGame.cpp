#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp> 
#include <iostream>
#include <cstdint>
#include <vector>
#include <random>
#include <algorithm>
#include <SFML/Audio.hpp>

// GAME STATE ENUM
enum class GameState {
    HOME,
    TIME_SELECT,
    QUIZ,
    CREDITS,
    HELP,
    COMPLETE
};

// FEEDBACK VARIABLES
sf::Color feedbackColor = sf::Color::Transparent;
bool showFeedback = false;
sf::Clock feedbackClock;
int lastClickedAnswer = -1;
bool inputBlocked = false;

// HITBOX STRUCT FOR MOUSE INTERACTIONS 
struct Hitbox {
    sf::Vector2f position;
    sf::Vector2f size;
    bool contains(const sf::Vector2f& point) const {
        return point.x >= position.x &&
            point.x <= position.x + size.x &&
            point.y >= position.y &&
            point.y <= position.y + size.y;
    }
};
struct ButtonAnim {
    float scale = 1.f;
    float targetScale = 1.f;
};

// BUTTON ANIMATION FUNCTION
void animateButton(sf::Sprite& sprite, ButtonAnim& anim, float dt) {
    const float speed = 12.f;
    anim.scale += (anim.targetScale - anim.scale) * speed * dt;
    sprite.setScale({ anim.scale, anim.scale });
}

// QUESTION STRUCT
struct Question {
    std::string text;
    std::vector<std::string> answers;
    int correctIndex;
    int score = 0;
};

// MAIN FUNCTION
int main() {
    sf::RenderWindow window(sf::VideoMode({ 1500, 900 }), "LOGIQ v1.0.0 - SFML 3.0.2");
    window.setFramerateLimit(60);

    GameState state = GameState::HOME;
    GameState nextState = state;

    bool isFading = false;
    bool fadeOut = true;
    bool quizTimerRunning = false;

    float fadeAlpha = 0.f;
    const float fadeSpeed = 500.f;
    sf::Clock fadeClock;

    int selectedTime = 0;
    sf::Clock quizClock;

    // LOAD FONT
    sf::Font lilitaFont;
    if (!lilitaFont.openFromFile("assets/LilitaOne-Regular.ttf")) {
        std::cerr << "Font failed to load\n";
        return 1;
    }
    sf::Font lilexFont;
    if (!lilexFont.openFromFile("assets/Lilex-SemiBold.ttf")) {
        std::cerr << "Lilex font failed to load\n";
        return 1;
    }

	// TIMER TEXT
    float remainingTime = 0.f;
    sf::Text timerText(lilexFont);
    timerText.setCharacterSize(60);
    timerText.setFillColor(sf::Color(101, 67, 33));
    timerText.setPosition({ 50.f, 40.f }); 

	// LOAD TEXTURES
    sf::Texture texTitle, texHomePage, texbtnStart, texbtnCredits, texbtnHelp;
    sf::Texture texCreditsPage, texHelpPage, texChoose;
    sf::Texture tex1m, tex2m, tex3m, texBack;
    sf::Texture texQuestionBG, texA1, texA2, texA3, texA4;
	sf::Texture texComplete, texTryAgain, texExit;

	// LOADING ASSETS - ENSURE ALL FILE EXIST
    if (!texTitle.loadFromFile("assets/titleLOGIQ.png") ||
        !texHomePage.loadFromFile("assets/homepage.png") ||
        !texbtnStart.loadFromFile("assets/btnStart.png") ||
        !texbtnCredits.loadFromFile("assets/btnCredits.png") ||
        !texbtnHelp.loadFromFile("assets/btnHelp.png") ||
        !texCreditsPage.loadFromFile("assets/creditsPage.png") ||
        !texHelpPage.loadFromFile("assets/helpPage.png") ||
        !texChoose.loadFromFile("assets/timeSelectionPage.png") ||
        !tex1m.loadFromFile("assets/btn1min.png") ||
        !tex2m.loadFromFile("assets/btn2mins.png") ||
        !tex3m.loadFromFile("assets/btn3mins.png") ||
        !texBack.loadFromFile("assets/btnBack.png") ||
        !texQuestionBG.loadFromFile("assets/questionPage.png") ||
        !texA1.loadFromFile("assets/btnA1.png") ||
        !texA2.loadFromFile("assets/btnA2.png") ||
        !texA3.loadFromFile("assets/btnA3.png") ||
        !texComplete.loadFromFile("assets/completePage.png") ||
        !texTryAgain.loadFromFile("assets/btnTryAgain.png") ||
        !texExit.loadFromFile("assets/btnExit.png") ||
        !texA4.loadFromFile("assets/btnA4.png"))
    {
        std::cerr << "Asset loading failed\n";
        return 1;
    }

	// INITIALIZATION OF SPRITES 
    sf::Sprite home(texHomePage);
    sf::Sprite titleLOGIQ(texTitle);
    sf::Sprite btnStart(texbtnStart);
    sf::Sprite btnHelp(texbtnHelp);
    sf::Sprite btnCredits(texbtnCredits);
    sf::Sprite creditsPage(texCreditsPage);
    sf::Sprite helpPage(texHelpPage);
    sf::Sprite chooseBG(texChoose);
    sf::Sprite btn1min(tex1m);
    sf::Sprite btn2mins(tex2m);
    sf::Sprite btn3mins(tex3m);
    sf::Sprite btnBack(texBack);
    sf::Sprite questionBG(texQuestionBG);
    sf::Sprite btnA1(texA1);
    sf::Sprite btnA2(texA2);
    sf::Sprite btnA3(texA3);
    sf::Sprite btnA4(texA4);
    sf::Sprite answerBtns[4] = { btnA1, btnA2, btnA3, btnA4 };
    sf::Sprite btnTryAgain(texTryAgain);
    sf::Sprite btnExit(texExit);

	// BUTTON ANIMATIONS
    ButtonAnim animStart, animHelp, animCredits, animBack;
    ButtonAnim anim1min, anim2mins, anim3mins;
    ButtonAnim animAnswers[4];
    ButtonAnim animTryAgain, animExit;

	// FADE IN RECTANGLE
    sf::RectangleShape fadeRect({ 1500.f, 900.f });
    fadeRect.setFillColor(sf::Color(0, 0, 0, 0));

    auto centerOrigin = [](sf::Sprite& spr) {
        sf::FloatRect lb = spr.getLocalBounds();
        spr.setOrigin({ lb.size.x / 2.f, lb.size.y / 2.f });
        };
    for (int i = 0; i < 4; i++) centerOrigin(answerBtns[i]);

	// POSITIONING OF BUTTONS
    btnStart.setPosition({ 750.f, 540.f });
    btnHelp.setPosition({ 750.f, 670.f });
    btnCredits.setPosition({ 750.f, 800.f });
    btnBack.setPosition({ 90.f, 800.f });
    btn1min.setPosition({ 400.f, 470.f });
    btn2mins.setPosition({ 830.f, 470.f });
    btn3mins.setPosition({ 600.f, 600.f });
    btnTryAgain.setPosition({ 570.f, 600.f });
    btnExit.setPosition({ 930.f, 600.f });

    // CENTER ORIGINS
    centerOrigin(btnStart);
    centerOrigin(btnHelp);
    centerOrigin(btnCredits);
    centerOrigin(btnBack);
    centerOrigin(btnTryAgain);
    centerOrigin(btnExit);
    centerOrigin(btnTryAgain);
    centerOrigin(btnExit);


	// ANSWER BUTTON POSITIONS
    sf::Vector2f answerPositions[4] = {
        {400.f, 650.f},
        {1100.f, 650.f},
        {400.f, 810.f},
        {1100.f, 810.f}
    };
    for (int i = 0; i < 4; i++) answerBtns[i].setPosition(answerPositions[i]);

	// HITBOXES FOR INTERACTIONS
    Hitbox hitStart, hitHelp, hitCredits, hitBack;
    Hitbox answerHitboxes[4];
    Hitbox hitTryAgain, hitExit;

    auto updateHitbox = [](Hitbox& hb, const sf::Sprite& spr) {
        sf::FloatRect gb = spr.getGlobalBounds();
        hb.position = { gb.position.x, gb.position.y };
        hb.size = { gb.size.x, gb.size.y };
        };

	// INITIAL HITBOX UPDATES
    sf::FloatRect tb = titleLOGIQ.getLocalBounds();
    titleLOGIQ.setOrigin({ tb.size.x / 2.f, tb.size.y / 2.f });
    titleLOGIQ.setPosition({ 750.f, -200.f });
    const float titleTargetY = 450.f;
    const float titleSpeed = 1500.f;
    sf::Clock titleClock;
    bool titleArrived = false;

	// RANDOMIZE QUIZ QUESTIONN
    std::vector<Question> quizQuestions = {
        {
            "\n\n         A is taller than B.\n"
            "          B is taller than C.\n\n"
            "       Who is the shortest?",
            {"A", "B", "C", "D"},
            2
        },
        {
            "\n\n        The more you take from me,\n"
            "                   the bigger I get.\n\n"
            "                       What am I?",
            {"Puddle", "Pit", "Hole", "Ditch"},
            2
        },
        {
            "\n\n         I speak without a mouth,\n"
            "                  hear without ears,\n"
            "      and answer when spoken to.\n\n"
            "                      What am I?",
            {"An echo", "A ghost", "A shadow", "A thought"},
            0
        },
        {
            "\n\n\n         What can travel around the world\n"
            "              while staying in a corner?",
            {"Globe", "Plane", "Compass", "Stamp"},
            3
        },
        {
            "\n\n       You see a boat filled with people,\n"
            "                   yet there isn’t a single \n                         person on board.\n\n"
            "                  How is that possible?",
            {"They're married", "They're invisible", "He jumped off", "A ghost ship"},
            0
        },
        {
            "\n\n    I’m tall when I’m young\n"
            "    and short when I’m old.\n\n"
            "                 What am I?",
            {"Chalk", "Pencil", "Candle", "Matchstick"},
            2
        },
        {
            "\n            It belongs to you,\n"
            "       but other people use \n          it more than you do.\n\n"
            "                What is it?",
            {"Your ID", "Your name", "Pen", "Comb"},
            1
        },
        {
            "\n\n     What starts with T,\n"
            "             ends with T,\n"
            "      and has T inside it?",
            {"Tent", "Teacup", "Ticket", "Teapot"},
            3
        },
        {
            "\n\n\n         What goes away as soon \n      as you talk about it?",
            {"Noise", "Silence", "Secret", "Shadow"},
            1
        },
        {
            "\n\n         What can run but never walks,\n"
            "         has a mouth but never talks,\n"
            "         has a head but never weeps,\n"
            "         has a bed but never sleeps?",
            {"Clock", "Train", "River", "Road"},
            2
        },
        {
            "\n\n\n\n         What has hands but can’t clap?",
            {"Robot", "Clock", "Statue", "Mannequin"},
            1
        },
        {
            "\n         A is the brother of B.\n"
            "         B is the brother of C.\n"
            "         C is the father of D.\n\n"
            "        How is D related to A?",
            {"Niece", "Child", "Uncle", "Cousin"},
            0
        },
        {
            "\n\n         I can fall off a building and live,\n"
            "         but put me in water and I will die.\n\n"
            "                         What am I?",
            {"Candle", "Paper", "Leaf", "Feather"},
            1
        },
        {
            "\n\n         I have a face but no eyes,\n"
            "         hands but no arms.\n\n"
            "                 What am I?",
            {"Doll", "Clock", "Mirror", "Mask"},
            1
        },
        {
            "\n\n\n         What falls but never breaks,\n"
            "         and breaks but never falls?",
            {"Day & night", "Dawn & dusk", "Rain & hail", "Snow & ice"},
            0
        },
        {
            "\n\n         I’m not a teacher,\n"
            "      but I help you write.\n\n"
            "                What am I?",
            {"Sister", "Book", "Parents", "Pencil"},
            3
        },
        {
            "\n\n\n         What can go up a chimney down,\n"
            "         but can’t go down a chimney up?",
            {"Umbrella", "Person", "Balloon", "Hat"},
            0
        },
        {
            "\n\n             I can fill a room,\n"
            "         but I take up no space.\n\n"
            "                      What am I?",
            {"Water", "Air", "Light", "Fire"},
            2
        },
        {
            "\n\n\n         I’m always in front of you\n"
            "             but can’t be seen.\n\n"
            "                   What am I?",
            {"Future", "Nose", "Eyelashes", "Human"},
            0
        },

    };

	// GAME LOOP VARIABLES
    int currentQuestion = 0;
    int score = 0;
    std::vector<int> answerOrder = { 0,1,2,3 };
    std::mt19937 rng(std::random_device{}());

    while (window.isOpen()) {
        if (state == GameState::HOME) {
            updateHitbox(hitStart, btnStart);
            updateHitbox(hitHelp, btnHelp);
            updateHitbox(hitCredits, btnCredits);
        }
        else {
            updateHitbox(hitBack, btnBack);
        }

		// UPDATING THE ANSWER HITBOXEE
        for (int i = 0; i < 4; i++)
            updateHitbox(answerHitboxes[i], answerBtns[i]);

        if (state == GameState::COMPLETE) {
            updateHitbox(hitTryAgain, btnTryAgain);
            updateHitbox(hitExit, btnExit);
        }

		// EVENT POLLING
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();

            if (!isFading) {
                if (auto mouse = event->getIf<sf::Event::MouseButtonPressed>()) {
                    if (mouse->button == sf::Mouse::Button::Left) {
                        sf::Vector2f pos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

						// HANDLING TIMER CLICK EVENTS
                        if (state == GameState::TIME_SELECT) {
                            if (btn1min.getGlobalBounds().contains(pos)) {
                                selectedTime = 60;
                                currentQuestion = 0;
                                remainingTime = selectedTime;
                                quizClock.restart();
                                quizTimerRunning = true;   
                                nextState = GameState::QUIZ;

                            } 
                            else if (btn2mins.getGlobalBounds().contains(pos)) {
                                selectedTime = 120;
                                currentQuestion = 0;
                                remainingTime = selectedTime;
                                quizClock.restart();
                                quizTimerRunning = true;  
                                nextState = GameState::QUIZ;

                            }
                            else if (btn3mins.getGlobalBounds().contains(pos)) {
                                selectedTime = 180;
                                currentQuestion = 0;
                                remainingTime = selectedTime;
                                quizClock.restart();
                                quizTimerRunning = true;   
                                nextState = GameState::QUIZ;

                            }
                            else if (hitBack.contains(pos)) {
                                nextState = GameState::HOME;
                            }
                        }

						// HANDLING CLICK EVENTS DIFFERENT GAME STATESS
                        if (state == GameState::HOME) {
                            if (hitStart.contains(pos)) nextState = GameState::TIME_SELECT;
                            else if (hitCredits.contains(pos)) nextState = GameState::CREDITS;
                            else if (hitHelp.contains(pos)) nextState = GameState::HELP;
                        }
                        if (state == GameState::COMPLETE) {
                            animTryAgain.targetScale = (hitTryAgain.contains(pos)) ? 1.1f : 1.f;
                            animExit.targetScale = (hitExit.contains(pos)) ? 1.1f : 1.f;
                        }

                        if (state == GameState::COMPLETE) {
                            if (hitTryAgain.contains(pos)) {
                                score = 0;                
                                currentQuestion = 0;
                                nextState = GameState::TIME_SELECT;
                                isFading = true; fadeOut = true; fadeClock.restart();
                            }
                            else if (hitExit.contains(pos)) {
                                nextState = GameState::HOME;
                                isFading = true; fadeOut = true; fadeClock.restart();
                            }
                        }
                        else if (state == GameState::TIME_SELECT) {
                            if (btn1min.getGlobalBounds().contains(pos)) {
                                selectedTime = 60;
                                score = 0;
                                currentQuestion = 0;
                                remainingTime = selectedTime; 
                                quizClock.restart();        
                                nextState = GameState::QUIZ;
                            }
                            else if (btn2mins.getGlobalBounds().contains(pos)) {
                                selectedTime = 120;
                                score = 0;
                                currentQuestion = 0;
                                remainingTime = selectedTime;
                                quizClock.restart();
                                nextState = GameState::QUIZ;
                            }
                            else if (btn3mins.getGlobalBounds().contains(pos)) {
                                selectedTime = 180;
                                score = 0;
                                currentQuestion = 0;
                                remainingTime = selectedTime;
                                quizClock.restart();
                                nextState = GameState::QUIZ;
                            }

                            else if (hitBack.contains(pos)) nextState = GameState::HOME;
                        }

                        else if (state == GameState::QUIZ && !inputBlocked) {  
                            for (int i = 0; i < 4; i++) {
                                if (answerHitboxes[i].contains(pos)) {
                                    int chosen = answerOrder[i];
                                    int correct = quizQuestions[currentQuestion].correctIndex;
                                    lastClickedAnswer = i;
                                    inputBlocked = true; 

									// FEEDBACK COLOR LOGIC
                                    if (chosen == correct) {             
                                        feedbackColor = sf::Color(0, 255, 0, 120); 
                                    }
                                    else {
                                        feedbackColor = sf::Color(139, 0, 0, 150); 
                                    }

                                    showFeedback = true;
                                    feedbackClock.restart();

                                    if (chosen == quizQuestions[currentQuestion].correctIndex) {
                                        std::cout << "CORRECT +1\n";
                                        score++;
                                    }
                                    else {
                                        std::cout << "WRONG!\n";
                                    }

                                    if (currentQuestion >= quizQuestions.size()) {
                                        nextState = GameState::COMPLETE;
                                        isFading = true;
                                        fadeOut = true;
                                        fadeClock.restart();
                                    }
                                    break;
                                }
                            }

                        }
                        else if (hitBack.contains(pos)) {
                            nextState = GameState::HOME;
                        }

                        if (nextState != state) {
                            isFading = true;
                            fadeOut = true;
                            fadeClock.restart();
                        }
                    }
                }
            }
        }

        // TIMER UPDATE LOGIX
        if (state == GameState::QUIZ && quizTimerRunning && !isFading) {
            float dt = quizClock.restart().asSeconds();
            remainingTime -= dt;

            if (remainingTime <= 0.f) {
                remainingTime = 0.f;
                quizTimerRunning = false;
                nextState = GameState::COMPLETE;
                isFading = true;
                fadeOut = true;
                fadeClock.restart();
            }

			// FORMAT TIMER TEXT IN STTRING
            int minutes = static_cast<int>(remainingTime) / 60;
            int seconds = static_cast<int>(remainingTime) % 60;

            timerText.setString(
                (minutes < 10 ? "0" : "") + std::to_string(minutes) + ":" +
                (seconds < 10 ? "0" : "") + std::to_string(seconds)
            );
        }

        if (isFading) {
            float dt = fadeClock.restart().asSeconds();
            fadeAlpha += (fadeOut ? 1 : -1) * fadeSpeed * dt;
            if (fadeAlpha >= 180.f) { fadeAlpha = 180.f;

			// SHUFFLE PART OF QUESTION LOGIC
            if (nextState == GameState::QUIZ && state != GameState::QUIZ) {
                std::shuffle(quizQuestions.begin(), quizQuestions.end(), rng);
                std::shuffle(answerOrder.begin(), answerOrder.end(), rng);
                currentQuestion = 0;
            }

            state = nextState; fadeOut = false; }
            if (fadeAlpha <= 0.f) { fadeAlpha = 0.f; isFading = false; }
            fadeRect.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(fadeAlpha)));
        }

        if (state == GameState::HOME && !titleArrived) {
            float dt = titleClock.restart().asSeconds();
            auto pos = titleLOGIQ.getPosition();
            pos.y += titleSpeed * dt;
            if (pos.y >= titleTargetY) { pos.y = titleTargetY; titleArrived = true; }
            titleLOGIQ.setPosition(pos);
        }

        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        if (!isFading) {

			// START, HELP, CREDITS, BACK BUTTONS HOVER
            animStart.targetScale = (state == GameState::HOME && hitStart.contains(mousePos)) ? 1.1f : 1.f;
            animHelp.targetScale = (state == GameState::HOME && hitHelp.contains(mousePos)) ? 1.1f : 1.f;
            animCredits.targetScale = (state == GameState::HOME && hitCredits.contains(mousePos)) ? 1.1f : 1.f;
            animBack.targetScale = (state != GameState::HOME && hitBack.contains(mousePos)) ? 1.1f : 1.f;

			// TIME SELECT BUTTONS HOVER
            if (state == GameState::TIME_SELECT) {
                anim1min.targetScale = (btn1min.getGlobalBounds().contains(mousePos)) ? 1.1f : 1.f;
                anim2mins.targetScale = (btn2mins.getGlobalBounds().contains(mousePos)) ? 1.1f : 1.f;
                anim3mins.targetScale = (btn3mins.getGlobalBounds().contains(mousePos)) ? 1.1f : 1.f;
            }

			// 1MIN, 2MINS, 3MINS BUTTONS HOVER
            if (state == GameState::QUIZ) {
                for (int i = 0; i < 4; i++)
                    animAnswers[i].targetScale = (answerHitboxes[i].contains(mousePos)) ? 1.1f : 1.f;
            }
            
			// TRY  AGAIN AND EXIT BUTTONS HOVER
            if (state == GameState::COMPLETE) {
                animTryAgain.targetScale = (hitTryAgain.contains(mousePos)) ? 1.1f : 1.f;
                animExit.targetScale = (hitExit.contains(mousePos)) ? 1.1f : 1.f;
            }
        }

		// ANIMATION UPDATES
        float animDT = titleClock.restart().asSeconds();
        animateButton(btnStart, animStart, animDT);
        animateButton(btnHelp, animHelp, animDT);
        animateButton(btnCredits, animCredits, animDT);
        animateButton(btnBack, animBack, animDT);
        animateButton(btn1min, anim1min, animDT);
        animateButton(btn2mins, anim2mins, animDT);
        animateButton(btn3mins, anim3mins, animDT);
        for (int i = 0; i < 4; i++) animateButton(answerBtns[i], animAnswers[i], animDT);

		// ANIMATION OF TRY AGAIN AND EXIT BUTTONS
        if (state == GameState::COMPLETE) {
            animateButton(btnTryAgain, animTryAgain, animDT);
            animateButton(btnExit, animExit, animDT);
        }

		// DRAWING LOGIC 
        window.clear();
        if (state == GameState::HOME) {
            window.draw(home);
            window.draw(titleLOGIQ);
            window.draw(btnStart);
            window.draw(btnCredits);
            window.draw(btnHelp);
        }
        else if (state == GameState::TIME_SELECT) {
            window.draw(chooseBG);
            window.draw(btn1min);
            window.draw(btn2mins);
            window.draw(btn3mins);
            window.draw(btnBack);
        }
        if (state == GameState::QUIZ) {
            window.draw(questionBG);
            window.draw(timerText);
            
  
            if (currentQuestion < quizQuestions.size()) {
                for (int i = 0; i < 4; i++)
                    window.draw(answerBtns[i]);

				// SETTING QUESTION TEXT
                sf::Text qText(lilitaFont);
                qText.setString(quizQuestions[currentQuestion].text);
                qText.setCharacterSize(50);
                qText.setFillColor(sf::Color::White);

                sf::FloatRect bounds = qText.getLocalBounds();
                qText.setOrigin({
                    bounds.position.x + bounds.size.x / 2.f,
                    bounds.position.y
                    });
                qText.setPosition({ 750.f, 100.f });

                window.draw(qText);
                for (int i = 0; i < 4; i++) {
 
                    if (i >= quizQuestions[currentQuestion].answers.size()) continue;

                    sf::Text aText(lilitaFont);
                    aText.setString(quizQuestions[currentQuestion].answers[answerOrder[i]]);
                    aText.setCharacterSize(35);
                    aText.setFillColor(sf::Color::White);

                    sf::FloatRect btnBounds = answerBtns[i].getGlobalBounds();
                    sf::FloatRect textBounds = aText.getLocalBounds();


                    aText.setPosition({
    answerBtns[i].getPosition().x - aText.getLocalBounds().size.x / 1.f,
    answerBtns[i].getPosition().y - aText.getLocalBounds().size.y / 2.f
                        });

                    window.draw(aText);
                }

            }
        }
        
        // DRAW FEEDBACK FULL SCREEN GREEN/RED OVERLAYY
        if (showFeedback) {
            sf::RectangleShape fullScreen(sf::Vector2f(1500.f, 900.f)); 
            fullScreen.setFillColor(feedbackColor);
            fullScreen.setPosition(sf::Vector2f(0.f, 0.f)); 

            window.draw(fullScreen);

			// MOVING TO NEXT QUESTION AFTER FEEDBACK DURATION
            if (feedbackClock.getElapsedTime().asSeconds() > 0.6f) {
                showFeedback = false;
                lastClickedAnswer = -1; 
                inputBlocked = false;
                currentQuestion++; 

                if (currentQuestion >= quizQuestions.size()) {
                    nextState = GameState::COMPLETE;
                    isFading = true;
                    fadeOut = true;
                    fadeClock.restart();
                }
                else {
                    for (int j = 0; j < 4; j++)
                        answerBtns[j].setPosition(answerPositions[j]);
                    std::shuffle(answerOrder.begin(), answerOrder.end(), rng);
                }
            }
        }

		// COMPLETE PAGE LOGIC
        else if (state == GameState::COMPLETE) {
            updateHitbox(hitTryAgain, btnTryAgain);
            updateHitbox(hitExit, btnExit);
            window.draw(sf::Sprite(texComplete));
            window.draw(btnTryAgain); 
            window.draw(btnExit);

            sf::Text completeText(lilitaFont);
            completeText.setString("You scored " + std::to_string(score) + " points");

            completeText.setCharacterSize(45);
            completeText.setFillColor(sf::Color::White);

            auto bounds = completeText.getLocalBounds();
            completeText.setOrigin({
                bounds.position.x + bounds.size.x / 2.f,
                bounds.position.y + bounds.size.y / 2.f
                });
            completeText.setPosition({ 750.f, 434.f }); 

            window.draw(completeText);
        }

        else if (state == GameState::CREDITS) {
            window.draw(creditsPage);
            window.draw(btnBack);
        }
        else if (state == GameState::HELP) {
            window.draw(helpPage);
            window.draw(btnBack);
        }

        if (isFading) window.draw(fadeRect);
        window.display();
    }

    return 0;
}
