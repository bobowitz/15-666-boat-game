#include "ColorTextureProgram.hpp"

#include "Mode.hpp"
#include "GL.hpp"
#include "load_save_png.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <list>
#include <deque>

/*
 * BoatMode is a game mode that implements a single-player infinite runner boating game
 */

struct BoatMode : Mode {
	BoatMode();
	virtual ~BoatMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	void reset_level();

	//----- constants -----
	static const int RIVER_WIDTH = 312;
	static const int RIVER_HEIGHT = 480;

	const float START_Y = RIVER_HEIGHT - 72.0f;

	const float MAX_CAMERA_SPEED = 150.0f;
	const float CAMERA_START_SPEED = 60.0f;
	const float CAMERA_ACCEL_ZONE = RIVER_HEIGHT * 0.5f;
	
	const float ROTATION_SPEED = 5.0f;
	const float MOVE_SPEED = 180.0f;
	const float ACCEL_SPEED = 400.0f;

	//----- game state -----

	struct Boat {
		Boat(glm::vec2 const &position_, glm::vec2 const &size_, glm::vec2 const &draw_size_, glm::vec2 const &velocity_, float const &rotation_) :
			position(position_), size(size_), drawsize(draw_size_), velocity(velocity_), rotation(rotation_) { }
		glm::vec2 position;
		glm::vec2 size;
		glm::vec2 drawsize;
		glm::vec2 velocity;
		float rotation;
	};

	struct Box {
		Box(glm::vec2 const &position_, glm::vec2 const &size_, float const &bob_offset_) :
			position(position_), size(size_), bob_offset(bob_offset_) { }
		glm::vec2 position;
		glm::vec2 size;
		float bob_offset;
	};

	struct Bomb {
		Bomb(glm::vec2 const &position_, glm::vec2 const &size_, float const &bob_offset_) :
			position(position_), size(size_), bob_offset(bob_offset_) { }
		glm::vec2 position;
		glm::vec2 size;
		float bob_offset;
	};

	struct RiverbankPoint {
		glm::vec2 position_left;
		float momentum_left;
		glm::vec2 position_right;
		float momentum_right;
	};

	Boat boat;
	float ripple_frame = 0.0f;
	float ripple_frames = 8.0f;
	const float BOB_TIME = 1.25f; // period of 1 bob
	float elapsed_time = 0.0f;
	std::list< Box > boxes;
	std::vector< Bomb > bombs;
	float score = 0.0f;
	bool game_over = false;

	static const int MIN_NORMAL_RIVER_WIDTH = 156;
	static const int START_NORMAL_RIVER_WIDTH = RIVER_WIDTH;
	static const int MIN_RIVER_WIDTH = 108;
	float normal_river_width = START_NORMAL_RIVER_WIDTH;
	static const int RIVERBANK_GENERATE = RIVER_HEIGHT - 32;
	static const int RIVERBANK_BUFFER_LENGTH = RIVER_HEIGHT * 2;
	int riverbank_last;
	bool riverbank_empty = true;
	RiverbankPoint riverbank[RIVERBANK_BUFFER_LENGTH]; // river bank ring buffer


	glm::vec2 camera = glm::vec2(0.0f, 0.0f);
	float camera_speed = CAMERA_START_SPEED;
	bool camera_started = false;

	//----- music -----
	Sound::Sample music;

	//----- opengl assets / helpers ------

	//draw functions will work on vectors of vertices, defined as follows:
	struct Vertex {
		Vertex(glm::vec3 const &Position_, glm::u8vec4 const &Color_, glm::vec2 const &TexCoord_) :
			Position(Position_), Color(Color_), TexCoord(TexCoord_) { }
		glm::vec3 Position;
		glm::u8vec4 Color;
		glm::vec2 TexCoord;
	};
	static_assert(sizeof(Vertex) == 4*3 + 1*4 + 4*2, "BoatMode::Vertex should be packed");

	//Shader program that draws transformed, vertices tinted with vertex colors:
	ColorTextureProgram color_texture_program;

	//Buffer used to hold vertex data during drawing:
	GLuint vertex_buffer = 0;

	//Vertex Array Object that maps buffer locations to color_texture_program attribute locations:
	GLuint vertex_buffer_for_color_texture_program = 0;

	//Solid white texture:
	GLuint white_tex = 0;

	//tileset texture:
	GLuint tileset_tex = 0;
	glm::vec2 tilesize = glm::vec2(24.0f, 36.0f);
	glm::vec2 tileset_size;
	glm::vec2 tileset_tiles;

	//matrix that maps from clip coordinates to court-space coordinates:
	glm::mat3x2 clip_to_court = glm::mat3x2(1.0f);
	// computed in draw() as the inverse of OBJECT_TO_CLIP
	// (stored here so that the mouse handling code can use it to position the paddle)

	// river generation
	void generateRiver(int num_samples);

	// helper draw functions
	void drawTexture(std::vector< Vertex > &vertices, glm::vec2 pos, glm::vec2 size, glm::vec2 tilepos, glm::vec2 tilesize, glm::u8vec4 color, float rotation);
	void drawText(std::vector< Vertex > &vertices, std::string text, glm::vec2 pos, float scale, glm::u8vec4 color);
	void drawBoatRipples(std::vector< Vertex > &vertices);
	void drawBoxRipples(std::vector< Vertex > &vertices);
	void drawBombRipples(std::vector< Vertex > &vertices);
	void drawBoat(std::vector< Vertex > &vertices);
	void drawBoxes(std::vector< Vertex > &vertices);
	void drawBombs(std::vector< Vertex > &vertices);
};
