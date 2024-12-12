#include <SFML/Graphics.hpp>
#include <random>
#include "../build/events.hpp"
#include "../build/configuration.hpp"
#include "../build/star.hpp"

//can use unsigned int or just int instead of uint32 but uint32 is more consisten 
// in this case as we do not need any negative values where we use uint32_t + more range

//all the stars are stored in a vector created in this function
//Update: adding a scaling parameter
std::vector<Star> createStar(uint32_t count, float scale)
{
    std::vector<Star>stars;

    // vector_name.reserve(n): Requests that the vector capacity be at least enough to contain 
    // n elements.If n is greater than the current vector capacity, the function causes the 
    // container to reallocate its storage increasing its capacity to n(or greater).
    // In all other cases, the function call does not cause a reallocation and the vector 
    // capacity is not affected.This function has no effect on the vector size and 
    // cannot alter its elements.
    stars.reserve(count);

    //don't use this, doesn't generate random stars in uniform distribution for some reason :/
    //int r = std::rand();
    //creating a robust system for generating random floating-point numbers
    //std::random_device: uniformly-distributed integer random number generator that produces non-deterministic random numbers.
    //std::mt19937: implementation of the Mersenne Twister pseudo-random number generator (PRNG).
    //mt19937 is known for its high quality and long period (2^321937-1)
    //std::uniform_real_distribution<float>:This defines a uniform real distribution object that generates random floating-point numbers in the range [0.0, 1.0).
    std::random_device rd;      //rd ->provides an unpredictable seed to the generator.
    std::mt19937 gen(rd());     //gen ->generates a sequence of pseudo-random numbers based on that seed.
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);  //dis ->ensures the numbers are scaled to fall within the desired range [0.0, 1.0)
    
    //define a star free zone
    sf::Vector2f const window_world_size = conf::window_size_f * conf::near;
    sf::FloatRect const star_free_zone = { - window_world_size * 0.5f, window_world_size };


    //Create randomly distributed stars on the screen
    for (uint32_t i = count ; i--;)
    {
        //after scaling star objects get centered to top left of the screen (origin of the monitor)
        //to fix this generate random position around 0 and translate them to centre of the screen
        float const x = (dis(gen) - 0.5f) * conf::window_size_f.x * scale;   //random x coord of a star
        float const y = (dis(gen) - 0.5f) * conf::window_size_f.y * scale;   //random y coord of a star
        float const z = dis(gen) * (conf::far - conf::near) + conf::near;
        
        //discard any star inside the star free zone
        if (star_free_zone.contains(x, y))
        {
            ++i;
            continue;
        }
        //else add it in the vector
        stars.push_back({ {x,y} ,z}); //first curly braces create the Star object 2nd one puts the x,y value in its vector array    
    }
    //depth ordering
    //stars are on top of one another after depth color 
    //FIX: just sort them, computation isn't to heave as it sorts only once
    //using a lamda function to sort in a descending order
    std::sort(stars.begin(), stars.end(), [](Star const& s_1, Star const& s_2){
            return s_1.z > s_2.z;});

    return stars;   //return the vector
}
//manually drawing the vertices and storing them in the array and then displaying them all at once
//using this method we can now display 1000000 stars smoothly with no frame rate drops

void updateGeometry(uint32_t idx, Star const& s, sf::VertexArray& va)
{
    float const scale = 1.0 / s.z;
    //depth effect - make distant stars darker
    //depth ratio = (z-near)/(far-near)
    float const depth_ratio = (s.z - conf::near) / (conf::far - conf::near);
    //color ratio = 1-depth ratio
    float const color_ratio = 1.0f - depth_ratio;
    //type conversion to uint8 bit cuz rgb values are 8 bits
    auto const c = static_cast<uint8_t>(color_ratio * 255.0f);

    sf::Vector2f const p = s.position * scale;
    float const r = conf::radius * scale; 
    uint32_t const i = 4 * idx;
    //These lines calculate the positions of the four vertices for the star’s rectangle(quad)
    va[i + 0].position = { p.x - r,p.y - r };   //Top-left corner
    va[i + 1].position = { p.x + r,p.y - r };   //Top-right corner
    va[i + 2].position = { p.x + r,p.y + r };   //Bottom-right corner
    va[i + 3].position = { p.x - r,p.y + r };   //Bottom-left corner

    //Sets the color of all four vertices to the calculated brightness(c, c, c), resulting in a grayscale color
    //Closer stars are brighter (c near 255).
    //Farther stars are dimmer (c near 0).
    sf::Color const color{c,c,c};
    va[i + 0].color = color;
    va[i + 1].color = color;
    va[i + 2].color = color;
    va[i + 3].color = color;
}

int main()
{
    auto window = sf::RenderWindow({conf::window_size.x, conf::window_size.y}, "CMake SFML Project",sf::Style::Fullscreen);
    window.setFramerateLimit(conf::max_framerate);
    window.setMouseCursorVisible(false);

    //using sprites to make the stars instead of squares on screen
    sf::Texture texture;
    texture.loadFromFile("C:/Users/safiul/SFMLCode/cmake-sfml-project/build/star.png");
    texture.setSmooth(true);
    texture.generateMipmap();
    
    //scaled with far pont to make stars more distant
    std::vector<Star>stars = createStar(conf::count, conf::far);
    
    //vertexarrays let us manually specify in a large array the characteristics of the vertices that are to be drawn
    sf::VertexArray va{ sf::PrimitiveType::Quads, 4 * conf::count };
   
    //Pre fill texture coords as they will remain constant
    auto const texture_size_f = static_cast<sf::Vector2f>(texture.getSize());
    //do not use, error
    //for (uint32_t idx = conf::count ; idx < 0;idx--)
    for (uint32_t idx = conf::count ; idx--;)
    {
        uint32_t const i = 4 * idx;
        va[i+0].texCoords = {0.0f,0.0f};
        va[i+1].texCoords = { texture_size_f.x,0.0f};
        va[i+2].texCoords = { texture_size_f.x,texture_size_f.y };
        va[i+3].texCoords = {0.0f, texture_size_f.y };
    }   
    
    uint32_t first = 0;     //initializing a variable to find the star that will be the furthest
    while (window.isOpen())
    {
        processEvents(window);
        //Create a fake moving effect
        //substracting the z with the speed*time step to make it framerate invariant
        //start from near then end at far to avoid stars being inside other stars
        for (uint32_t i = conf::count; i--;)
        {
            Star& s = stars[i];
            s.z -= conf::speed * conf::dt;
            //reuse the stars that went behind 'near' value
            //but there is still stars inside other stars
            if (s.z < conf::near)
            {
                //offset the star by the excess travel it made behind near to keep spacing constant
                s.z = conf::far - (conf::near - s.z);
                //This star is now the first we need to draw because it is further away from us
                first = i;
            }
        }


        //Rendering
        window.clear();

        //circle
        sf::CircleShape shape{conf::radius};
        shape.setOrigin(conf::radius, conf::radius);

        for (uint32_t i = 0 ; i < conf::count; ++i)
        {
            uint32_t const idx = (i + first) % conf::count;
            Star const& s = stars[idx];
            updateGeometry(i, s, va);
            //Update:visibility already handled above if condition not required
            //when z value is negative the stars look like they are coming back
            //soln: just dont draw the stars with z value lower than 0
            //if (s.z > conf::near)
            //{
                //we can use the z value as a scaling coefficent
                //scaling coefficient is inversely proportional to the z-coordinate
                //e.g larger the z value means smaller (further away) the star
            //--------------------------------------------------------------------------------------------------------------------------------------
            /*Update: made a function in updateGeom 
            float const scale = 1.0 / s.z;

            shape.setPosition(s.position * scale + conf::window_size_f * 0.5f);
            shape.setScale(scale, scale);

            //depth effect - make distant stars darker
            //depth ratio = (z-near)/(far-near)
            float const depth_ratio = (s.z - conf::near) / (conf::far - conf::near);
            
            //color ratio = 1-depth ratio
            float const color_ratio = 1.0f - depth_ratio;

            //type conversion to uint8 bit cuz rgb values are 8 bit
            auto const c = static_cast<uint8_t>(color_ratio * 255.0);
            shape.setFillColor({ c, c, c });
            
            //draw calls are very costly and lowers framerates, its best to use as few as possible and draw all the stars at once
            window.draw(shape);
            //}
            -----------------------------------------------------------------------------------------------------------------------------------*/
        }
        sf::RenderStates states;
        //to translate all the vertices at once we need a transformation matrix as an arg in the draw function
        //sf::Transform tf;
        //translate it to window size
        states.transform.translate(conf::window_size_f * 0.5f);
        states.texture = &texture;
        window.draw(va, states);

        window.display();
    }
}
