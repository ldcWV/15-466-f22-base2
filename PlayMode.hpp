#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t pressed = 0;
	} left, right, r, space;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	// Duck transforms
	Scene::Transform* duck = nullptr;
	glm::vec3 duck_initial_position;
	Scene::Transform* head = nullptr;
	glm::quat head_base_rotation;
	float wobble = 0.f;
	float head_degrees = 0.f;
	
	// Sphere transforms
	static constexpr int NUM_SPHERES = 39;
	Scene::Transform* spheres[NUM_SPHERES];

	// Game over flag
	bool game_over = false;

	//camera:
	Scene::Camera *camera = nullptr;

};
