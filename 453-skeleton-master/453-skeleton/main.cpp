#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "glm/ext.hpp"

#include <iostream>
#include <string>
#include <math.h>
#include <cmath>

#include "Geometry.h"
#include "GLDebug.h"
#include "Log.h"
#include "ShaderProgram.h"
#include "Shader.h"
#include "Texture.h"
#include "Window.h"


#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

using namespace std;

const float PI = 3.14159265358979323846f;


// most of the work done here is cited from Assignment tutorial and conversations w TAs

// An example struct for Game Objects.
// You are encouraged to customize this as you see fit.
struct GameObject {
	// Struct's constructor deals with the texture.
	// Also sets default position, theta, scale, and transformationMatrix
	GameObject(std::string texturePath, GLenum textureInterpolation) :
		texture(texturePath, textureInterpolation),
		position(0.0f, 0.0f, 0.0f),
		mousepos(0.1f, 0.1f, 0.0f),
		vectorhat(0.0f, 0.0f, 0.0f),
		move(0.f),
		theta(0),
		oldtheta(0),
		scale(1),
		transformationMatrix(1.0f), // This constructor sets it as the identity matrix
		hasMoved(false),
		restart(false)
	{}

	CPU_Geometry cgeom;
	GPU_Geometry ggeom;
	Texture texture;
	glm::vec3 position;
	glm::vec3 mousepos;
	glm::vec3 vectorhat;
	float move;
	float oldtheta;
	float theta; // Object's rotation
	// Alternatively, you could represent rotation via a normalized heading vec:
	// glm::vec3 heading;
	float scale; // Or, alternatively, a glm::vec2 scale;
	glm::mat4 transformationMatrix;
	
	bool hasMoved;
	bool restart = false;
};


// EXAMPLE CALLBACKS
class MyCallbacks : public CallbackInterface {

public:
	MyCallbacks(ShaderProgram& shader, GameObject& ship, GameObject& diamond, GameObject& diamond2, GameObject& diamond3) : shader(shader), ship(ship), diamond(diamond), diamond2(diamond2), diamond3(diamond3) {}


	virtual void keyCallback(int key, int scancode, int action, int mods) {
		if (action == GLFW_PRESS || action == GLFW_REPEAT) {
			

			if (key == GLFW_KEY_UP) {
				if (glm::distance(ship.mousepos, ship.position) <= 0.07f) {
					ship.move = 0.f;
				}
				else {
					ship.move += 0.001f;
					ship.hasMoved = true;
				}
				shader.recompile();
				std::cout << "ship pos new: (" << ship.position.x << ", " << ship.position.y << ")" << std::endl;
			}
			if (key == GLFW_KEY_DOWN) {
				ship.move = -0.001f;
				ship.hasMoved = true;
				shader.recompile();
			}

			// r for restart
			if (key == GLFW_KEY_R) {
				ship.restart = true;
			}

		}
	
	}


	virtual void cursorPosCallback(double xpos, double ypos) {
		//std::cout << xpos << "," << ypos << std::endl;

		static const glm::vec2 screen_dims = { 800.0f, 800.0f };

		glm::mat4 pixel_centering_T = glm::translate(glm::mat4(1.f), glm::vec3(0.5f, 0.5f, 0.f));

		glm::mat4 normalize_S = glm::scale(glm::mat4(1.f), glm::vec3(2.f / (screen_dims.x - 1.f), -2.f / (screen_dims.y - 1.f), 1.f));

		glm::mat4 normalized_centering_T = glm::translate(glm::mat4(1.f), glm::vec3(-1.f, 1.f, 0.f));

		glm::mat4 Matrx = normalized_centering_T * normalize_S * pixel_centering_T;

		// the initial position where the ship is facing??
		glm::vec4 p0 = { xpos, ypos, 0.f, 1.f };

		glm::vec4 p1 = pixel_centering_T * p0;

		glm::vec4 p2 = normalize_S * p1;

		glm::vec4 p3 = normalized_centering_T * p2;

		normalizedCpos = p3;
		ship.mousepos = p3;

		glm::vec3 vector = ship.mousepos - ship.position;
		glm::vec3 vectorhat = glm::normalize(vector);

		ship.vectorhat = vectorhat;

		// figuring out the directions and quadrants

		if (p3[0] > ship.position.x && p3[1] > ship.position.y) {
			ship.theta = 3 * (PI / 2) + (acos(glm::dot(vectorhat, { 1,0,0 })));
		}
		else if (p3[0] == ship.position.x && p3[1] > ship.position.y) {
			ship.theta = 0;
		}
		else if (p3[0] < ship.position.x && p3[1] == ship.position.y) {
			ship.theta = PI / 2;
		}
		else if (p3[0] == ship.position.x && p3[1] < ship.position.y) {
			ship.theta = PI;
		}
		else if (p3[0] > ship.position.x && p3[1] == ship.position.y) {
			ship.theta = 3 * (PI / 2);
		}
		else if (p3[0] > ship.position.x && p3[1] < ship.position.y) {
			ship.theta = (2 * PI) - acos(glm::dot(vectorhat, { 1, 0, 0 })) - PI / 2;
		}
		else if (p3[0] < ship.position.x && p3[1] < ship.position.y) {
			ship.theta = PI + (PI - acos(glm::dot(vectorhat, { 1, 0, 0 }))) - PI / 2;
		}
		else if (p3[0] < ship.position.x && p3[1]>ship.position.y) {
			ship.theta = acos(glm::dot(vectorhat, { 1,0,0 })) - PI / 2;
		}

		shader.recompile();
	}

	glm::vec2 normalizedCpos = { 0., 0. };


private:
	ShaderProgram& shader;
	GameObject& ship;
	GameObject& diamond;
	GameObject& diamond2;
	GameObject& diamond3;
	//bool restart;
	//double mouseX, mouseY;
};


CPU_Geometry GameObj(float width, float height) {
	float halfWidth = width / 2.0f;
	float halfHeight = height / 2.0f;
	CPU_Geometry retGeom;
	// vertices for the spaceship quad
	/*retGeom.verts.push_back(glm::vec3(-halfWidth, halfHeight, 1.f));
	retGeom.verts.push_back(glm::vec3(-halfWidth, -halfHeight, 1.f));
	retGeom.verts.push_back(glm::vec3(halfWidth, -halfHeight, 1.f));
	retGeom.verts.push_back(glm::vec3(-halfWidth, halfHeight, 1.f));
	retGeom.verts.push_back(glm::vec3(halfWidth, -halfHeight, 1.f));
	retGeom.verts.push_back(glm::vec3(halfWidth, halfHeight, 1.f));*/

	// For full marks (Part IV), you'll need to use the following vertex coordinates instead.
	// Then, you'd get the correct scale/translation/rotation by passing in uniforms into
	// the vertex shader.

	retGeom.verts.push_back(glm::vec3(-1.f, 1.f, 0.f));
	retGeom.verts.push_back(glm::vec3(-1.f, -1.f, 0.f));
	retGeom.verts.push_back(glm::vec3(1.f, -1.f, 0.f));
	retGeom.verts.push_back(glm::vec3(-1.f, 1.f, 0.f));
	retGeom.verts.push_back(glm::vec3(1.f, -1.f, 0.f));
	retGeom.verts.push_back(glm::vec3(1.f, 1.f, 0.f));


	// texture coordinates
	retGeom.texCoords.push_back(glm::vec2(0.f, 1.f));
	retGeom.texCoords.push_back(glm::vec2(0.f, 0.f));
	retGeom.texCoords.push_back(glm::vec2(1.f, 0.f));
	retGeom.texCoords.push_back(glm::vec2(0.f, 1.f));
	retGeom.texCoords.push_back(glm::vec2(1.f, 0.f));
	retGeom.texCoords.push_back(glm::vec2(1.f, 1.f));
	return retGeom;
}

void restart(GameObject& ship, GameObject& diamond, GameObject& diamond2, GameObject& diamond3) {

	// doesnt work
	/*ship.cgeom.verts.clear();
	diamond.cgeom.verts.clear();
	diamond2.cgeom.verts.clear();
	diamond3.cgeom.verts.clear();*/

	// reset the locations:

	ship.cgeom = GameObj(0.18f, 0.12f);
	diamond.cgeom = GameObj(0.10f, 0.10f);
	diamond2.cgeom = GameObj(0.10f, 0.10f);
	diamond3.cgeom = GameObj(0.10f, 0.10f);

	// perform the translation again and

	diamond.position = glm::vec3{ 0.4f,0.4f,0.f };
	diamond2.position = glm::vec3{ -0.4f,0.4f,0.f };
	diamond3.position = glm::vec3{ 0.4f,-0.4f,0.f };
	ship.position = { 0.0f, 0.0f, 0.0f };

	ship.move = 0.00f;
	diamond.move = 0.005f;
	diamond2.move = 0.005f;
	diamond3.move = 0.005f;

	// this did not workkkk afterwards

	//for (int i = 0; i < diamond.cgeom.verts.size(); i++) {
	//	diamond.cgeom.verts[i] = trMatrix4 * diamond.cgeom.verts[i];
	//}
	//diamond.position += glm::vec3{ 0.4f, 0.4f, 0.f };

	//for (int i = 0; i < diamond2.cgeom.verts.size(); i++) {
	//	diamond2.cgeom.verts[i] = trMatrix2 * diamond2.cgeom.verts[i];
	//}
	//diamond2.position += glm::vec3{ -0.4f, 0.4f, 0.f };

	//for (int i = 0; i < diamond3.cgeom.verts.size(); i++) {
	//	diamond3.cgeom.verts[i] = trMatrix3 * diamond3.cgeom.verts[i];
	//}
	//diamond3.position += glm::vec3{ 0.4f, -0.4f, 0.f };


}


// END EXAMPLES

// put thr matrix in the loop lowkey

int main() {

	Log::debug("Starting main");

	// WINDOW
	glfwInit();

	Window window(800, 800, "CPSC 453"); // can set callbacks at construction if desired

	GLDebug::enable();

	// SHADERS
	ShaderProgram shader("shaders/test.vert", "shaders/test.frag");
	GameObject ship("textures/ship.png", GL_NEAREST);
	GameObject diamond("textures/diamond.png", GL_NEAREST);
	GameObject diamond2("textures/diamond.png", GL_NEAREST);
	GameObject diamond3("textures/diamond.png", GL_NEAREST);


	// CALLBACKS
	auto myCallbacks = (std::make_shared<MyCallbacks>(shader, ship, diamond, diamond2, diamond3)); // can also update callbacks to new ones
	window.setCallbacks(myCallbacks);


	ship.cgeom = GameObj(0.18f, 0.12f);
	diamond.cgeom = GameObj(0.10f, 0.10f);
	diamond2.cgeom = GameObj(0.10f, 0.10f);
	diamond3.cgeom = GameObj(0.10f, 0.10f);

	ship.move = 0.000f;
	diamond.move = 0.005f;
	diamond2.move = 0.005f;
	diamond3.move = 0.005f;

	diamond.position = glm::vec3(0.4f, 0.4f, 0.f);
	diamond2.position = glm::vec3(-0.4f, 0.4f, 0.f);
	diamond3.position = glm::vec3(0.4f, -0.4f, 0.f);


	int score = 0;

	int diamoundTouch = 0;
	int diamondTouch2 = 0;
	int diamondTouch3 = 0;

	// RENDER LOOP
	while (!window.shouldClose()) {

		//Log::debug("starting frame");
		// so if R is pressed i want to reset or ratehr clear everything and call the resstart
		// needed to be set back to fals after sha

		if (ship.restart == true) {

			ship.hasMoved = false;
			ship.cgeom.verts.clear();
			diamond.cgeom.verts.clear();
			diamond2.cgeom.verts.clear();
			diamond3.cgeom.verts.clear();
			score = 0;
			restart(ship, diamond, diamond2, diamond3);
			ship.restart = false;
		}


		glfwPollEvents();

		shader.use();
		ship.ggeom.bind();

		glm::vec3 v0 = ship.move * (ship.mousepos - ship.position) / glm::distance(ship.mousepos, ship.position);
		glm::mat4 shipTranslationM = glm::mat4(
			1.0f, 0.f, 0.f, 0.f,
			0.f, 1.0f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			v0[0], v0[1], 0.f, 1.0f
		);
		glm::mat4 shipOGTransM = glm::mat4(
			1.0f, 0.f, 0.f, 0.f,
			0.f, 1.0f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			(0.f - ship.position[0]), (0.f - ship.position[1]), 0.f, 1.0f
		);
		glm::mat4 shipNegTransM = glm::mat4(
			1.0f, 0.f, 0.f, 0.f,
			0.f, 1.0f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			(ship.position[0] - 0.f), (ship.position[1] - 0.f), 0.f, 1.0f
		);

		// rotation matrix

		glm::mat4 rotationMatrix = glm::mat4(
			cos(ship.theta), sin(ship.theta), 0.f, 0.f,
			-sin(ship.theta), cos(ship.theta), 0.f, 0.f,
			0.f, 0.f, 0.0f, 0.f,
			0.f, 0.f, 0.f, 1.0f
		);

		// scale

		glm::mat4 scaleShip = glm::mat4(
			0.09f, 0.f, 0.f, 0.f,
			0.f, 0.06f, 0.f, 0.f,
			0.f, 0.f, 0.1f, 0.f,
			0.f, 0.f, 0.f, 1.0f
		);


		ship.transformationMatrix = shipTranslationM * shipNegTransM * rotationMatrix * scaleShip * shipOGTransM;

		// shout out tut
		GLint myLoc = glGetUniformLocation(shader.getProgram(), "Matrx");
		glUniformMatrix4fv(myLoc, 1, false, glm::value_ptr(ship.transformationMatrix));

		ship.position += ship.move * (ship.mousepos - ship.position);
		ship.oldtheta = ship.theta;
		ship.scale = 1.0f;

		glEnable(GL_FRAMEBUFFER_SRGB);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ship.ggeom.setVerts(ship.cgeom.verts);
		ship.ggeom.setTexCoords(ship.cgeom.texCoords);
		ship.texture.bind();
		glDrawArrays(GL_TRIANGLES, 0, 6);
		ship.texture.unbind();
		glDisable(GL_FRAMEBUFFER_SRGB);


		/// now have to redo all diamonds:
		float move = 0.005f;

		bool isCollision = glm::distance(ship.position, diamond.position) <= 0.1f && ship.hasMoved;
		if (isCollision) {
			ship.scale = 1.15f;
			diamond.cgeom.verts.clear();
			diamond.position = { 100.f, 100.f, 100.f };
			score +=1;
			std::cout << "Ship destroyed 1!" << std::endl;
		}

		diamond.ggeom.bind();

		glm::mat4 transD1Matrix = glm::mat4(
			1.0f, 0.f, 0.f, 0.f,
			0.f, 1.0f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.4f, 0.4f, 0.f, 1.0f
		);
		glm::mat4 transMatrix = glm::mat4(
			1.0f, 0.f, 0.f, 0.f,
			0.f, 1.0f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			diamond.move, 0.f, 0.f, 1.0f
		);
		glm::mat4 diamondscaleMatrix = glm::mat4(
			0.05f, 0.f, 0.f, 0.f,
			0.f, 0.05f, 0.f, 0.f,
			0.f, 0.f, 0.1f, 0.f,
			0.f, 0.f, 0.f, 1.0f
		);

		//pos check
		if (diamond.position[0] >= 1.0f) {
			diamoundTouch = 1;
		}
		else if (diamond.position[0] <= -1.0f) {
			diamoundTouch = 2;
		}
		if (diamoundTouch == 0) {
			diamond.move += move;
			diamond.position += glm::vec3{ 0.005, 0.f,0.f };
		}
		else if (diamoundTouch == 1) {
			diamond.move -= move;
			diamond.position += glm::vec3{ -0.005, 0.f,0.f };
		}
		else if (diamoundTouch == 2) {
			diamond.move += move;
			diamond.position += glm::vec3{ 0.005, 0.f,0.f };
		}

		diamond.transformationMatrix = transMatrix * transD1Matrix * diamondscaleMatrix;

		myLoc = glGetUniformLocation(shader.getProgram(), "Matrx");
		glUniformMatrix4fv(myLoc, 1, false, glm::value_ptr(diamond.transformationMatrix));

		diamond.ggeom.setVerts(diamond.cgeom.verts);
		diamond.ggeom.setTexCoords(diamond.cgeom.texCoords);

		diamond.texture.bind();
		diamond.texture.unbind();
		glDrawArrays(GL_TRIANGLES, 0, diamond.cgeom.verts.size());




		// start diamond 2
		// no need to use isCollision me

		bool isCollision2 = glm::distance(ship.position, diamond2.position) <= 0.1f && ship.hasMoved;

		if (isCollision2) {
			ship.scale = 1.15f;

			diamond2.cgeom.verts.clear();
			diamond2.position = { 100.f, 100.f, 100.f }; // send that boy outtttttttttttttt
			score +=1;


			std::cout << "Ship destroyed 2!" << std::endl;
		}


		diamond2.ggeom.bind();
		glm::mat4 transD2Matrix = glm::mat4(
			1.0f, 0.f, 0.f, 0.f,
			0.f, 1.0f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			-0.4f, 0.4f, 0.f, 1.0f
		);
		glm::mat4 trans21Matrix = glm::mat4(
			1.0f, 0.f, 0.f, 0.f,
			0.f, 1.0f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			diamond2.move, -diamond2.move, 0.f, 1.0f
		);
		glm::mat4 diamond2scaleMatrix = glm::mat4(
			0.05f, 0.f, 0.f, 0.f,
			0.f, 0.05f, 0.f, 0.f,
			0.f, 0.f, 0.1f, 0.f,
			0.f, 0.f, 0.f, 1.0f
		);
		if (diamond2.position[0] >= 1.0f) {
			diamondTouch2 = 1;
		}
		else if (diamond2.position[0] <= -1.0f) {
			diamondTouch2 = 2;
		}

		if (diamondTouch2 == 0) {
			diamond2.move += move;
			diamond2.position += glm::vec3{ 0.005, -0.005f,0.f };
		}
		else if (diamondTouch2 == 1) {
			diamond2.move -= move;
			diamond2.position += glm::vec3{ -0.005, 0.005f,0.f };
		}
		else if (diamondTouch2 == 2) {
			diamond2.move += move;
			diamond2.position += glm::vec3{ 0.005, -0.005f,0.f };
		}

		diamond2.transformationMatrix = trans21Matrix * trans21Matrix * diamond2scaleMatrix;
		myLoc = glGetUniformLocation(shader.getProgram(), "Matrx");
		glUniformMatrix4fv(myLoc, 1, false, glm::value_ptr(diamond2.transformationMatrix));

		diamond2.ggeom.setVerts(diamond2.cgeom.verts);
		diamond2.ggeom.setTexCoords(diamond2.cgeom.texCoords);

		diamond2.texture.bind();
		diamond2.texture.unbind();
		glDrawArrays(GL_TRIANGLES, 0, diamond2.cgeom.verts.size());



		bool isCollision3 = glm::distance(ship.position, diamond3.position) <= 0.1f && ship.hasMoved;

		if (isCollision3) {
			ship.scale = 1.15f;
			diamond3.cgeom.verts.clear();
			diamond3.position = { 100.f, 100.f, 100.f };
			score +=1;
			std::cout << "Ship destroyed 3!" << std::endl;
		}

		diamond3.ggeom.bind();
		glm::mat4 trans3Matrix = glm::mat4(
			1.0f, 0.f, 0.f, 0.f,
			0.f, 1.0f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.4f, -0.4f, 0.f, 1.0f
		);
		glm::mat4 trans31Matrix = glm::mat4(
			1.0f, 0.f, 0.f, 0.f,
			0.f, 1.0f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			-diamond3.move, -diamond3.move / 2, 0.f, 1.0f
		);
		glm::mat4 diamond3scaleMatrix = glm::mat4(
			0.05f, 0.f, 0.f, 0.f,
			0.f, 0.05f, 0.f, 0.f,
			0.f, 0.f, 0.1f, 0.f,
			0.f, 0.f, 0.f, 1.0f
		);
		if (diamond3.position[1] <= -1.0f) {
			diamondTouch3 = 1;
		}
		else if (diamond3.position[0] >= 1.0f) {
			diamondTouch3 = 2;
		}
		if (diamondTouch3 == 0) {
			diamond3.move += move;
			diamond3.position += glm::vec3{ -0.005, -0.005f / 2,0.f };
		}
		else if (diamondTouch3 == 1) {
			diamond3.move -= move;
			diamond3.position += glm::vec3{ 0.005, 0.005f / 2,0.f };
		}
		else if (diamondTouch3 == 2) {
			diamond3.move += move;
			diamond3.position += glm::vec3{ -0.005, -0.005f / 2,0.f };
		}

		diamond3.transformationMatrix = trans31Matrix * trans3Matrix * diamond3scaleMatrix;
		myLoc = glGetUniformLocation(shader.getProgram(), "Matrx");
		glUniformMatrix4fv(myLoc, 1, false, glm::value_ptr(diamond3.transformationMatrix));


		diamond3.ggeom.setVerts(diamond3.cgeom.verts);
		diamond3.ggeom.setTexCoords(diamond3.cgeom.texCoords);
		diamond3.texture.bind();
		diamond3.texture.unbind();
		glDrawArrays(GL_TRIANGLES, 0, diamond3.cgeom.verts.size());



		//glDisable(GL_FRAMEBUFFER_SRGB); // disable sRGB for things like imgui

		// Starting the new ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		// Putting the text-containing window in the top-left of the screen.
		ImGui::SetNextWindowPos(ImVec2(5, 5));

		// Setting flags
		ImGuiWindowFlags textWindowFlags =
			ImGuiWindowFlags_NoMove |				// text "window" should not move
			ImGuiWindowFlags_NoResize |				// should not resize
			ImGuiWindowFlags_NoCollapse |			// should not collapse
			ImGuiWindowFlags_NoSavedSettings |		// don't want saved settings mucking things up
			ImGuiWindowFlags_AlwaysAutoResize |		// window should auto-resize to fit the text
			ImGuiWindowFlags_NoBackground |			// window should be transparent; only the text should be visible
			ImGuiWindowFlags_NoDecoration |			// no decoration; only the text should be visible
			ImGuiWindowFlags_NoTitleBar;			// no title; only the text should be visible

		// Begin a new window with these flags. (bool *)0 is the "default" value for its argument.
		ImGui::Begin("scoreText", (bool*)0, textWindowFlags);

		// Scale up text a little, and set its value
		ImGui::SetWindowFontScale(1.5f);

		if (score != 3) {
			ImGui::Text("Score: %d", score); // Second parameter gets passed into "%d"

		}
		else {
			ImGui::Text("Good job completing the game!!! Press [R] to Restart"); // Second parameter gets passed into "%d"
		}

		// End the window.
		ImGui::End();

		ImGui::Render();	// Render the ImGui window
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); // Some middleware thing

		window.swapBuffers();
	}

	// ImGui cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;

}

