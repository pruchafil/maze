#include <iostream>
#include <set>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <math.h>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <stop_token>

struct cell {
	int32_t x, y;
	// g = from start, h = to finish
	float g, h;

	std::vector<cell> path_back;

	float get_f() const {
		return g + h;
	}

	bool operator<(const cell& _Cell) const {
		return get_f() < _Cell.get_f();
	}
};

std::mutex mu;

int32_t width, height;
const uint16_t size = 10;
std::vector<std::string> maze;
std::vector<cell> path;

void calc_h_g(cell& _Cell) {
	_Cell.g = std::sqrtf(static_cast<float>((_Cell.x * _Cell.x) + (_Cell.y * _Cell.y)));
	const float tmpx = width - 1.0f - _Cell.x;
	const float tmpy = height - 1.0f - _Cell.y;
	_Cell.h = std::sqrtf((tmpx * tmpx) + (tmpy * tmpy));
}

void look_around(
	const cell& _Cell,
	std::vector<cell>& _Closed,
	std::set<cell>& _Set,
	const int32_t _X,
	const int32_t _Y)
{
	std::lock_guard<std::mutex> lg(mu);

	if
	(
		_X >= 0
		&& _X < width
		&& _Y >= 0
		&& _Y < height
		&& maze[_Y][_X] != '#'
		&& std::find_if(_Closed.cbegin(), _Closed.cend(), [&](const auto& itr)
		{
			return itr.x == _X && itr.y == _Y;
		}) == _Closed.cend()
	)
	{
		std::vector<cell> vec = _Cell.path_back;
		vec.emplace_back(_Cell.x, _Cell.y);

		path = vec;

		cell c { _X, _Y, 0.0f, 0.0f, vec };
		calc_h_g(c);
		_Set.insert(c);
		_Closed.push_back(c);
	}
}

void a_star(std::stop_token _Token) {
	std::set<cell> set;
	std::vector<cell> closed;

	cell start{ 0, 0, 0.0f, 0.0f };
	set.insert(start);
	closed.push_back(start);

	while (!set.empty() && !_Token.stop_requested()) {
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(10ms);

		cell c = *(set.begin());
		set.erase(set.begin());

		look_around(c, closed, set, c.x + 1, c.y    );
		look_around(c, closed, set, c.x - 1, c.y    );
		look_around(c, closed, set, c.x,     c.y + 1);
		look_around(c, closed, set, c.x,     c.y - 1);
		//look_around(c, closed, set, c.x + 1, c.y + 1);
		//look_around(c, closed, set, c.x + 1, c.y - 1);
		//look_around(c, closed, set, c.x - 1, c.y + 1);
		//look_around(c, closed, set, c.x - 1, c.y - 1);

		if (c.x == width - 1 && c.y == height - 1) {
			std::cout << "it's done";
			break;
		}
	}
}

int main() {
	std::cout << "height width ";
	std::cin >> height >> width;

	maze.reserve(height);

	for (size_t i = 0; i < height; i++) {
		std::cout << i + 1 << ": ";
		std::string s;
		std::cin >> s;
		maze.push_back(s);
	}

	sf::RenderWindow window(sf::VideoMode(size * width, size * height), "maze");

	std::stop_token token;
	std::jthread th(a_star, token);

	while (window.isOpen()) {
		sf::Event event;

		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				window.close();
			}
		}

		window.clear();

		for (size_t i = 0; i < height; i++) {
			for (size_t j = 0; j < width; j++) {
				sf::RectangleShape rs(sf::Vector2<float>(size, size));
				const char& c = maze[i][j];

				std::lock_guard<std::mutex> lg(mu);

				if (std::find_if(path.cbegin(), path.cend(), [&](const auto& itr) {
					return i == itr.y && j == itr.x;
					}) != path.cend())
				{
					rs.setFillColor(sf::Color::Green);
				} else {
					rs.setFillColor(c == '#' ? sf::Color::White : sf::Color::Black);
				}

				rs.setPosition(static_cast<float>(j * size), static_cast<float>(i * size));
				window.draw(rs);
			}
		}

		window.display();
	}

	th.request_stop();
	th.join();

	return 0;
}