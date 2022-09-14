#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint duck_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > duck_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("duck.pnct"));
	duck_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > duck_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("duck.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = duck_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = duck_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

std::default_random_engine gen;
std::uniform_real_distribution<float> distribution(0, 1);

PlayMode::PlayMode() : scene(*duck_scene) {
	int sphere_ctr = 0;
	for (auto& transform : scene.transforms) {
		if (transform.name == "Head") {
			head = &transform;
		} else if (transform.name == "Duck") {
			duck = &transform;
		} else if (transform.name.substr(0, 9) == "Icosphere") {
			spheres[sphere_ctr++] = &transform;
		}
	}
	if (head == nullptr) {
		throw std::runtime_error("Head not found.");
	}
	if (duck == nullptr) {
		throw std::runtime_error("Duck not found.");
	}
	for (int i = 0; i < NUM_SPHERES; i++) {
		if (spheres[i] == nullptr) {
			throw std::runtime_error("Sphere " + std::to_string(i) + " not found.");
		}
	}

	// Initialize initial sphere locations
	{
		float cur = -30;
		for (int i = 0; i < NUM_SPHERES; i++) {
			float scale = distribution(gen) * 0.7f + 0.3f;
			float pos = -20.f + distribution(gen) * 40.f;
			spheres[i]->scale *= glm::vec3(scale, scale, scale);
			spheres[i]->position = glm::vec3(cur, pos, 0);
			cur -= 30;
		}
	}

	head_base_rotation = head->rotation;
	duck_initial_position = duck->position;

	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_r) {
			r.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_r) {
			r.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	if (game_over) {
		if (r.pressed) {
			game_over = false;

			// Reset duck position
			duck->position = duck_initial_position;

			// Initialize initial sphere locations
			{
				float cur = -30;
				for (int i = 0; i < NUM_SPHERES; i++) {
					float scale = distribution(gen) * 0.7f + 0.3f;
					float pos = -20.f + distribution(gen) * 40.f;
					spheres[i]->scale *= glm::vec3(scale, scale, scale);
					spheres[i]->position = glm::vec3(cur, pos, 0);
					cur -= 30;
				}
			}
		}
		return;
	}

	// wobble from starter code used to raise up/down duck's head
	wobble += elapsed / 6.f;
	wobble -= std::floor(wobble);
	head->rotation = head_base_rotation * glm::angleAxis(
		glm::radians(7 * std::sin(wobble * 2.0f * float(M_PI))),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);

	float head_rot = elapsed * 500;
	float strafe = space.pressed ? elapsed * 30 : elapsed * 10;
	if (left.pressed) {
		head_degrees += head_rot;
		head_degrees = std::min(head_degrees, 50.f);
		strafe = std::min(strafe, duck->position.y + 18);
		duck->position -= glm::vec3(0.f, strafe, 0.f);
	}
	if (right.pressed) {
		head_degrees -= head_rot;
		head_degrees = std::max(head_degrees, -50.f);
		strafe = std::min(strafe, 19.5f - duck->position.y);
		duck->position += glm::vec3(0.f, strafe, 0.f);
	}
	if (!left.pressed && !right.pressed) {
		if (head_degrees > 0) {
			head_degrees = std::max(0.f, head_degrees - head_rot);
		} else {
			head_degrees = std::min(0.f, head_degrees + head_rot);
		}
	}
	head->rotation *= glm::angleAxis(
		glm::radians(head_degrees),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);

	//Move obstacles closer
	{
		float forward_amt = elapsed * 50;
		for (int i = 0; i < NUM_SPHERES; i++) {
			spheres[i]->position += glm::vec3(forward_amt, 0.f, 0.f);
			if (spheres[i]->position.x > 30) {
				spheres[i]->position.x -= NUM_SPHERES * 30;
				spheres[i]->position.y = -20.f + distribution(gen) * 40.f;
			}
		}
	}

	//Check for collisions
	{
		float closest = 999999;
		for (int i = 0; i < NUM_SPHERES; i++) {
			float val = glm::distance(spheres[i]->position, duck->position) - spheres[i]->scale.x;
			closest = std::min(closest, val);
		}
		if (closest < 1) {
			std::cout << "GAME OVER!" << std::endl;
			game_over = true;
		}
	}
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	if (game_over) {
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Game Over! Press R to restart.",
			glm::vec3(-aspect + 0.2 * H, -0.5f + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Game Over! Press R to restart.",
			glm::vec3(-aspect + 0.2 * H + ofs, -0.5f + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}
