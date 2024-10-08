#include "json_parser.hpp"
#include "particle.hpp"
#include "simulation.hpp"
#include "utils.hpp"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Main.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/Window.hpp>
#include <SFML/Window/WindowStyle.hpp>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <fstream>
#include <ios>
#include <random>
#include <string>
#include <sys/types.h>
#include <vector>

static sf::Vector2f mouse_pos = sf::Vector2f(0.0f, 0.0f);

const float RADIUS = 5.0f;

static std::vector<sf::Text> texts;

static int enabled_colors = 1;
static bool should_spawn_particles = false;
static bool show_debug_ingo = false;

static int fps;

int main(int argc, char *argv[]) {

  if (argc > 1) {
    JsonParser json_parser(argv[1]);
    json_parser.parseFile();
    json_parser.loadInteractions();
  }

  sf::Font font;
  font.loadFromFile("./res/Symbola.otf");

  sf::Text text1;
  sf::Text text2;
  sf::Text text3;
  sf::Text text4;
  sf::Text text5;
  sf::Text text6;

  texts.push_back(text1);
  texts.push_back(text2);
  texts.push_back(text3);
  texts.push_back(text4);
  texts.push_back(text5);
  texts.push_back(text6);

  for (int text_index = 0; text_index < texts.size(); text_index++) {
    sf::Text *text = &texts[text_index];
    text->setFont(font);
    text->setPosition(sf::Vector2f(0.0f, 25.0f * text_index));
  }

  sf::RenderWindow window = sf::RenderWindow(sf::VideoMode(800, 600),
                                             "ParticleSim", sf::Style::Resize);
  /*window.setVerticalSyncEnabled(true);*/
  sf::Clock deltaClock;

  addParticle(5.0f, sf::Vector2f(100.0f, 100.0f), sf::Vector2f(50.0f, 50.0f));

  int spawn_timer = 0;
  bool red = false;

  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        window.close();
      }
      if (event.type == sf::Event::Resized) {
        sf::View view(
            sf::Vector2f((float)event.size.width / 2,
                         (float)event.size.height / 2),
            sf::Vector2f((float)event.size.width, (float)event.size.height));
        window.setView(view);
        setBounds((int)event.size.width, (int)event.size.height);
      }
      if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Button::Right) {
          addParticle(RADIUS,
                      sf::Vector2f((float)event.mouseButton.x,
                                   (float)event.mouseButton.y),
                      sf::Vector2f(50.0f, 50.0f),
                      getParticleCount() % enabled_colors);
        }
        if (event.mouseButton.button == sf::Mouse::Button::Left) {
          selectBall(sf::Vector2f((float)event.mouseButton.x,
                                  (float)event.mouseButton.y));
        }
      }
      if (event.type == sf::Event::MouseButtonReleased) {
        if (event.mouseButton.button == sf::Mouse::Button::Left) {
          unselectBall();
        }
      }
      if (event.type == sf::Event::MouseMoved) {
        mouse_pos.x = (float)event.mouseMove.x;
        mouse_pos.y = (float)event.mouseMove.y;
      }
      if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Key::G) {
          toggleGravity();
        }
        if (event.key.code == sf::Keyboard::Key::Q) {
          toggleQuadTree();
        }
        if (event.key.code == sf::Keyboard::Up) {
          enabled_colors += 1;
          if (enabled_colors > 8) {
            enabled_colors = 1;
          }
        }
        if (event.key.code == sf::Keyboard::Down) {
          enabled_colors -= 1;
          if (enabled_colors < 1) {
            enabled_colors = 8;
          }
        }
        if (event.key.code == sf::Keyboard::S) {
          should_spawn_particles = !should_spawn_particles;
        }
        if (event.key.code == sf::Keyboard::BackSpace) {
          clearParticles();
        }
        if (event.key.code == sf::Keyboard::I) {
          toggleInteractions();
        }
        if (event.key.code == sf::Keyboard::F3) {
          show_debug_ingo = !show_debug_ingo;
        }
      }
    }

    float deltaTime = deltaClock.restart().asSeconds();

    /*printf("%d, %d\n", window.getSize().x, window.getSize().y);*/

    window.clear(sf::Color::Black);

    physicsUpdate(deltaTime, window);

    renderPass(window);

    fps = int(1.0f / deltaTime);

    if (show_debug_ingo) {
      renderText(window, texts);
    }

    window.display();

    sf::Vector2f spawn_location =
        sf::Vector2f((float)window.getSize().x, (float)window.getSize().y);

    spawn_timer = (spawn_timer += 1) % 5;
    if (fps > 60 && spawn_timer == 0 && should_spawn_particles) {
      addParticle(5.0f, spawn_location / 2.0f,
                  sf::Vector2f(50.0f + (getParticleCount() % enabled_colors),
                               50.0f + (getParticleCount() % enabled_colors)),
                  getParticleCount() % enabled_colors);
    }
    if (fps < 100) {
      changeSubstepCount(-1);
    }
    if (fps > 100) {
      changeSubstepCount(1);
    }
  }

  return 0;
}

sf::Vector2f getMousePos() { return mouse_pos; }

void renderText(sf::RenderWindow &window, std::vector<sf::Text> &texts) {
  std::string fps_text = "FPS : ";
  fps_text.append(std::to_string(fps));
  texts[0].setString(fps_text);
  window.draw(texts[0]);

  std::string particle_count_text = "Particles : ";
  particle_count_text.append(std::to_string(getParticleCount()));
  texts[1].setString(particle_count_text);
  window.draw(texts[1]);

  std::string enabled_colors_text = "Color Count : ";
  enabled_colors_text.append(std::to_string(enabled_colors));
  texts[2].setString(enabled_colors_text);
  window.draw(texts[2]);

  std::string spawning_text = "Particle Spawner : ";
  if (should_spawn_particles) {
    spawning_text.append("On");
  } else {
    spawning_text.append("Off");
  }
  texts[3].setString(spawning_text);
  window.draw(texts[3]);

  std::string interactions_text = "Interactions : ";
  if (getInteractionsState()) {
    interactions_text.append("On");
  } else {
    interactions_text.append("Off");
  }
  texts[4].setString(interactions_text);
  window.draw(texts[4]);

  std::string substep_text = "Substeps : ";
  substep_text.append(std::to_string(getSubstepCount()));
  texts[5].setString(substep_text);
  window.draw(texts[5]);
}
