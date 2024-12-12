#pragma once

namespace conf 
{
	//window config
	sf::Vector2u const window_size = { 1920,1080 };
	sf::Vector2f const window_size_f = static_cast<sf::Vector2f>(window_size);

	uint32_t const max_framerate = 144;
	float const dt = 1.0 / static_cast<float>(max_framerate);

	//star config
	uint32_t count = 10000;
	float const radius = 30.0f;
	float const far = 10.0f;	//furthest a star can be on screen
	float const near = 0.1f;	//closest a star can be on screen
	float const speed = 0.5f;	//speed of the simulated stars
}