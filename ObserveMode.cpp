#include "ObserveMode.hpp"
#include "DrawLines.hpp"
#include "LitColorTextureProgram.hpp"
#include "Mesh.hpp"
#include "Sprite.hpp"
#include "DrawSprites.hpp"
#include "data_path.hpp"
#include "Sound.hpp"

#include <iostream>
#include <sstream>

typedef struct {
    float y;
    int type;  // 0 blue, 1 red, 2 hit
} car_info;

std::vector<car_info> upcar, downcar;

typedef struct {
    glm::vec3 vel;
    glm::vec3 loc;
    float deg;
} gre_info;

std::vector<gre_info> gres;
const glm::vec3 rot_axis = glm::vec3(0.5, 0.5, 0.707);

int score = 0;
float ti = 30.f;
bool beginning = false;

Load< SpriteAtlas > trade_font_atlas(LoadTagDefault, []() -> SpriteAtlas const * {
	return new SpriteAtlas(data_path("trade-font"));
});

GLuint meshes_for_lit_color_texture_program = 0;
static Load< MeshBuffer > meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer *ret = new MeshBuffer({data_path("city.pnct"), data_path
                                   ("bluecar.pnct"), data_path("redcar.pnct")
                                   ,data_path("grenade.pnct")});
	meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

static Load< Scene > scene(LoadTagLate, []() -> Scene const * {
	Scene *ret = new Scene();
	ret->load(data_path("city.scene"), [](Scene &scene, Scene::Transform
	*transform, std::string const &mesh_name){
		auto &mesh = meshes->lookup(mesh_name);
		scene.drawables.emplace_back(transform);
		Scene::Drawable::Pipeline &pipeline = scene.drawables.back().pipeline;

		pipeline = lit_color_texture_program_pipeline;
		pipeline.vao = meshes_for_lit_color_texture_program;
		pipeline.type = mesh.type;
		pipeline.start = mesh.start;
		pipeline.count = mesh.count;
	});
	return ret;
});

static Load< Scene > blue_car_scene(LoadTagLate, []() -> Scene const * {
    Scene *ret = new Scene();
    ret->load(data_path("bluecar.scene"), [](Scene &scene, Scene::Transform
    *transform, std::string const &mesh_name){
        auto &mesh = meshes->lookup(mesh_name);
        scene.drawables.emplace_back(transform);
        Scene::Drawable::Pipeline &pipeline = scene.drawables.back().pipeline;

        pipeline = lit_color_texture_program_pipeline;
        pipeline.vao = meshes_for_lit_color_texture_program;
        pipeline.type = mesh.type;
        pipeline.start = mesh.start;
        pipeline.count = mesh.count;
    });
    return ret;
});

static Load< Scene > red_car_scene(LoadTagLate, []() -> Scene const * {
    Scene *ret = new Scene();
    ret->load(data_path("redcar.scene"), [](Scene &scene, Scene::Transform
    *transform, std::string const &mesh_name){
        auto &mesh = meshes->lookup(mesh_name);
        scene.drawables.emplace_back(transform);
        Scene::Drawable::Pipeline &pipeline = scene.drawables.back().pipeline;

        pipeline = lit_color_texture_program_pipeline;
        pipeline.vao = meshes_for_lit_color_texture_program;
        pipeline.type = mesh.type;
        pipeline.start = mesh.start;
        pipeline.count = mesh.count;
    });
    return ret;
});

static Load< Scene > grenade_scene(LoadTagLate, []() -> Scene const * {
    Scene *ret = new Scene();
    ret->load(data_path("grenade.scene"), [](Scene &scene, Scene::Transform
    *transform, std::string const &mesh_name){
        auto &mesh = meshes->lookup(mesh_name);
        scene.drawables.emplace_back(transform);
        Scene::Drawable::Pipeline &pipeline = scene.drawables.back().pipeline;

        pipeline = lit_color_texture_program_pipeline;
        pipeline.vao = meshes_for_lit_color_texture_program;
        pipeline.type = mesh.type;
        pipeline.start = mesh.start;
        pipeline.count = mesh.count;
    });
    return ret;
});

ObserveMode::ObserveMode() {
	assert(scene->cameras.size() && "Observe requires cameras.");

	if (upcar.empty()) {
	    float upy = 10.f, downy = 0.f;
	    srand(time(nullptr));
	    for (int i = 0; i < 300; ++i) {
            upcar.push_back({upy, (rand()%3==0)?1:0});
            downcar.push_back({downy, (rand()%2==0)?1:0});
            upy += 2.5f;
            downy -= 2.5f;
	    }
	}

	current_camera = &scene->cameras.front();

//	noise_loop = Sound::loop_3D(*noise, 1.0f, glm::vec3(0.0f, 0.0f, 0.0f), 10.0f);
}

ObserveMode::~ObserveMode() {
	noise_loop->stop();
}

bool ObserveMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (evt.type == SDL_MOUSEBUTTONDOWN && ti > 0.f) {
        beginning = true;
	    float v = 15.f;
	    float x = evt.button.x;
        float y = evt.button.y;

        float ratio = y / 540.f;
        float new_x = ratio * 10.5f + 6.f;
        float new_y = -5.f + ratio * 4.5f + x / 800.f * (18.f - 9.f * ratio);

        auto pos = scene->cameras.front().transform->position;
        auto diff = glm::vec3(new_x, new_y, 0.f) - pos;
        auto r = diff / sqrt(diff.x * diff.x + diff.y * diff.y + diff.z *
                diff.z);
        gres.push_back({glm::vec3(v * r.x, v * r.y, v * r.z),
                        pos,
                        0.f});
        return true;
	}
	
	return false;
}

void ObserveMode::update(float elapsed) {

    for (auto& car : upcar) {
        car.y -= elapsed * 1.5f;
    }

    for (auto& car : downcar) {
        car.y += elapsed * 1.5f;
    }


    for (auto it = gres.begin(); it != gres.end();) {
        it->loc += elapsed * it->vel;
        it->vel.z -= elapsed * 9.8f;
        it->deg += elapsed * 4.0f;

        if (it->loc.z < 0.3) {
            // upper area
//            std::cout << it->loc.x << ", " << it->loc.y << std::endl;
            if (it->loc.x < 12.f && it->loc.x > 10.f && it->loc.y > -1.f &&
                it->loc.y < 11.f) {
                for (auto &car : upcar) {
                    if (abs(car.y - it->loc.y - 0.9f) < 1.f && car.type == 0) {
                        car.type = 2;
                        score++;
                    } else if (abs(car.y - it->loc.y - 0.8f) < 1.2f && car.type
                    == 1) {
                        car.type = 2;
                        score--;
                    }
                }
            }

            // lower area
            if (it->loc.x < 14.f && it->loc.x > 12.f && it->loc.y > -1.f &&
                it->loc.y < 11.f) {
                for (auto &car : downcar) {
                    if (abs(car.y - it->loc.y + 0.4f) < 1.f && car.type == 0) {
                        car.type = 2;
                        score++;
                    } else if (abs(car.y - it->loc.y + 0.9f) < 1.2f && car.type
                    == 1) {
                        car.type = 2;
                        score--;
                    }
                }
            }

            it = gres.erase(it);
        } else {
            it++;
        }
    }

    if (beginning) {
        ti -= elapsed;
        if (ti < 0.f) {
            ti = 0.f;
        }
    }
}

void ObserveMode::draw(glm::uvec2 const &drawable_size) {
	//--- actual drawing ---
	glClearColor(0.85f, 0.85f, 0.90f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	const_cast< Scene::Camera * >(current_camera)->aspect = drawable_size.x / float(drawable_size.y);

	scene->draw(*current_camera);

	Scene::Transform trans;
	trans.scale = glm::vec3(0.5f, 0.5f, 0.5f);

    for (const auto& car : upcar) {
        if (car.y >= -5.f && car.y <= 15.f) {
            if (car.type == 0) {
                trans.position = glm::vec3(16.3f, car.y, 5.f);
                blue_car_scene->draw(*current_camera, trans);
            } else if (car.type == 1) {
                trans.position = glm::vec3(15.3f, car.y, 5.f);
                red_car_scene->draw(*current_camera, trans);
            }
        }
    }

    trans.rotation = glm::quat(0.f, 0.f, 0.f, 1.f);
    for (const auto& car : downcar) {
        if (car.y >= -5.f && car.y <= 15.f) {
            if (car.type == 0) {
                trans.position = glm::vec3(15.3f, car.y, 5.f);
                blue_car_scene->draw(*current_camera, trans);
            } else if (car.type == 1) {
                trans.position = glm::vec3(16.3f, car.y, 5.f);
                red_car_scene->draw(*current_camera, trans);
            }
        }
    }

    trans.scale = glm::vec3(0.1f, 0.1f, 0.1f);
    for (const auto& gre : gres) {
        trans.rotation = glm::quat(cos(gre.deg), sin(gre.deg)*rot_axis.x,
                                   sin(gre.deg)*rot_axis.y, sin(gre.deg)
                                   *rot_axis.z);
        trans.position = gre.loc;
        grenade_scene->draw(*current_camera, trans);
    }

	{ //help text overlay:
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		DrawSprites draw(*trade_font_atlas, glm::vec2(0,0), glm::vec2(320, 200), drawable_size, DrawSprites::AlignPixelPerfect);

		std::string help_text = "--- THROW GRENADE TO BLUE CARS ---";
		glm::vec2 min, max;
		draw.get_text_extents(help_text, glm::vec2(0.0f, 0.0f), 1.0f, &min, &max);
		float x = std::round(160.0f - (0.5f * (max.x + min.x)));
		draw.draw_text(help_text, glm::vec2(x, 1.0f), 1.0f, glm::u8vec4(0x00,0x00,0x00,0xff));
		draw.draw_text(help_text, glm::vec2(x, 2.0f), 1.0f, glm::u8vec4(0xff,0xff,0xff,0xff));

        std::ostringstream stringStream;
        stringStream.precision(1);
        stringStream.str("");
        stringStream << "Time: " << std::fixed << ti << "s";

        DrawSprites draw2(*trade_font_atlas, glm::vec2(0,0), glm::vec2(800,
                540), drawable_size, DrawSprites::AlignSloppy);
        draw2.draw_text(stringStream.str(), glm::vec2(20.f, 500.0f), 2.f,
                glm::u8vec4(0x00,0x00,0x00,0xff));

        stringStream.precision(0);
        stringStream.str("");
        stringStream << "Score: " << score;
        draw2.draw_text(stringStream.str(), glm::vec2(650.f, 500.0f), 2.f,
                        glm::u8vec4(0x00,0x00,0x00,0xff));

        if (ti == 0.f) {
            stringStream.str("");
            stringStream << "You got " << score << "!";
            draw2.draw_text(stringStream.str(), glm::vec2(250.f, 270.0f), 4.f,
                            glm::u8vec4(0xff,0x00,0x00,0xff));
        }
    }
}
