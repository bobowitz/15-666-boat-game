#include "BoatMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"
#include "data_path.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <sstream>
#include <iomanip>

BoatMode::BoatMode()
	: boat(glm::vec2(0.5f * RIVER_WIDTH - 12.0f, START_Y), glm::vec2(20.0f, 34.0f), glm::vec2(24.0f, 36.0f), glm::vec2(0.0f, 0.0f), 0.0f),
	  music(data_path("music.wav"))
	{

	srand((int) time(NULL));

	generateRiver(RIVERBANK_BUFFER_LENGTH);

	Sound::loop(music, 0.0f, 1.0f);

	//----- allocate OpenGL resources -----
	{ //vertex buffer:
		glGenBuffers(1, &vertex_buffer);
		//for now, buffer will be un-filled.

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //vertex array mapping buffer for color_texture_program:
		//ask OpenGL to fill vertex_buffer_for_color_texture_program with the name of an unused vertex array object:
		glGenVertexArrays(1, &vertex_buffer_for_color_texture_program);

		//set vertex_buffer_for_color_texture_program as the current vertex array object:
		glBindVertexArray(vertex_buffer_for_color_texture_program);

		//set vertex_buffer as the source of glVertexAttribPointer() commands:
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		//set up the vertex array object to describe arrays of BoatMode::Vertex:
		glVertexAttribPointer(
			color_texture_program.Position_vec4, //attribute
			3, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 0 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Position_vec4);
		//[Note that it is okay to bind a vec3 input to a vec4 attribute -- the w component will be filled with 1.0 automatically]

		glVertexAttribPointer(
			color_texture_program.Color_vec4, //attribute
			4, //size
			GL_UNSIGNED_BYTE, //type
			GL_TRUE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 //offset
		);
		glEnableVertexAttribArray(color_texture_program.Color_vec4);

		glVertexAttribPointer(
			color_texture_program.TexCoord_vec2, //attribute
			2, //size
			GL_FLOAT, //type
			GL_FALSE, //normalized
			sizeof(Vertex), //stride
			(GLbyte *)0 + 4*3 + 4*1 //offset
		);
		glEnableVertexAttribArray(color_texture_program.TexCoord_vec2);

		//done referring to vertex_buffer, so unbind it:
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//done setting up vertex array object, so unbind it:
		glBindVertexArray(0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //solid white texture:
		//ask OpenGL to fill white_tex with the name of an unused texture object:
		glGenTextures(1, &white_tex);

		//bind that texture object as a GL_TEXTURE_2D-type texture:
		glBindTexture(GL_TEXTURE_2D, white_tex);

		//upload a 1x1 image of solid white to the texture:
		glm::uvec2 size = glm::uvec2(1,1);
		std::vector< glm::u8vec4 > data(size.x*size.y, glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		//set filtering and wrapping parameters:
		//(it's a bit silly to mipmap a 1x1 texture, but I'm doing it because you may want to use this code to load different sizes of texture)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		//since texture uses a mipmap and we haven't uploaded one, instruct opengl to make one for us:
		glGenerateMipmap(GL_TEXTURE_2D);

		//Okay, texture uploaded, can unbind it:
		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS(); //PARANOIA: print out any OpenGL errors that may have happened
	}

	{ //load tileset texture:
		std::vector< glm::u8vec4 > data;
		glm::uvec2 size(0, 0);
		load_png(data_path("boat.png"), &size, &data, UpperLeftOrigin);
		tileset_size = size;
		tileset_tiles = glm::vec2(tileset_size.x / tilesize.x, tileset_size.y / tilesize.y);

		glGenTextures(1, &tileset_tex);

		glBindTexture(GL_TEXTURE_2D, tileset_tex);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glBindTexture(GL_TEXTURE_2D, 0);

		GL_ERRORS();
	}
}

BoatMode::~BoatMode() {

	//----- free OpenGL resources -----
	glDeleteBuffers(1, &vertex_buffer);
	vertex_buffer = 0;

	glDeleteVertexArrays(1, &vertex_buffer_for_color_texture_program);
	vertex_buffer_for_color_texture_program = 0;

	glDeleteTextures(1, &white_tex);
	white_tex = 0;
}

bool BoatMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	return false;
}

void BoatMode::update(float elapsed) {
	camera_speed = CAMERA_START_SPEED + 0.4f * (0.4f * score);
	if (camera_speed > MAX_CAMERA_SPEED) camera_speed = MAX_CAMERA_SPEED;
	if (!game_over && camera_started) {
		if (boat.position.y - camera.y <= CAMERA_ACCEL_ZONE) camera.y += -camera_speed * elapsed + 50.0f * (boat.position.y - camera.y - CAMERA_ACCEL_ZONE) * elapsed * elapsed;
		else camera.y -= camera_speed * elapsed;
	}

	boxes.remove_if([this](const Box &box) -> bool { return box.position.y - camera.y > RIVER_HEIGHT; });

	if (riverbank[riverbank_last].position_left.y - camera.y >= 0) {
		generateRiver(RIVERBANK_GENERATE);
	}

	elapsed_time += elapsed;
	ripple_frame += ripple_frames * elapsed / BOB_TIME;
	while (ripple_frame >= ripple_frames) ripple_frame -= ripple_frames;

	glm::vec2 acceleration(0.0f, 0.0f);

	const Uint8 *state = SDL_GetKeyboardState(NULL);

	if (!game_over) {
		if (state[SDL_SCANCODE_LEFT]) {
			boat.rotation += ROTATION_SPEED * elapsed;
		} if (state[SDL_SCANCODE_RIGHT]) {
			boat.rotation -= ROTATION_SPEED * elapsed;
		}
	
		if (state[SDL_SCANCODE_UP]) {
			acceleration = glm::vec2(-glm::sin(boat.rotation) * ACCEL_SPEED, -glm::cos(boat.rotation) * ACCEL_SPEED);
		} else if (state[SDL_SCANCODE_DOWN]) {
			acceleration = glm::vec2(glm::sin(boat.rotation) * ACCEL_SPEED, glm::cos(boat.rotation) * ACCEL_SPEED);
		}
		else acceleration += -boat.velocity * 100.0f * elapsed;

		boat.velocity += acceleration * elapsed;
		if (glm::length(boat.velocity) > MOVE_SPEED) {
			boat.velocity = MOVE_SPEED * (boat.velocity / glm::length(boat.velocity));
		}

		auto collision = [](Box b1, Boat b2) -> bool {
			if (b1.position.x >= b2.position.x + b2.size.x ||
				b1.position.x <= b2.position.x - b1.size.x ||
				b1.position.y >= b2.position.y + b2.size.y ||
				b1.position.y <= b2.position.y - b1.size.y) return false;
			return true;
		};
		auto collision_bomb = [](Bomb b1, Boat b2) -> bool {
			if (b1.position.x >= b2.position.x + b2.size.x ||
				b1.position.x <= b2.position.x - b1.size.x ||
				b1.position.y >= b2.position.y + b2.size.y ||
				b1.position.y <= b2.position.y - b1.size.y) return false;
			return true;
		};
		auto collision_bank = [](RiverbankPoint b1, Boat b2) -> bool {
			if (b1.position_left.y >= b2.position.y && b1.position_left.y < b2.position.y + b2.size.y) {
				if (b2.position.x >= b1.position_left.x && b2.position.x + b2.size.x <= b1.position_right.x) return false;
				else return true;
			}
			return false;
		};

		boat.position.x += boat.velocity.x * elapsed + acceleration.x * elapsed * elapsed * 0.5f;
		for (Box box: boxes) {
			if (collision(box, boat)) {
				if (boat.velocity.x > 0) {
					boat.position.x = box.position.x - boat.size.x;
				}
				else {
					boat.position.x = box.position.x + box.size.x;
				}
				boat.velocity.x = 0;
			}
		}
		for (Bomb bomb: bombs) {
			if (collision_bomb(bomb, boat)) {
				game_over = true;
			}
		}
		if (boat.position.x + boat.size.x >= RIVER_WIDTH) boat.position.x = RIVER_WIDTH - boat.size.x;
		if (boat.position.x <= 0) boat.position.x = 0;

		boat.position.y += boat.velocity.y * elapsed + acceleration.y * elapsed * elapsed * 0.5f;
		for (Box box: boxes) {
			if (collision(box, boat)) {
				if (boat.velocity.y > 0) {
					boat.position.y = box.position.y - boat.size.y;
				}
				else {
					boat.position.y = box.position.y + box.size.y;
				}
				boat.velocity.y = 0;
			}
		}
		for (Bomb bomb: bombs) {
			if (collision_bomb(bomb, boat)) {
				game_over = true;
			}
		}
		if (!camera_started && boat.position.y <= RIVER_HEIGHT * 0.5f) camera_started = true;
		if (boat.position.y - camera.y >= RIVER_HEIGHT + 96.0f) {
			game_over = true;
		}
		if (boat.position.y - camera.y <= 0) boat.position.y = camera.y;

		score = glm::max(score, (START_Y - boat.position.y) / 24.0f);

		if (normal_river_width > MIN_NORMAL_RIVER_WIDTH) normal_river_width = START_NORMAL_RIVER_WIDTH - 1.5f * (0.4f * score);
		else normal_river_width = MIN_NORMAL_RIVER_WIDTH;

		for (int i = 0; i < RIVERBANK_BUFFER_LENGTH; i++) {
			if (collision_bank(riverbank[i], boat)) {
				game_over = true;
				break;
			}
		}
	}
	else {
		if (state[SDL_SCANCODE_SPACE]) {
			reset_level();
		}
	}
}

void BoatMode::reset_level() {
	score = 0.0f;

	boat.rotation = 0.0f;
	boat.position = glm::vec2(0.5f * RIVER_WIDTH - 12.0f, START_Y);
	boat.velocity = glm::vec2(0.0f, 0.0f);

	camera = glm::vec2(0.0f, 0.0f);
	camera_started = false;
	camera_speed = CAMERA_START_SPEED;

	boxes.clear();
	bombs.clear();

	normal_river_width = RIVER_WIDTH;
	riverbank_empty = true;
	generateRiver(RIVERBANK_BUFFER_LENGTH);

	game_over = false;
}

void BoatMode::generateRiver(int num_samples) {
	int pixels_since_obj = 1000;
	int min_pixels_since_obj = 144;
	for (int i = 0; i <= num_samples; i++) {
		pixels_since_obj++;

		RiverbankPoint current;
		if (riverbank_empty) {
			current.position_left = glm::vec2(0, RIVER_HEIGHT);
			current.momentum_left = 1;
			current.position_right = glm::vec2(RIVER_WIDTH - 1, RIVER_HEIGHT);
			current.momentum_right = -1;
		}
		else {
			RiverbankPoint prev = riverbank[riverbank_last];
			current = prev;
			current.position_left.x += current.momentum_left;
			current.position_left.y--;
			current.position_right.x += current.momentum_right;
			current.position_right.y--;

			// mutate
			//current.momentum_left += 0.005f * (0.01f * (rand() % 100) - 1.0f);
			//current.momentum_right -= 0.005f * (0.01f * (rand() % 100) - 1.0f);

			if (current.position_left.x < 0) {
				current.position_left.x = 0;
				current.momentum_left *= -1;
			}
			if (current.position_right.x >= RIVER_WIDTH) {
				current.position_right.x = (float) RIVER_WIDTH - 1.0f;
				current.momentum_right *= -1;
			}

			float gap = current.position_right.x - current.position_left.x;

			if (gap <= MIN_RIVER_WIDTH) {
				if (current.momentum_left > 0 && current.momentum_right < 0) {
					if (glm::abs(current.momentum_left) > glm::abs(current.momentum_right)) current.momentum_left *= -1;
					else current.momentum_right *= -1;
				} else if (current.momentum_left > 0) current.momentum_left *= -1;
				else if (current.momentum_right < 0) current.momentum_right *= 1;
			} else if (gap <= normal_river_width) {
				if (rand() % 100 >= 95) {
					if (current.momentum_left > 0 && current.momentum_right < 0) {
						if (rand() % 2 == 0) current.momentum_left *= -1;
						else current.momentum_right *= -1;
					} else if (current.momentum_left > 0) current.momentum_left *= -1;
					else if (current.momentum_right < 0) current.momentum_right *= -1;
				}

				if (rand() % 10 >= 2 && pixels_since_obj > min_pixels_since_obj && current.position_left.y - camera.y + 48.0f < 0.0f) {
					if (score >= 150.0f && rand() % 5 == 1) {
						int minx = (int) current.position_left.x + 12;
						int maxx = (int) current.position_right.x - 24 - 12;
						int x = (rand() % (maxx - minx)) + minx;
						bombs.push_back(Bomb(glm::vec2(x, current.position_left.y), glm::vec2(12.0f, 1.0f), 0.1f * (rand() % 10)));
						pixels_since_obj = 0;
					} else {
						int minx = (int) current.position_left.x + 12;
						int maxx = (int) current.position_right.x - 24 - 12;
						int x = (rand() % (maxx - minx)) + minx;
						boxes.push_back(Box(glm::vec2(x, current.position_left.y), glm::vec2(24.0f, 24.0f), 0.1f * (rand() % 10)));
						pixels_since_obj = 0;
					}
				}
			}
		}

		riverbank_last = (riverbank_last + 1) % RIVERBANK_BUFFER_LENGTH;
		riverbank[riverbank_last] = current; // add to the front of the ring buffer

		riverbank_empty = false;
	}
}

void BoatMode::drawTexture(std::vector< Vertex > &vertices, glm::vec2 pos, glm::vec2 size, glm::vec2 tilepos, glm::vec2 tilesize, glm::u8vec4 color, float rotation) {

	glm::mat4 rotate_around_origin_mat = glm::mat4(
		glm::vec4(glm::cos(rotation), -glm::sin(rotation), 0.0f, 0.0f),
		glm::vec4(glm::sin(rotation), glm::cos(rotation), 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
	);

	glm::vec2 rotation_center_2d = pos + 0.5f * size;

	glm::mat4 translate_to_origin_mat = glm::mat4(
		glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(-rotation_center_2d.x, -rotation_center_2d.y, 0.0f, 1.0f)
	);

	glm::mat4 translate_to_center_mat = glm::mat4(
		glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(rotation_center_2d.x, rotation_center_2d.y, 0.0f, 1.0f)
	);

	glm::mat4 rotate_around_center_mat = translate_to_center_mat * rotate_around_origin_mat * translate_to_origin_mat;

	//inline helper function for textured rectangle drawing:
	auto draw_tex_rectangle = [&vertices, &rotate_around_center_mat](glm::vec2 const &pos, glm::vec2 const &size, glm::vec2 const &tilepos, glm::vec2 const &tilesize, glm::u8vec4 const &color) {
		// bot_left will be top left on screen after clip transformation
		glm::vec4 bot_left = rotate_around_center_mat * glm::vec4(pos.x, pos.y, 0.0f, 1.0f);
		glm::vec4 bot_right = rotate_around_center_mat * glm::vec4(pos.x+size.x, pos.y, 0.0f, 1.0f);
		glm::vec4 top_right = rotate_around_center_mat * glm::vec4(pos.x+size.x, pos.y+size.y, 0.0f, 1.0f);
		glm::vec4 top_left = rotate_around_center_mat * glm::vec4(pos.x, pos.y+size.y, 0.0f, 1.0f);

		//draw rectangle as two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(bot_left), color, glm::vec2(tilepos.x, tilepos.y));
		vertices.emplace_back(glm::vec3(bot_right), color, glm::vec2(tilepos.x+tilesize.x, tilepos.y));
		vertices.emplace_back(glm::vec3(top_right), color, glm::vec2(tilepos.x+tilesize.x, tilepos.y+tilesize.y));

		vertices.emplace_back(glm::vec3(bot_left), color, glm::vec2(tilepos.x, tilepos.y));
		vertices.emplace_back(glm::vec3(top_right), color, glm::vec2(tilepos.x+tilesize.x, tilepos.y+tilesize.y));
		vertices.emplace_back(glm::vec3(top_left), color, glm::vec2(tilepos.x, tilepos.y+tilesize.y));
	};

	draw_tex_rectangle(pos, size, tilepos, tilesize, color);
}

void BoatMode::drawText(std::vector< Vertex > &vertices, std::string text, glm::vec2 pos, float scale, glm::u8vec4 color) {
	std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789. ";
	const glm::vec2 CHAR_SIZE(11.0f, 14.0f);
	const glm::vec2 CHAR_OFFSET(12.0f, 0.0f);
	const glm::vec2 CHAR_TILESET_SIZE(11.0f / tileset_size.x, 14.0f / tileset_size.y);
	const glm::vec2 CHAR_TILESET_OFFSET(11.0f / tileset_size.x, 0.0f);
	const glm::vec2 CHAR_TILESET_ORIGIN(0.0f, 396.0f / tileset_size.y);

	glm::vec2 next_char_pos = pos;
	for (char const c: text) {
		for (int i = 0; i < alphabet.size(); i++) {
			if (alphabet[i] == c) {
				drawTexture(vertices, next_char_pos, CHAR_SIZE * scale, CHAR_TILESET_ORIGIN + ((float) i) * CHAR_TILESET_OFFSET, CHAR_TILESET_SIZE, color, 0.0f);
				next_char_pos += CHAR_OFFSET * scale;
				break;
			}
		}
	}
}

void BoatMode::drawBoatRipples(std::vector< Vertex > &vertices) {
	const int LAYERS = 36;
	const float LAYER_OFFSET = 1.0f;
	const int BOAT_TILES_X = 12;
	const int BOAT_TILES_Y = 3;
	const int UNDERWATER_LAYER = 7;
	for (int i = 0; i <= UNDERWATER_LAYER; i++) {
		float tilesetX = (i / BOAT_TILES_Y) * (1.0f / tileset_tiles.x);
		float tilesetY = (i % BOAT_TILES_Y) * (1.0f / tileset_tiles.y);

		glm::vec2 bob = glm::vec2(0.0f, 1.0f * glm::sin(5.0f + elapsed_time * 2.0f * glm::pi<float>() / BOB_TIME));
		glm::vec2 layer_offset = glm::vec2(0.0f, i * -LAYER_OFFSET);

		drawTexture(
			vertices,
			boat.position + bob + layer_offset - camera + 0.5f * (boat.size - boat.drawsize),
			boat.drawsize,
			glm::vec2(tilesetX, tilesetY),
			glm::vec2(1.0f / tileset_tiles.x, 1.0f / tileset_tiles.y),
			glm::u8vec4(255, 255, 255, 255),
			boat.rotation
		);

		if (i == 7) {
			glm::vec2 frameloc;
			if ((int) ripple_frame == 0) frameloc = glm::vec2(4.0f / tileset_tiles.x, 3.0f / tileset_tiles.y);
			else if ((int) ripple_frame == 1) frameloc = glm::vec2(6.0f / tileset_tiles.x, 3.0f / tileset_tiles.y);
			else if ((int) ripple_frame == 2) frameloc = glm::vec2(8.0f / tileset_tiles.x, 3.0f / tileset_tiles.y);
			else if ((int) ripple_frame == 3) frameloc = glm::vec2(10.0f / tileset_tiles.x, 3.0f / tileset_tiles.y);
			else if ((int) ripple_frame == 4) frameloc = glm::vec2(4.0f / tileset_tiles.x, 5.0f / tileset_tiles.y);
			else if ((int) ripple_frame == 5) frameloc = glm::vec2(6.0f / tileset_tiles.x, 5.0f / tileset_tiles.y);
			else if ((int) ripple_frame == 6) frameloc = glm::vec2(8.0f / tileset_tiles.x, 5.0f / tileset_tiles.y);
			else frameloc = glm::vec2(10.0f / tileset_tiles.x, 5.0f / tileset_tiles.y);
			drawTexture(
				vertices,
				boat.position + layer_offset + glm::vec2(-12.0f, -18.0f) - camera + 0.5f * (boat.size - boat.drawsize),
				glm::vec2(48.0f, 72.0f),
				frameloc,
				glm::vec2(2.0f / tileset_tiles.x, 2.0f / tileset_tiles.y),
				glm::u8vec4(255, 255, 255, 255),
				boat.rotation
			);
		}
	}
}

void BoatMode::drawBoxRipples(std::vector< Vertex > &vertices) {
	for (Box box: boxes) {

		// draw underwater portion
		glm::vec2 bob = glm::vec2(0.0f, glm::sin(box.bob_offset + elapsed_time * 2.0f * glm::pi<float>() / BOB_TIME));

		drawTexture(
			vertices,
			box.position + glm::vec2(0.0f, 24.0f) + bob - camera,
			glm::vec2(24.0f, 36.0f),
			glm::vec2(0.0f, 4.0f / tileset_tiles.y),
			glm::vec2(1.0f / tileset_tiles.x, 1.0f / tileset_tiles.y),
			glm::u8vec4(255, 255, 255, 255),
			0.0f
		);

		// draw ripples
		glm::vec2 frameloc;
		float box_ripple_frame = ripple_frame + box.bob_offset * ripple_frames / (2.0f * glm::pi<float>());
		while (box_ripple_frame > ripple_frames) box_ripple_frame -= ripple_frames;
		if ((int) box_ripple_frame == 0) frameloc = glm::vec2(4.0f / tileset_tiles.x, 7.0f / tileset_tiles.y);
		else if ((int) box_ripple_frame == 1) frameloc = glm::vec2(6.0f / tileset_tiles.x, 7.0f / tileset_tiles.y);
		else if ((int) box_ripple_frame == 2) frameloc = glm::vec2(8.0f / tileset_tiles.x, 7.0f / tileset_tiles.y);
		else if ((int) box_ripple_frame == 3) frameloc = glm::vec2(10.0f / tileset_tiles.x, 7.0f / tileset_tiles.y);
		else if ((int) box_ripple_frame == 4) frameloc = glm::vec2(4.0f / tileset_tiles.x, 9.0f / tileset_tiles.y);
		else if ((int) box_ripple_frame == 5) frameloc = glm::vec2(6.0f / tileset_tiles.x, 9.0f / tileset_tiles.y);
		else if ((int) box_ripple_frame == 6) frameloc = glm::vec2(8.0f / tileset_tiles.x, 9.0f / tileset_tiles.y);
		else frameloc = glm::vec2(10.0f / tileset_tiles.x, 9.0f / tileset_tiles.y);
		drawTexture(
			vertices,
			box.position + glm::vec2(-12.0f, -30.0f) - camera,
			glm::vec2(48.0f, 72.0f),
			frameloc,
			glm::vec2(2.0f / tileset_tiles.x, 2.0f / tileset_tiles.y),
			glm::u8vec4(255, 255, 255, 255),
			0.0f
		);
	}
}

void BoatMode::drawBombRipples(std::vector< Vertex > &vertices) {
	for (Bomb bomb: bombs) {
		glm::vec2 frameloc;
		float bomb_ripple_frames = 6.0f;
		float bomb_ripple_frame = (ripple_frame * 6.0f / 8.0f) + bomb.bob_offset * bomb_ripple_frames / (2.0f * glm::pi<float>());
		while (bomb_ripple_frame > bomb_ripple_frames) bomb_ripple_frame -= bomb_ripple_frames;
		if ((int) bomb_ripple_frame == 0) frameloc = glm::vec2(12.0f / tileset_tiles.x, 3.0f / tileset_tiles.y);
		else if ((int) bomb_ripple_frame == 1) frameloc = glm::vec2(14.0f / tileset_tiles.x, 3.0f / tileset_tiles.y);
		else if ((int) bomb_ripple_frame == 2) frameloc = glm::vec2(16.0f / tileset_tiles.x, 3.0f / tileset_tiles.y);
		else if ((int) bomb_ripple_frame == 3) frameloc = glm::vec2(12.0f / tileset_tiles.x, 5.0f / tileset_tiles.y);
		else if ((int) bomb_ripple_frame == 4) frameloc = glm::vec2(14.0f / tileset_tiles.x, 5.0f / tileset_tiles.y);
		else frameloc = glm::vec2(16.0f / tileset_tiles.x, 5.0f / tileset_tiles.y);
		drawTexture(
			vertices,
			bomb.position + glm::vec2(-18.0f, -46.0f) - camera,
			glm::vec2(48.0f, 72.0f),
			frameloc,
			glm::vec2(2.0f / tileset_tiles.x, 2.0f / tileset_tiles.y),
			glm::u8vec4(255, 255, 255, 255),
			0.0f
		);
	}
}

void BoatMode::drawBoat(std::vector< Vertex > &vertices) {
	const int LAYERS = 36;
	const float LAYER_OFFSET = 1.0f;
	const int BOAT_TILES_X = 12;
	const int BOAT_TILES_Y = 3;
	const int UNDERWATER_LAYER = 7;
	for (int i = UNDERWATER_LAYER + 1; i < LAYERS; i++) {
		float tilesetX = (i / BOAT_TILES_Y) * (1.0f / tileset_tiles.x);
		float tilesetY = (i % BOAT_TILES_Y) * (1.0f / tileset_tiles.y);

		glm::vec2 bob = glm::vec2(0.0f, 1.0f * glm::sin(5.0f + elapsed_time * 2.0f * glm::pi<float>() / BOB_TIME));
		glm::vec2 layer_offset = glm::vec2(0.0f, i * -LAYER_OFFSET);

		drawTexture(
			vertices,
			boat.position + bob + layer_offset - camera + 0.5f * (boat.size - boat.drawsize),
			boat.drawsize,
			glm::vec2(tilesetX, tilesetY),
			glm::vec2(1.0f / tileset_tiles.x, 1.0f / tileset_tiles.y),
			glm::u8vec4(255, 255, 255, 255),
			boat.rotation
		);
	}
}

void BoatMode::drawBoxes(std::vector< Vertex > &vertices) {
	for (Box box : boxes) {
		glm::vec2 bob = glm::vec2(0.0f, glm::sin(box.bob_offset + elapsed_time * 2.0f * glm::pi<float>() / BOB_TIME));

		drawTexture(
			vertices,
			box.position + glm::vec2(0.0f, -12.0f) + bob - camera,
			glm::vec2(24.0f, 36.0f),
			glm::vec2(0.0f, 3.0f / tileset_tiles.y),
			glm::vec2(1.0f / tileset_tiles.x, 1.0f / tileset_tiles.y),
			glm::u8vec4(255, 255, 255, 255),
			0.0f
		);
	}
}

void BoatMode::drawBombs(std::vector< Vertex > &vertices) {
	for (Bomb bomb : bombs) {
		glm::vec2 bob = glm::vec2(0.0f, glm::sin(bomb.bob_offset + elapsed_time * 2.0f * glm::pi<float>() / BOB_TIME));

		float frame = (float) (int) (2.0f + 0.5f * (glm::sin(bomb.bob_offset + elapsed_time * 12.0f)));
		drawTexture(
			vertices,
			bomb.position + glm::vec2(-6.0f, -35.0f) + bob - camera,
			glm::vec2(24.0f, 36.0f),
			glm::vec2(frame / tileset_tiles.x, 3.0f / tileset_tiles.y),
			glm::vec2(1.0f / tileset_tiles.x, 1.0f / tileset_tiles.y),
			glm::u8vec4(255, 255, 255, 255),
			0.0f
		);
	}
}

void BoatMode::draw(glm::uvec2 const &drawable_size) {
	//some nice colors from the course web page:
	#define HEX_TO_U8VEC4( HX ) (glm::u8vec4( (HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX) & 0xff ))
	const glm::u8vec4 bg_color = HEX_TO_U8VEC4(0x5e82b6ff);
	const glm::u8vec4 fg_color = HEX_TO_U8VEC4(0x305f22ff);
	const glm::u8vec4 shadow_color = HEX_TO_U8VEC4(0x604d29ff);
	#undef HEX_TO_U8VEC4

	//other useful drawing constants:
	const float wall_radius = 0.05f;
	const float shadow_offset = 0.07f;
	const float padding = 0.14f; //padding between outside of walls and edge of window

	//---- compute vertices to draw ----

	//vertices will be accumulated into this list and then uploaded+drawn at the end of this function:
	
	std::vector< Vertex > vertices;

	//inline helper function for rectangle drawing:
	auto draw_rectangle = [&vertices](glm::vec2 const &center, glm::vec2 const &radius, glm::u8vec4 const &color) {
		//draw rectangle as two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y-radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x+radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x-radius.x, center.y+radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};

	drawBoatRipples(vertices);
	drawBoxRipples(vertices);
	drawBombRipples(vertices);
	drawBoxes(vertices);
	drawBombs(vertices);
	

	for (int i = 0; i < RIVERBANK_BUFFER_LENGTH; i++) {
		RiverbankPoint p = riverbank[i];
		glm::vec2 offset(0.0f, 24.0f);
		drawTexture(vertices, glm::vec2(0, p.position_left.y - 1) - camera - offset, glm::vec2(p.position_left.x, 1) + offset, glm::vec2(2.0f / tileset_size.x, 390.0f / tileset_size.y), 1.0f / tileset_size, glm::u8vec4(255, 255, 255, 255), 0.0f);
		drawTexture(vertices, glm::vec2(p.position_right.x, p.position_right.y - 1) - camera - offset, glm::vec2(RIVER_WIDTH, 1) + offset, glm::vec2(2.0f / tileset_size.x, 390.0f / tileset_size.y), 1.0f / tileset_size, glm::u8vec4(255, 255, 255, 255), 0.0f);
	}

	for (int i = 0; i < RIVERBANK_BUFFER_LENGTH; i++) {
		RiverbankPoint p = riverbank[i];
		glm::vec2 offset(0.0f, 24.0f);
		drawTexture(vertices, glm::vec2(0, p.position_left.y - 1) - camera - offset, glm::vec2(p.position_left.x, 1), glm::vec2(8.0f / tileset_size.x, 390.0f / tileset_size.y), 1.0f / tileset_size, glm::u8vec4(255, 255, 255, 255), 0.0f);
		drawTexture(vertices, glm::vec2(p.position_right.x, p.position_right.y - 1) - camera - offset, glm::vec2(RIVER_WIDTH, 1), glm::vec2(8.0f / tileset_size.x, 390.0f / tileset_size.y), 1.0f / tileset_size, glm::u8vec4(255, 255, 255, 255), 0.0f);
	}

	drawBoat(vertices);

	if (game_over) {
		drawTexture(vertices, glm::vec2(0, 0), glm::vec2(RIVER_WIDTH, RIVER_HEIGHT), glm::vec2(15.0f / tileset_size.x, 390.0f / tileset_size.y), 1.0f / tileset_size, glm::u8vec4(0, 0, 0, 128), 0.0f);

		{
			std::string game_over_text = "GAME";
			float width = ((float) game_over_text.size()) * 12.0f * 4.0f;
			drawText(vertices, game_over_text, glm::vec2(0.5f * (RIVER_WIDTH - width), 0.5f * RIVER_HEIGHT - 128.0f), 4.0f, glm::u8vec4(255, 0, 0, 255));
		}
		
		{
			std::string game_over_text = "OVER";
			float width = ((float) game_over_text.size()) * 12.0f * 4.0f;
			drawText(vertices, game_over_text, glm::vec2(0.5f * (RIVER_WIDTH - width), 0.5f * RIVER_HEIGHT - 64.0f), 4.0f, glm::u8vec4(255, 0, 0, 255));
		}

		{
			std::stringstream stream;
			stream << "SCORE " << std::fixed << std::setprecision(0) << score << "M";
			std::string score_text = stream.str();
			float width = ((float) score_text.size()) * 24.0f;
			drawText(vertices, score_text, glm::vec2(0.5f * (RIVER_WIDTH - width),  0.5f * RIVER_HEIGHT), 2.0f, glm::u8vec4(255, 255, 255, 255));
		}

		{
			std::string press_space_text = "PRESS SPACE";
			float width = ((float) press_space_text.size()) * 12.0f * 2.0f;
			drawText(vertices, press_space_text, glm::vec2(0.5f * (RIVER_WIDTH - width), RIVER_HEIGHT - 96.0f), 2.0f, glm::u8vec4(255, 255, 255, 255));
		}
		{
			std::string retry_text = "TO RETRY";
			float width = ((float) retry_text.size()) * 12.0f * 2.0f;
			drawText(vertices, retry_text, glm::vec2(0.5f * (RIVER_WIDTH - width), RIVER_HEIGHT - 64.0f), 2.0f, glm::u8vec4(255, 255, 255, 255));
		}
		
	} else {
		std::stringstream stream;
		stream << std::fixed << std::setprecision(0) << score << "M";
		std::string score_text = stream.str();
		float width = ((float) score_text.size()) * 24.0f;
		drawText(vertices, score_text, glm::vec2(0.5f * (RIVER_WIDTH - width), 24.0f), 2.0f, glm::u8vec4(255, 255, 255, 255));
	}

	// boat hitbox
	//drawTexture(vertices, boat.position - camera, boat.size, glm::vec2(8.0f / tileset_size.x, 150.0f / tileset_size.y), 1.0f / tileset_size, glm::u8vec4(255, 0, 0, 255), 0.0f);

	//compute window scale matrix
	glm::mat4 pixels_to_clip = glm::mat4(
		glm::vec4(2.0f / RIVER_WIDTH, 0.0f, 0.0f, 0.0f),
		glm::vec4(0.0f, -2.0f / RIVER_HEIGHT, 0.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 1.0f, 0.0f),
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
	);

	glm::vec4 pixels_clip_offset = glm::vec4(-1.0f, 1.0f, 0.0f, 0.0f);

	//---- actual drawing ----

	//clear the color buffer:
	glClearColor(bg_color.r / 255.0f, bg_color.g / 255.0f, bg_color.b / 255.0f, bg_color.a / 255.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	//use alpha blending:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	glDisable(GL_DEPTH_TEST);

	//upload vertices to vertex_buffer:
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer); //set vertex_buffer as current
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STREAM_DRAW); //upload vertices array
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//set color_texture_program as current program:
	glUseProgram(color_texture_program.program);

	//upload OBJECT_TO_CLIP to the proper uniform location:
	glUniformMatrix4fv(color_texture_program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(pixels_to_clip));
	glUniform4fv(color_texture_program.CLIP_OFFSET_vec4, 1, glm::value_ptr(pixels_clip_offset));

	//use the mapping vertex_buffer_for_color_texture_program to fetch vertex data:
	glBindVertexArray(vertex_buffer_for_color_texture_program);

	//bind the solid white texture to location zero so things will be drawn just with their colors:
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tileset_tex);

	//run the OpenGL pipeline:
	glDrawArrays(GL_TRIANGLES, 0, GLsizei(vertices.size()));

	//unbind the solid white texture:
	glBindTexture(GL_TEXTURE_2D, 0);

	//reset vertex array to none:
	glBindVertexArray(0);

	//reset current program to none:
	glUseProgram(0);
	
	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.
}
