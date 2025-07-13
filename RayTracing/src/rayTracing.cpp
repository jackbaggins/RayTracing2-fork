#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <OpenGL/shaderClass.h>
#include <OpenGL/VAO.h>
#include <OpenGL/VBO.h>
#include <OpenGL/SSBO.h>
#include <OpenGL/UBO.h>
#include <OpenGL/FBO.h>

#include <Assets/headers/BVH.h>

#include <Assets/headers/camera.h>
#include <Assets/headers/mesh.h>

#include <Assets/headers/tinyfiledialogs.h>

#include "stb/stb_image_write.h"

#include <iomanip>
#include <sstream>

// const std::string modelFolderName = "toonHouse";
// const std::string modelFolderName = "mccree";
// const std::string modelFolderName = "cat";
// const std::string modelFolderName = "trees";
// const std::string modelFolderName = "windmill";
// const std::string modelFolderName = "building";
// const std::string modelFolderName = "jet";
// const std::string modelFolderName = "ghetto";
const std::string modelFolderName = "campfire";
// const std::string modelFolderName = "trees_3_camps";
// const std::string modelFolderName = "plants";
// const std::string modelFolderName = "rinTex";
// const std::string modelFolderName = "plants";
// const std::string modelFolderName = "autumn_kitten";
// const std::string modelFolderName = "robot";
// const std::string modelFolderName = "dragon";
// const std::string modelFolderName = "goofy";
// const std::string modelFolderName = "sleeping";

const int WORK_SIZE_X = 8;
const int WORK_SIZE_Y = 4;
const int WORK_SIZE_Z = 1;

const int SCR_WIDTH = 1000;
const int SCR_HEIGHT = 1000;
// const int SCR_WIDTH = 400;
// const int SCR_HEIGHT = 400;

const int MAX_SPHERES = 5;

const int MAX_BOUNCE_COUNT = 10;
float numRaysPerPixel = 5;
const float RAYS_PER_PIXEL_SENSITIVITY = 10.0f;

const bool BASIC_SHADING = true;
const bool BASIC_SHADING_SHADOW = false;
const glm::vec3 LIGHT_POSITION = glm::vec3(10.0f, 100.0f, 1.0f);
const bool BASIC_SHADING_ENVIRONMENTAL_LIGHT = false;

const bool SCREENSHOT_BASIC_SHADING = false;
const int SCREENSHOT_ENVIRONMENTAL_LIGHT = true;
const int SCREENSHOT_MAX_BOUNCE_COUNT = 20;
const int SCREENSHOT_RAYS_PER_PIXEL = 64;
const int SCREENSHOT_FRAMES = 10;

const float CORNELL_LIGHT_BRIGHTNESS = 15.0f;
const float CORNELL_PADDING = 0.3f;
const float CORNELL_LIGHT_SIZE = 0.17f;

const int FPS = 120;
const float SPF = 1.0f / FPS;

// Starting values for camera
float maxSpeed = 10.0f;
float hfov = PI / 6;
float pitch = 0.0f;
float yaw = PI / 2.0f;
float focusDistance = 20.0f;
float defocusAngle = 0.0f;
float zoom = 1.0f;
glm::vec3 cameraPos(0.0f, 5.0f, 10.0f);

float vertices[] = {
	-1.0f, -1.0f, 0.0f,
	 1.0f, -1.0f, 0.0f,
	 1.0f,  1.0f, 0.0f,
	-1.0f, -1.0f, 0.0f,
	-1.0f,  1.0f, 0.0f,
	 1.0f,  1.0f, 0.0f
};

/**
 * @brief Renders a high-quality path-traced screenshot and saves it as a PNG file.
 *
 * This function performs an off-screen path tracing render over multiple frames to produce a
 * high-quality image. It accumulates the pixel data across frames to simulate anti-aliasing
 * and denoising via Monte Carlo integration, then writes the final result to disk.
 *
 * Steps performed:
 * - Configures rendering uniforms and initializes framebuffer and texture for off-screen rendering.
 * - Iteratively dispatches compute shaders and renders to a framebuffer across `SCREENSHOT_FRAMES` frames.
 * - Reads and accumulates pixel data from the framebuffer after each frame.
 * - Averages pixel values, vertically flips the image, and writes the final PNG using `stb_image_write`.
 * - Logs render time and optionally terminates the program if it exceeds a time threshold.
 *
 * @param window            The GLFW window handle used for buffer swapping and timing.
 * @param camera            Reference to the active camera used for updating view parameters.
 * @param VAO               Vertex Array Object used during screen-space rendering.
 * @param UBO               Uniform Buffer Object to pass global shader parameters.
 * @param uniforms          GlobalUniforms struct with rendering parameters.
 * @param renderShader      Shader program used to draw the final texture to the framebuffer.
 * @param computeShader     Compute shader used to perform ray tracing and write to the screen texture.
 * @param screenTexture     The texture used to display the current render output.
 * @param terminateProgram  Output flag; set to true if render time exceeds a predefined threshold.
 */
void screenshot(GLFWwindow* window, Camera& camera, VAO& VAO, UBO& UBO, GlobalUniforms& uniforms, Shader& renderShader, ComputeShader& computeShader, Texture2D screenTexture, bool& terminateProgram)
{
	std::cout << "Performing Path Tracing, this will take a very long time and slow down your computer." << std::endl;


	/* SCR_WIDTH and SCR_HEIGHT are statically assigned constants, this is potentially dead code
	*
	*  Check for potential overflow and validate dimensions
	if (SCR_WIDTH <= 0 || SCR_HEIGHT <= 0) {
	*   std::cerr << "Invalid screen dimensions!" << std::endl;
	*   return;
	*   }
	*/
	size_t pixelCount = static_cast<size_t>(SCR_WIDTH) * static_cast<size_t>(SCR_HEIGHT) * 3;
	if (pixelCount > SIZE_MAX / sizeof(GLubyte) || pixelCount > SIZE_MAX / sizeof(float)) {
		std::cerr << "Image too large, would cause overflow!" << std::endl;
		return;
	}

	std::cout << "Image dimensions: " << SCR_WIDTH << "x" << SCR_HEIGHT << std::endl;
	std::cout << "Total pixel data size: " << pixelCount << " elements" << std::endl;

	uniforms.basicShading = SCREENSHOT_BASIC_SHADING;
	uniforms.environmentalLight = SCREENSHOT_ENVIRONMENTAL_LIGHT;
	uniforms.maxBounceCount = SCREENSHOT_MAX_BOUNCE_COUNT;
	uniforms.numRaysPerPixel = SCREENSHOT_RAYS_PER_PIXEL;
	uniforms.frameIndex = 0;

	camera.updateUniforms(uniforms);
	UBO.Update(&uniforms, sizeof(GlobalUniforms));

	// Use vectors for automatic memory management and safety
	std::vector<GLubyte> pixels(pixelCount, 0);
	std::vector<float> pixelsCumulative(pixelCount, 0.0f);

	// Draw
	VAO.Bind();

	Texture2D screenshotTexture(SCR_WIDTH, SCR_HEIGHT, nullptr, 0, GL_RGB, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_TEXTURE6);
	FBO FBO(screenshotTexture);

	// Check framebuffer completeness
	FBO.Bind();
	GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fbStatus != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "Framebuffer not complete: 0x" << std::hex << fbStatus << std::dec;
		switch (fbStatus) {
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: std::cerr << " (INCOMPLETE_ATTACHMENT)"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: std::cerr << " (MISSING_ATTACHMENT)"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: std::cerr << " (INCOMPLETE_DRAW_BUFFER)"; break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: std::cerr << " (INCOMPLETE_READ_BUFFER)"; break;
		case GL_FRAMEBUFFER_UNSUPPORTED: std::cerr << " (UNSUPPORTED)"; break;
		default: std::cerr << " (UNKNOWN)"; break;
		}
		std::cerr << std::endl;
		return;
	}

	float renderStart = glfwGetTime();

	for (int i = 0; i < SCREENSHOT_FRAMES; i++)
	{
		float start = glfwGetTime();
		uniforms.frameIndex = i;
		UBO.Update(&uniforms, sizeof(GlobalUniforms));

		computeShader.Activate();
		glDispatchCompute(ceil(SCR_WIDTH / WORK_SIZE_X), ceil(SCR_HEIGHT / WORK_SIZE_Y), 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		screenTexture.SetActive();
		screenTexture.Bind();
		FBO.Bind();
		glClear(GL_COLOR_BUFFER_BIT);
		renderShader.Activate();
		VAO.Bind();
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// Check for OpenGL errors before reading pixels
		GLenum error = glGetError();
		if (error != GL_NO_ERROR) {
			std::cerr << "OpenGL error before glReadPixels: 0x" << std::hex << error << std::dec;
			switch (error) {
			case GL_INVALID_ENUM: std::cerr << " (GL_INVALID_ENUM)"; break;
			case GL_INVALID_VALUE: std::cerr << " (GL_INVALID_VALUE)"; break;
			case GL_INVALID_OPERATION: std::cerr << " (GL_INVALID_OPERATION)"; break;
			case GL_INVALID_FRAMEBUFFER_OPERATION: std::cerr << " (GL_INVALID_FRAMEBUFFER_OPERATION)"; break;
			case GL_OUT_OF_MEMORY: std::cerr << " (GL_OUT_OF_MEMORY)"; break;
			default: std::cerr << " (UNKNOWN)"; break;
			}
			std::cerr << std::endl;
		}

		glReadPixels(0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

		// Check for OpenGL errors after reading pixels
		error = glGetError();
		if (error != GL_NO_ERROR) {
			std::cerr << "OpenGL error after glReadPixels: 0x" << std::hex << error << std::dec;
			switch (error) {
			case GL_INVALID_ENUM: std::cerr << " (GL_INVALID_ENUM)"; break;
			case GL_INVALID_VALUE: std::cerr << " (GL_INVALID_VALUE)"; break;
			case GL_INVALID_OPERATION: std::cerr << " (GL_INVALID_OPERATION)"; break;
			case GL_INVALID_FRAMEBUFFER_OPERATION: std::cerr << " (GL_INVALID_FRAMEBUFFER_OPERATION)"; break;
			case GL_OUT_OF_MEMORY: std::cerr << " (GL_OUT_OF_MEMORY)"; break;
			default: std::cerr << " (UNKNOWN)"; break;
			}
			std::cerr << std::endl;
			break; // Exit loop on error
		}

		// Accumulate pixel values
		for (size_t j = 0; j < pixelCount; j++) {
			pixelsCumulative[j] += static_cast<float>(pixels[j]);
		}

		std::cout << "Frame " << i << " done. Render time: " << glfwGetTime() - start << std::endl;
		glfwSwapBuffers(window);
	}

	screenshotTexture.Delete();
	FBO.Delete();

	// Get the average result of all frames
	for (size_t j = 0; j < pixelCount; j++) {
		pixels[j] = static_cast<GLubyte>(std::min(255.0f, pixelsCumulative[j] / SCREENSHOT_FRAMES));
	}

	// Flip the image vertically (OpenGL's origin is at the bottom-left, but most images start at the top-left)
	for (int y = 0; y < SCR_HEIGHT / 2; y++) {
		for (int x = 0; x < SCR_WIDTH * 3; x++) {
			size_t topIndex = static_cast<size_t>(y) * SCR_WIDTH * 3 + x;
			size_t bottomIndex = static_cast<size_t>(SCR_HEIGHT - 1 - y) * SCR_WIDTH * 3 + x;
			std::swap(pixels[topIndex], pixels[bottomIndex]);
		}
	}

	std::string path = getPath("\Images\\test.png", 1);

	// Write the image
	int result = stbi_write_png(path.c_str(), SCR_WIDTH, SCR_HEIGHT, 3, pixels.data(), SCR_WIDTH * 3);
	if (!result) {
		std::cerr << "Failed to write PNG file: " << path << std::endl;
	}
	else {
		std::cout << "Screenshot saved to: " << path << std::endl;
	}

	// Restore viewport
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

	float totalRenderTime = glfwGetTime() - renderStart;
	std::cout << "Total render time: " << totalRenderTime / 60.0f << " minutes." << std::endl;

	if (totalRenderTime > 10.0f) {
		terminateProgram = true;
	}

	// Memory is automatically cleaned up by vectors when they go out of scope
}
/**
 * @brief Forwards mouse movement input to the camera.
 *
 * Retrieves the camera pointer from the window's user data and passes the cursor position to it.
 */

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
	Camera* cameraPtr = static_cast<Camera*>(glfwGetWindowUserPointer(window));
	cameraPtr->mouseCallback(xpos, ypos);
}

/**
 * @brief Handles mouse button input to toggle cursor visibility mode.
 *
 * Enables or disables the cursor based on left or middle mouse button presses.
 */

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		else if (button == GLFW_MOUSE_BUTTON_LEFT) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
	}
}

/**
 * @brief Handles mouse scroll input to adjust the camera's zoom or focal settings.
 *
 * Passes the vertical scroll offset to the camera to update its viewport vectors accordingly.
 */

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	Camera* cameraPtr = static_cast<Camera*>(glfwGetWindowUserPointer(window));
	cameraPtr->updateViewportVectors(yoffset);
}

/**
 * @brief Handles keyboard input for camera movement, focus adjustment, and screenshot triggering.
 *
 * Processes user keypresses using GLFW to update the camera's movement direction and defocus state.
 * Also adjusts rendering parameters (e.g. rays per pixel) and sets a flag when a screenshot is requested.
 *
 * @param window         Pointer to the GLFW window.
 * @param camera         Reference to the active camera object.
 * @param dt             Delta time since the last frame (for smooth input scaling).
 * @param isScreenshot   Output flag set to true if Ctrl+S is pressed.
 */

void keyBoardInput(GLFWwindow* window, Camera& camera, float dt, bool& isScreenshot)
{
	uint8_t inputBits = 0;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE))
	{
		glfwSetWindowShouldClose(window, true);
		return;
	}
	if (glfwGetKey(window, GLFW_KEY_W) && !glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
		inputBits |= Camera::FORWARD;
	if (glfwGetKey(window, GLFW_KEY_S) && !glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
		inputBits |= Camera::BACKWARD;
	if (glfwGetKey(window, GLFW_KEY_A))
		inputBits |= Camera::LEFT;
	if (glfwGetKey(window, GLFW_KEY_D))
		inputBits |= Camera::RIGHT;
	if (glfwGetKey(window, GLFW_KEY_W) && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
		inputBits |= Camera::UP;
	if (glfwGetKey(window, GLFW_KEY_S) && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
		inputBits |= Camera::DOWN;
	if (glfwGetKey(window, GLFW_KEY_Z) && !glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
		inputBits |= Camera::DEFOCUS_UP;
	if (glfwGetKey(window, GLFW_KEY_Z) && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
		inputBits |= Camera::DEFOCUS_DOWN;
	if (glfwGetKey(window, GLFW_KEY_X) && !glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
	{
		numRaysPerPixel += RAYS_PER_PIXEL_SENSITIVITY * dt;
		numRaysPerPixel = clamp(numRaysPerPixel, 1.1f, 200.0f);
	}
	if (glfwGetKey(window, GLFW_KEY_X) && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
	{
		numRaysPerPixel -= RAYS_PER_PIXEL_SENSITIVITY * dt;
		numRaysPerPixel = clamp(numRaysPerPixel, 1.1f, 200.0f);
	}

	camera.keyboardInput(inputBits, dt);
	isScreenshot = (glfwGetKey(window, GLFW_KEY_S) && glfwGetKey(window, GLFW_KEY_LEFT_CONTROL));
}
/**
 * @brief Adds a large rectangular skylight above the scene to simulate ambient lighting.
 *
 * This function computes the scene's bounding box based on the existing BVH geometry and creates
 * a large flat rectangular light source (a "sky light") above the scene. The light is positioned
 * above the top of the scene's bounding box, offset by 30% of the scene's vertical size.
 *
 * The light is created as a horizontal plane composed of two triangles. Both RTX and BVH triangle
 * vectors are updated with the resulting geometry. Notably, the triangles are inserted **twice**
 * into both vectors—potentially to increase light contribution or intensity.
 *
 * @param rtxTriangles     Output vector of RTXTriangle objects to append the skylight geometry.
 * @param bvhTriangles     Output vector of BVHTriangle objects to append the skylight geometry.
 * @param lightMtlIndex    Material index to assign to the skylight (typically emissive).
 */

void addSkyLightPlane(std::vector<RTXTriangle>& rtxTriangles, std::vector<BVHTriangle>& bvhTriangles, int lightMtlIndex)
{
	BoundingBox sceneBounds;
	for (const BVHTriangle& tri : bvhTriangles)
		sceneBounds.growToInclude(tri);

	glm::vec3 sceneSize = sceneBounds.size();
	float minX = sceneBounds.min.x;
	float maxX = sceneBounds.max.x;
	float minY = sceneBounds.min.y;
	float maxY = sceneBounds.max.y;
	float minZ = sceneBounds.min.z;
	float maxZ = sceneBounds.max.z;

	float planeY = maxY + sceneSize.y * 0.3f;

	glm::vec3 boxSize = glm::vec3(maxX, maxY, maxZ) - glm::vec3(minX, minY, minZ);

	std::vector<glm::vec3> skyCorners =
	{
		glm::vec3(minX, planeY, maxZ),
		glm::vec3(maxX, planeY, maxZ),
		glm::vec3(minX, planeY, minZ),
		glm::vec3(maxX, planeY, minZ),
	};

	int skyCornersIndicies[2][3] =
	{
		{0, 3, 1},
		{0, 2, 3}
	};

	std::vector<RTXTriangle> skyTrianglesRTX;
	for (int i = 0; i < 2; i++)
		skyTrianglesRTX.push_back(RTXTriangle(lightMtlIndex, glm::vec4(skyCorners[skyCornersIndicies[i][0]], 0.0f), glm::vec4(skyCorners[skyCornersIndicies[i][1]], 0.0f),
			glm::vec4(skyCorners[skyCornersIndicies[i][2]], 0.0f), glm::vec2(), glm::vec2(), glm::vec2()));
	std::vector<BVHTriangle> skyTrianglesBVH;
	for (int i = 0; i < 2; i++)
		skyTrianglesBVH.push_back(BVHTriangle(skyCorners[skyCornersIndicies[i][0]], skyCorners[skyCornersIndicies[i][1]], skyCorners[skyCornersIndicies[i][2]]));

	rtxTriangles.insert(rtxTriangles.end(), skyTrianglesRTX.begin(), skyTrianglesRTX.end());
	bvhTriangles.insert(bvhTriangles.end(), skyTrianglesBVH.begin(), skyTrianglesBVH.end());
	rtxTriangles.insert(rtxTriangles.end(), skyTrianglesRTX.begin(), skyTrianglesRTX.end());
	bvhTriangles.insert(bvhTriangles.end(), skyTrianglesBVH.begin(), skyTrianglesBVH.end());
}


/**
 * @brief Adds a Cornell Box geometry around an existing scene for lighting and benchmarking purposes.
 *
 * This function generates the six walls of a Cornell Box (a standard test scene in computer graphics)
 * and optionally adds a rectangular area light at the top. It calculates the bounding box of the
 * existing scene (using the provided BVH triangles), expands it with a padding factor, and constructs
 * the Cornell Box around it.
 *
 * The box is made of 12 triangles (2 per face), and the light is constructed using 4 triangles
 * forming a rectangular area. The resulting triangles are added to both the RTX and BVH triangle
 * buffers for rendering and acceleration structure usage.
 *
 * @param rtxTriangles      Output vector of RTXTriangle structures to append the Cornell box geometry.
 * @param bvhTriangles      Output vector of BVHTriangle structures to append the Cornell box geometry.
 * @param lightSize         Relative size (0–1) of the area light as a percentage of the box width/depth.
 * @param padPercentage     Percentage of scene bounding box dimensions to expand for box spacing.
 * @param lightMtlIndex     Material index to apply to the light geometry (for emissive surfaces).
 * @param lightEnabled      Whether to add the ceiling light geometry to the scene.
 */

void addCornellBox(std::vector<RTXTriangle>& rtxTriangles, std::vector<BVHTriangle>& bvhTriangles, float lightSize, float padPercentage, int lightMtlIndex, bool lightEnabled)
{
	BoundingBox sceneBounds;
	for (const BVHTriangle& tri : bvhTriangles)
		sceneBounds.growToInclude(tri);

	glm::vec3 sceneSize = sceneBounds.size();
	float minX = sceneBounds.min.x - sceneSize.x * padPercentage;
	float maxX = sceneBounds.max.x + sceneSize.x * padPercentage;
	float minY = sceneBounds.min.y - sceneSize.y * padPercentage * 0.1f;
	float maxY = sceneBounds.max.y + sceneSize.y * padPercentage;
	float minZ = sceneBounds.min.z - sceneSize.z * padPercentage;
	float maxZ = sceneBounds.max.z + sceneSize.z * padPercentage;

	std::vector<glm::vec3> corners =
	{
		glm::vec3(minX, minY, maxZ),
		glm::vec3(maxX, minY, maxZ),
		glm::vec3(minX, maxY, maxZ),
		glm::vec3(maxX, maxY, maxZ),
		glm::vec3(minX, minY, minZ),
		glm::vec3(maxX, minY, minZ),
		glm::vec3(minX, maxY, minZ),
		glm::vec3(maxX, maxY, minZ),
	};

	glm::vec3 boxSize = glm::vec3(maxX, maxY, maxZ) - glm::vec3(minX, minY, minZ);
	float centerX = (maxX + minX) / 2.0f;
	float centerZ = (maxZ + minZ) / 2.0f;
	float lightMinX = centerX - lightSize * boxSize.x / 2.0f;
	float lightMaxX = centerX + lightSize * boxSize.x / 2.0f;
	float lightMinZ = centerZ - lightSize * boxSize.z / 2.0f;
	float lightMaxZ = centerZ + lightSize * boxSize.z / 2.0f;
	float lightY = maxY - 1e-3f;

	std::vector<glm::vec3> lightCorners =
	{
		glm::vec3(lightMinX, lightY, lightMaxZ),
		glm::vec3(lightMaxX, lightY, lightMaxZ),
		glm::vec3(lightMinX, lightY, lightMinZ),
		glm::vec3(lightMaxX, lightY, lightMinZ),
	};

	int triCornersIndicies[12][3] =
	{
		{0, 3, 1},
		{0, 2, 3},
		{0, 5, 4},
		{0, 1, 5},
		{0, 6, 2},
		{0, 4, 6},
		{7, 1, 3},
		{7, 5, 1},
		{7, 2, 6},
		{7, 3, 2},
		{7, 4, 5},
		{7, 6, 4}
	};

	int lightCornersIndicies[4][3] =
	{
		{0, 3, 1},
		{0, 2, 3},
		{0, 1, 3},
		{0, 3, 2}
	};

	std::vector<RTXTriangle> cornellCornersRTX;
	for (int i = 0; i < 12; i++)
	{
		Material mat = Material();
		cornellCornersRTX.push_back(RTXTriangle(0, glm::vec4(corners[triCornersIndicies[i][0]], 0.0f), glm::vec4(corners[triCornersIndicies[i][1]], 0.0f),
			glm::vec4(corners[triCornersIndicies[i][2]], 0.0f), glm::vec2(), glm::vec2(), glm::vec2()));
	}
	std::vector<BVHTriangle> cornellCornersBVH;
	for (int i = 0; i < 12; i++)
		cornellCornersBVH.push_back(BVHTriangle(corners[triCornersIndicies[i][0]], corners[triCornersIndicies[i][1]], corners[triCornersIndicies[i][2]]));

	std::vector<RTXTriangle> cornellLightRTX;
	for (int i = 0; i < 4; i++)
		cornellLightRTX.push_back(RTXTriangle(lightMtlIndex, glm::vec4(lightCorners[lightCornersIndicies[i][0]], 0.0f), glm::vec4(lightCorners[lightCornersIndicies[i][1]], 0.0f),
			glm::vec4(lightCorners[lightCornersIndicies[i][2]], 0.0f), glm::vec2(), glm::vec2(), glm::vec2()));
	std::vector<BVHTriangle> cornellLightBVH;
	for (int i = 0; i < 4; i++)
		cornellLightBVH.push_back(BVHTriangle(lightCorners[triCornersIndicies[i][0]], lightCorners[triCornersIndicies[i][1]], lightCorners[triCornersIndicies[i][2]]));

	rtxTriangles.insert(rtxTriangles.end(), cornellCornersRTX.begin(), cornellCornersRTX.end());
	bvhTriangles.insert(bvhTriangles.end(), cornellCornersBVH.begin(), cornellCornersBVH.end());

	if (lightEnabled)
	{
		rtxTriangles.insert(rtxTriangles.end(), cornellLightRTX.begin(), cornellLightRTX.end());
		bvhTriangles.insert(bvhTriangles.end(), cornellLightBVH.begin(), cornellLightBVH.end());
	}
}

// Function 1: Mirror Cornell Box (all walls are specular mirrors) - Fixed ceiling light
/**
 * @brief Adds a mirrored Cornell Box around the current scene bounds with an optional ceiling light.
 *
 * This function constructs a Cornell Box—a cube-like enclosure—around the provided scene using
 * the bounds of the existing BVH triangles, with padding applied to ensure spacing. Unlike the
 * standard Cornell Box, all interior walls in this variant are assigned a mirror-like material
 * to simulate highly reflective surfaces. A rectangular area light is also placed at the ceiling,
 * centered over the scene.
 *
 * The function generates 12 triangles for the six walls of the Cornell Box and 2 triangles for
 * the ceiling light. All triangles are appended to the RTX triangle buffer (for rendering) and the
 * BVH triangle buffer (for acceleration structure construction).
 *
 * @param rtxTriangles     Output vector of RTXTriangle structures to append the mirrored box and light geometry.
 * @param bvhTriangles     Output vector of BVHTriangle structures to append the mirrored box and light geometry.
 * @param lightSize        Relative size (0–1) of the square ceiling light, as a percentage of scene height.
 * @param padPercentage    Percentage of the scene's dimensions used to expand the bounding box for box placement.
 * @param lightMtlIndex    Material index to assign to the ceiling light surface (emissive).
 * @param mirrorMtlIndex   Material index to assign to all Cornell Box wall surfaces (mirror-like).
 */

void addMirrorCornellBox(std::vector<RTXTriangle>& rtxTriangles, std::vector<BVHTriangle>& bvhTriangles, float lightSize, float padPercentage, int lightMtlIndex, int mirrorMtlIndex)
{
	BoundingBox sceneBounds;
	for (const BVHTriangle& tri : bvhTriangles)
		sceneBounds.growToInclude(tri);

	glm::vec3 sceneSize = sceneBounds.size();
	float minX = sceneBounds.min.x - sceneSize.x * padPercentage;
	float maxX = sceneBounds.max.x + sceneSize.x * padPercentage;
	float minY = sceneBounds.min.y - sceneSize.y * padPercentage * 0.1f;
	float maxY = sceneBounds.max.y + sceneSize.y * padPercentage;
	float minZ = sceneBounds.min.z - sceneSize.z * padPercentage;
	float maxZ = sceneBounds.max.z + sceneSize.z * padPercentage;

	std::vector<glm::vec3> corners =
	{
		glm::vec3(minX, minY, maxZ), // 0
		glm::vec3(maxX, minY, maxZ), // 1
		glm::vec3(minX, maxY, maxZ), // 2
		glm::vec3(maxX, maxY, maxZ), // 3
		glm::vec3(minX, minY, minZ), // 4
		glm::vec3(maxX, minY, minZ), // 5
		glm::vec3(minX, maxY, minZ), // 6
		glm::vec3(maxX, maxY, minZ), // 7
	};

	// Ceiling light position (centered on ceiling)
	float lightCenterX = (maxX + minX) / 2.0f;
	float lightCenterZ = (maxZ + minZ) / 2.0f;
	float lightHalfSize = lightSize * sceneSize.y / 2.0f;
	float lightOffset = 1e-3f;

	// Ceiling light corners
	std::vector<glm::vec3> lightCorners =
	{
		glm::vec3(lightCenterX - lightHalfSize, maxY - lightOffset, lightCenterZ + lightHalfSize),
		glm::vec3(lightCenterX + lightHalfSize, maxY - lightOffset, lightCenterZ + lightHalfSize),
		glm::vec3(lightCenterX - lightHalfSize, maxY - lightOffset, lightCenterZ - lightHalfSize),
		glm::vec3(lightCenterX + lightHalfSize, maxY - lightOffset, lightCenterZ - lightHalfSize),
	};

	// Triangle indices with proper winding for viewer at origin looking at -Z
	int triCornersIndicies[12][3] =
	{
		// Front face (maxZ)
		{0, 3, 1}, {0, 2, 3},
		// Bottom face (minY)
		{0, 5, 4}, {0, 1, 5},
		// Left face (minX)
		{0, 6, 2}, {0, 4, 6},
		// Right face (maxX)
		{1, 7, 5}, {1, 3, 7},
		// Top face (maxY)
		{2, 7, 3}, {2, 6, 7},
		// Back face (minZ)
		{4, 7, 6}, {4, 5, 7}
	};

	int lightCornersIndicies[2][3] = { {0, 1, 2}, {1, 3, 2} };

	// Create mirror walls - ALL walls use mirrorMtlIndex
	std::vector<RTXTriangle> cornellCornersRTX;
	for (int i = 0; i < 12; i++)
	{
		cornellCornersRTX.push_back(RTXTriangle(mirrorMtlIndex,
			glm::vec4(corners[triCornersIndicies[i][0]], 0.0f),
			glm::vec4(corners[triCornersIndicies[i][1]], 0.0f),
			glm::vec4(corners[triCornersIndicies[i][2]], 0.0f),
			glm::vec2(), glm::vec2(), glm::vec2()));
	}

	std::vector<BVHTriangle> cornellCornersBVH;
	for (int i = 0; i < 12; i++)
		cornellCornersBVH.push_back(BVHTriangle(corners[triCornersIndicies[i][0]],
			corners[triCornersIndicies[i][1]], corners[triCornersIndicies[i][2]]));

	// Add ceiling light
	std::vector<RTXTriangle> lightRTX;
	for (int i = 0; i < 2; i++)
		lightRTX.push_back(RTXTriangle(lightMtlIndex,
			glm::vec4(lightCorners[lightCornersIndicies[i][0]], 0.0f),
			glm::vec4(lightCorners[lightCornersIndicies[i][1]], 0.0f),
			glm::vec4(lightCorners[lightCornersIndicies[i][2]], 0.0f),
			glm::vec2(), glm::vec2(), glm::vec2()));

	std::vector<BVHTriangle> lightBVH;
	for (int i = 0; i < 2; i++)
		lightBVH.push_back(BVHTriangle(lightCorners[lightCornersIndicies[i][0]],
			lightCorners[lightCornersIndicies[i][1]], lightCorners[lightCornersIndicies[i][2]]));

	// Add all triangles
	rtxTriangles.insert(rtxTriangles.end(), cornellCornersRTX.begin(), cornellCornersRTX.end());
	bvhTriangles.insert(bvhTriangles.end(), cornellCornersBVH.begin(), cornellCornersBVH.end());
	rtxTriangles.insert(rtxTriangles.end(), lightRTX.begin(), lightRTX.end());
	bvhTriangles.insert(bvhTriangles.end(), lightBVH.begin(), lightBVH.end());
}

// Function 2: Cornell Box with lights on both sides - with optional rotation
/**
 * @brief Adds a Cornell Box with side lighting to the scene geometry.
 *
 * This function builds a Cornell Box enclosure around an existing scene using the bounds of
 * the current BVH triangles and adds two rectangular area lights on opposing side walls.
 * The lights can be placed either on the left/right walls (default) or on the front/back
 * walls if the `rotate` parameter is enabled.
 *
 * The Cornell Box is composed of 12 triangles (2 per face), and each area light consists of
 * 2 triangles. The function appends all generated geometry to both the RTX triangle list
 * (for rendering purposes) and the BVH triangle list (for spatial acceleration).
 *
 * The materials used for the box walls and lights are provided via material indices.
 * Padding is applied around the scene bounds to ensure the box encloses the scene with margin.
 *
 * @param rtxTriangles     Output vector of RTXTriangle objects to append box and light geometry.
 * @param bvhTriangles     Output vector of BVHTriangle objects to append box and light geometry.
 * @param lightSize        Relative size (0–1) of the side lights as a percentage of scene height.
 * @param padPercentage    Amount of padding to apply around the scene bounds for the box (as a fraction).
 * @param lightMtlIndex    Material index to assign to the side light surfaces.
 * @param wallMtlIndex     Material index to assign to all Cornell Box walls.
 * @param rotate           If true, places the lights on front/back walls instead of left/right.
 */

void addSideLitCornellBox(std::vector<RTXTriangle>& rtxTriangles, std::vector<BVHTriangle>& bvhTriangles, float lightSize, float padPercentage, int lightMtlIndex, int wallMtlIndex, bool rotate = false)
{
	BoundingBox sceneBounds;
	for (const BVHTriangle& tri : bvhTriangles)
		sceneBounds.growToInclude(tri);

	glm::vec3 sceneSize = sceneBounds.size();
	float minX = sceneBounds.min.x - sceneSize.x * padPercentage;
	float maxX = sceneBounds.max.x + sceneSize.x * padPercentage;
	float minY = sceneBounds.min.y - sceneSize.y * padPercentage * 0.1f;
	float maxY = sceneBounds.max.y + sceneSize.y * padPercentage;
	float minZ = sceneBounds.min.z - sceneSize.z * padPercentage;
	float maxZ = sceneBounds.max.z + sceneSize.z * padPercentage;

	std::vector<glm::vec3> corners =
	{
		glm::vec3(minX, minY, maxZ), // 0
		glm::vec3(maxX, minY, maxZ), // 1
		glm::vec3(minX, maxY, maxZ), // 2
		glm::vec3(maxX, maxY, maxZ), // 3
		glm::vec3(minX, minY, minZ), // 4
		glm::vec3(maxX, minY, minZ), // 5
		glm::vec3(minX, maxY, minZ), // 6
		glm::vec3(maxX, maxY, minZ), // 7
	};

	// Light positions - either left/right OR front/back based on rotate parameter
	float lightCenterY = (maxY + minY) / 2.0f;
	float lightHalfSize = lightSize * sceneSize.y / 2.0f;
	float lightOffset = 1e-3f;

	std::vector<glm::vec3> firstLightCorners, secondLightCorners;
	int firstLightIndicies[2][3], secondLightIndicies[2][3];

	if (!rotate) {
		// Original behavior: Left and Right side lights
		float lightCenterZ = (maxZ + minZ) / 2.0f;

		// Left side light (on minX wall)
		firstLightCorners = {
			glm::vec3(minX + lightOffset, lightCenterY - lightHalfSize, lightCenterZ + lightHalfSize),
			glm::vec3(minX + lightOffset, lightCenterY + lightHalfSize, lightCenterZ + lightHalfSize),
			glm::vec3(minX + lightOffset, lightCenterY - lightHalfSize, lightCenterZ - lightHalfSize),
			glm::vec3(minX + lightOffset, lightCenterY + lightHalfSize, lightCenterZ - lightHalfSize),
		};

		// Right side light (on maxX wall)
		secondLightCorners = {
			glm::vec3(maxX - lightOffset, lightCenterY - lightHalfSize, lightCenterZ + lightHalfSize),
			glm::vec3(maxX - lightOffset, lightCenterY + lightHalfSize, lightCenterZ + lightHalfSize),
			glm::vec3(maxX - lightOffset, lightCenterY - lightHalfSize, lightCenterZ - lightHalfSize),
			glm::vec3(maxX - lightOffset, lightCenterY + lightHalfSize, lightCenterZ - lightHalfSize),
		};

		// Light triangle indices (facing inward toward viewer)
		int leftLightIdx[2][3] = { {0, 2, 1}, {1, 2, 3} };   // Left light faces right (+X)
		int rightLightIdx[2][3] = { {0, 1, 2}, {1, 3, 2} };  // Right light faces left (-X)

		memcpy(firstLightIndicies, leftLightIdx, sizeof(leftLightIdx));
		memcpy(secondLightIndicies, rightLightIdx, sizeof(rightLightIdx));
	}
	else {
		// Rotated behavior: Front and Back lights
		float lightCenterX = (maxX + minX) / 2.0f;

		// Front light (on maxZ wall)
		firstLightCorners = {
			glm::vec3(lightCenterX - lightHalfSize, lightCenterY - lightHalfSize, maxZ - lightOffset),
			glm::vec3(lightCenterX + lightHalfSize, lightCenterY - lightHalfSize, maxZ - lightOffset),
			glm::vec3(lightCenterX - lightHalfSize, lightCenterY + lightHalfSize, maxZ - lightOffset),
			glm::vec3(lightCenterX + lightHalfSize, lightCenterY + lightHalfSize, maxZ - lightOffset),
		};

		// Back light (on minZ wall)
		secondLightCorners = {
			glm::vec3(lightCenterX - lightHalfSize, lightCenterY - lightHalfSize, minZ + lightOffset),
			glm::vec3(lightCenterX + lightHalfSize, lightCenterY - lightHalfSize, minZ + lightOffset),
			glm::vec3(lightCenterX - lightHalfSize, lightCenterY + lightHalfSize, minZ + lightOffset),
			glm::vec3(lightCenterX + lightHalfSize, lightCenterY + lightHalfSize, minZ + lightOffset),
		};

		// Light triangle indices (facing inward toward viewer)
		int frontLightIdx[2][3] = { {0, 2, 1}, {1, 2, 3} };  // Front light faces back (-Z)
		int backLightIdx[2][3] = { {0, 1, 2}, {1, 3, 2} };   // Back light faces front (+Z)

		memcpy(firstLightIndicies, frontLightIdx, sizeof(frontLightIdx));
		memcpy(secondLightIndicies, backLightIdx, sizeof(backLightIdx));
	}

	// Triangle indices with proper winding for viewer at origin looking at -Z
	int triCornersIndicies[12][3] =
	{
		// Front face (maxZ)
		{0, 3, 1}, {0, 2, 3},
		// Bottom face (minY)
		{0, 5, 4}, {0, 1, 5},
		// Left face (minX)
		{0, 6, 2}, {0, 4, 6},
		// Right face (maxX)
		{1, 7, 5}, {1, 3, 7},
		// Top face (maxY)
		{2, 7, 3}, {2, 6, 7},
		// Back face (minZ)
		{4, 7, 6}, {4, 5, 7}
	};

	// Create walls with specified material
	std::vector<RTXTriangle> cornellCornersRTX;
	for (int i = 0; i < 12; i++)
	{
		cornellCornersRTX.push_back(RTXTriangle(wallMtlIndex,
			glm::vec4(corners[triCornersIndicies[i][0]], 0.0f),
			glm::vec4(corners[triCornersIndicies[i][1]], 0.0f),
			glm::vec4(corners[triCornersIndicies[i][2]], 0.0f),
			glm::vec2(), glm::vec2(), glm::vec2()));
	}

	std::vector<BVHTriangle> cornellCornersBVH;
	for (int i = 0; i < 12; i++)
		cornellCornersBVH.push_back(BVHTriangle(corners[triCornersIndicies[i][0]],
			corners[triCornersIndicies[i][1]], corners[triCornersIndicies[i][2]]));

	// Add FIRST light (left or front depending on rotation)
	std::vector<RTXTriangle> firstLightRTX;
	for (int i = 0; i < 2; i++)
		firstLightRTX.push_back(RTXTriangle(lightMtlIndex,
			glm::vec4(firstLightCorners[firstLightIndicies[i][0]], 0.0f),
			glm::vec4(firstLightCorners[firstLightIndicies[i][1]], 0.0f),
			glm::vec4(firstLightCorners[firstLightIndicies[i][2]], 0.0f),
			glm::vec2(), glm::vec2(), glm::vec2()));

	std::vector<BVHTriangle> firstLightBVH;
	for (int i = 0; i < 2; i++)
		firstLightBVH.push_back(BVHTriangle(firstLightCorners[firstLightIndicies[i][0]],
			firstLightCorners[firstLightIndicies[i][1]], firstLightCorners[firstLightIndicies[i][2]]));

	// Add SECOND light (right or back depending on rotation)
	std::vector<RTXTriangle> secondLightRTX;
	for (int i = 0; i < 2; i++)
		secondLightRTX.push_back(RTXTriangle(lightMtlIndex,
			glm::vec4(secondLightCorners[secondLightIndicies[i][0]], 0.0f),
			glm::vec4(secondLightCorners[secondLightIndicies[i][1]], 0.0f),
			glm::vec4(secondLightCorners[secondLightIndicies[i][2]], 0.0f),
			glm::vec2(), glm::vec2(), glm::vec2()));

	std::vector<BVHTriangle> secondLightBVH;
	for (int i = 0; i < 2; i++)
		secondLightBVH.push_back(BVHTriangle(secondLightCorners[secondLightIndicies[i][0]],
			secondLightCorners[secondLightIndicies[i][1]], secondLightCorners[secondLightIndicies[i][2]]));

	// Add all triangles
	rtxTriangles.insert(rtxTriangles.end(), cornellCornersRTX.begin(), cornellCornersRTX.end());
	bvhTriangles.insert(bvhTriangles.end(), cornellCornersBVH.begin(), cornellCornersBVH.end());
	rtxTriangles.insert(rtxTriangles.end(), firstLightRTX.begin(), firstLightRTX.end());
	bvhTriangles.insert(bvhTriangles.end(), firstLightBVH.begin(), firstLightBVH.end());
	rtxTriangles.insert(rtxTriangles.end(), secondLightRTX.begin(), secondLightRTX.end());
	bvhTriangles.insert(bvhTriangles.end(), secondLightBVH.begin(), secondLightBVH.end());
}

// Helper function to add a cube - Fixed triangle winding
/**
 * @brief Adds a rotated, axis-aligned cube to the scene geometry.
 *
 * This function generates a cube based on the specified
 * center position, size, and rotation. The cube is constructed locally around the origin,
 * transformed by the given Euler rotation (in radians), and then translated to the desired center.
 *
 * Each triangle is added to both the RTX triangle list (for rendering or shading) and the BVH
 * triangle list (for acceleration structure construction). The triangles are wound counter-clockwise
 * when viewed from outside the cube, ensuring correct surface normals.
 *
 * @param rtxTriangles     Output vector of RTXTriangle structures to append the cube geometry.
 * @param bvhTriangles     Output vector of BVHTriangle structures to append the cube geometry.
 * @param center           World-space position to place the center of the cube.
 * @param size             Width, height, and depth of the cube.
 * @param rotation         Euler angles (in radians) to rotate the cube around X, Y, and Z axes.
 * @param materialIndex    Material index to assign to all cube triangles in the RTX triangle buffer.
 */

void addCube(std::vector<RTXTriangle>& rtxTriangles, std::vector<BVHTriangle>& bvhTriangles,
	glm::vec3 center, glm::vec3 size, glm::vec3 rotation, int materialIndex)
{
	// Create rotation matrices
	glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1, 0, 0));
	glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0, 1, 0));
	glm::mat4 rotZ = glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0, 0, 1));
	glm::mat4 rotMatrix = rotZ * rotY * rotX;

	glm::vec3 halfSize = size * 0.5f;

	// Define cube vertices
	std::vector<glm::vec3> vertices = {
		glm::vec3(-halfSize.x, -halfSize.y, +halfSize.z), // 0
		glm::vec3(+halfSize.x, -halfSize.y, +halfSize.z), // 1
		glm::vec3(-halfSize.x, +halfSize.y, +halfSize.z), // 2
		glm::vec3(+halfSize.x, +halfSize.y, +halfSize.z), // 3
		glm::vec3(-halfSize.x, -halfSize.y, -halfSize.z), // 4
		glm::vec3(+halfSize.x, -halfSize.y, -halfSize.z), // 5
		glm::vec3(-halfSize.x, +halfSize.y, -halfSize.z), // 6
		glm::vec3(+halfSize.x, +halfSize.y, -halfSize.z), // 7
	};

	// Apply rotation and translation
	for (auto& vertex : vertices) {
		glm::vec4 rotated = rotMatrix * glm::vec4(vertex, 1.0f);
		vertex = glm::vec3(rotated) + center;
	}

	// Cube face indices with proper winding (counter-clockwise when viewed from outside)
	int faceIndices[12][3] = {
		// Front face (+Z)
		{0, 1, 3}, {0, 3, 2},
		// Right face (+X)
		{1, 5, 7}, {1, 7, 3},
		// Back face (-Z)
		{5, 4, 6}, {5, 6, 7},
		// Left face (-X)
		{4, 0, 2}, {4, 2, 6},
		// Top face (+Y)
		{2, 3, 7}, {2, 7, 6},
		// Bottom face (-Y)
		{4, 5, 1}, {4, 1, 0}
	};

	// Add triangles
	for (int i = 0; i < 12; i++) {
		rtxTriangles.push_back(RTXTriangle(materialIndex,
			glm::vec4(vertices[faceIndices[i][0]], 0.0f),
			glm::vec4(vertices[faceIndices[i][1]], 0.0f),
			glm::vec4(vertices[faceIndices[i][2]], 0.0f),
			glm::vec2(), glm::vec2(), glm::vec2()));

		bvhTriangles.push_back(BVHTriangle(vertices[faceIndices[i][0]],
			vertices[faceIndices[i][1]], vertices[faceIndices[i][2]]));
	}
}

// Function 3: Classic Cornell Box scene generator - Fixed face orientations
/**
 * @brief Constructs a classic Cornell Box scene with colored walls, a ceiling light, and two interior cubes.
 *
 * This function generates the geometry for a standardized Cornell Box test scene commonly used in global
 * illumination and path tracing research. The scene is centered at the origin, and the camera is expected
 * to look down the -Z axis.
 *
 * The generated geometry includes:
 * - A cubic room with six walls: red left wall, green right wall, and white front, back, floor, and ceiling.
 * - A rectangular area light on the ceiling.
 * - Two white interior cubes (short and tall) with positions, sizes, and rotations modeled after the
 *   original Cornell Box specification.
 *
 * All triangles are appended to both the RTX and BVH triangle vectors for rendering and acceleration structure use.
 * This function is typically used as a foundation for more advanced scene setups.
 *
 * @param rtxTriangles     Output vector of RTXTriangle objects for rendering.
 * @param bvhTriangles     Output vector of BVHTriangle objects for spatial acceleration.
 * @param roomSize         Side length of the cubic room (scene is centered around origin).
 * @param redMtlIndex      Material index for the left wall (typically diffuse red).
 * @param greenMtlIndex    Material index for the right wall (typically diffuse green).
 * @param whiteMtlIndex    Material index for white walls and interior cubes.
 * @param lightMtlIndex    Material index for the ceiling light (typically emissive).
 */

void createClassicCornellBox(std::vector<RTXTriangle>& rtxTriangles, std::vector<BVHTriangle>& bvhTriangles, float roomSize, int redMtlIndex, int greenMtlIndex, int whiteMtlIndex, int lightMtlIndex)
{
	float half = roomSize * 0.5f;

	// Room corners
	std::vector<glm::vec3> corners = {
		glm::vec3(-half, -half, +half), // 0
		glm::vec3(+half, -half, +half), // 1
		glm::vec3(-half, +half, +half), // 2
		glm::vec3(+half, +half, +half), // 3
		glm::vec3(-half, -half, -half), // 4
		glm::vec3(+half, -half, -half), // 5
		glm::vec3(-half, +half, -half), // 6
		glm::vec3(+half, +half, -half), // 7
	};

	// Wall triangles with specific materials and correct winding
	struct WallTriangle { int indices[3]; int materialIndex; };

	std::vector<WallTriangle> walls = {
		// Front wall (white) - faces viewer
		{{0, 3, 1}, whiteMtlIndex}, {{0, 2, 3}, whiteMtlIndex},
		// Back wall (white) - faces viewer
		{{4, 7, 6}, whiteMtlIndex}, {{4, 5, 7}, whiteMtlIndex},
		// Floor (white) - faces up
		{{0, 5, 4}, whiteMtlIndex}, {{0, 1, 5}, whiteMtlIndex},
		// Ceiling (white) - faces down
		{{2, 6, 7}, whiteMtlIndex}, {{2, 7, 3}, whiteMtlIndex},
		// Left wall (red) - faces right
		{{0, 4, 6}, redMtlIndex}, {{0, 6, 2}, redMtlIndex},
		// Right wall (green) - faces left  
		{{1, 3, 7}, greenMtlIndex}, {{1, 7, 5}, greenMtlIndex}
	};

	// Add walls
	for (const auto& wall : walls) {
		rtxTriangles.push_back(RTXTriangle(wall.materialIndex,
			glm::vec4(corners[wall.indices[0]], 0.0f),
			glm::vec4(corners[wall.indices[1]], 0.0f),
			glm::vec4(corners[wall.indices[2]], 0.0f),
			glm::vec2(), glm::vec2(), glm::vec2()));

		bvhTriangles.push_back(BVHTriangle(corners[wall.indices[0]],
			corners[wall.indices[1]], corners[wall.indices[2]]));
	}

	// Add ceiling light - original proportions: 130x105 units in 555 unit box
	float lightWidth = roomSize * (130.0f / 555.0f);   // ~0.234
	float lightDepth = roomSize * (105.0f / 555.0f);   // ~0.189
	float lightY = half - 0.001f;
	std::vector<glm::vec3> lightCorners = {
		glm::vec3(-lightWidth * 0.5f, lightY, +lightDepth * 0.5f),
		glm::vec3(+lightWidth * 0.5f, lightY, +lightDepth * 0.5f),
		glm::vec3(-lightWidth * 0.5f, lightY, -lightDepth * 0.5f),
		glm::vec3(+lightWidth * 0.5f, lightY, -lightDepth * 0.5f),
	};

	int lightIndices[2][3] = { {0, 2, 1}, {1, 2, 3} };
	for (int i = 0; i < 2; i++) {
		rtxTriangles.push_back(RTXTriangle(lightMtlIndex,
			glm::vec4(lightCorners[lightIndices[i][0]], 0.0f),
			glm::vec4(lightCorners[lightIndices[i][1]], 0.0f),
			glm::vec4(lightCorners[lightIndices[i][2]], 0.0f),
			glm::vec2(), glm::vec2(), glm::vec2()));

		bvhTriangles.push_back(BVHTriangle(lightCorners[lightIndices[i][0]],
			lightCorners[lightIndices[i][1]], lightCorners[lightIndices[i][2]]));
	}

	// Add two classic boxes with correct original proportions
	// Original Cornell Box coordinates: (0,0) at back-left corner, camera looks down +Z
	// In our coordinate system: camera at origin looks down -Z, so we need to flip Z coordinates

	float boxScale = 165.0f / 555.0f;  // ~0.297
	float boxSize = roomSize * boxScale;

	// Short box - position it on the RIGHT when looking down -Z
	float shortBoxX = half * 0.5f;   // Positive X = right side
	float shortBoxZ = -half * 0.3f;  // Slightly forward from center
	addCube(rtxTriangles, bvhTriangles,
		glm::vec3(shortBoxX, -half + boxSize * 0.5f, shortBoxZ),
		glm::vec3(boxSize, boxSize, boxSize),
		glm::vec3(0.0f, glm::radians(-18.0f), 0.0f), whiteMtlIndex);

	// Tall box - position it on the LEFT when looking down -Z
	float tallBoxX = -half * 0.3f;   // Negative X = left side
	float tallBoxZ = -half * 0.6f;   // Further back
	float tallBoxHeight = roomSize * (330.0f / 555.0f);     // ~0.594
	addCube(rtxTriangles, bvhTriangles,
		glm::vec3(tallBoxX, -half + tallBoxHeight * 0.5f, tallBoxZ),
		glm::vec3(boxSize, tallBoxHeight, boxSize),
		glm::vec3(0.0f, glm::radians(16.5f), 0.0f), whiteMtlIndex);
}

// Function 4: Diverse Cornell Box with various cubes
/**
 * @brief Builds an extended Cornell Box scene featuring a variety of materials and geometry for visual diversity and shading complexity.
 *
 * This function first constructs a classic Cornell Box using `createClassicCornellBox`, and then populates the scene
 * with a diverse set of additional cubes. These cubes vary in size, position, rotation, and material type to introduce
 * more challenging rendering scenarios, such as reflective, refractive, and textured surfaces.
 *
 * The additional geometry includes:
 * - Glass cubes with transparency and rotation
 * - Small mirrored cubes and clusters to simulate sphere-like reflections
 * - Checker-textured cubes and platforms
 * - Tall and tilted metal cubes to test specular highlights and anisotropy
 *
 * Each object is appended to the `rtxTriangles` and `bvhTriangles` vectors, which represent the geometry used for
 * rendering and acceleration structure traversal, respectively.
 *
 * @param rtxTriangles     Output vector of RTXTriangle objects for rendering.
 * @param bvhTriangles     Output vector of BVHTriangle objects for spatial acceleration (BVH).
 * @param roomSize         Side length of the Cornell Box room (scene is centered at the origin).
 * @param redMtlIndex      Material index for the left wall (diffuse red).
 * @param greenMtlIndex    Material index for the right wall (diffuse green).
 * @param whiteMtlIndex    Material index for white walls and standard cubes.
 * @param lightMtlIndex    Material index for the ceiling light (emissive).
 * @param glassMtlIndex    Material index for transparent glass cubes.
 * @param mirrorMtlIndex   Material index for reflective mirror cubes.
 * @param checkerMtlIndex  Material index for checkered-texture cubes or platforms.
 * @param metalMtlIndex    Material index for metallic cubes with specular surfaces.
 */

void createDiverseCornellBox(std::vector<RTXTriangle>& rtxTriangles, std::vector<BVHTriangle>& bvhTriangles, float roomSize, int redMtlIndex, int greenMtlIndex, int whiteMtlIndex, int lightMtlIndex,
	int glassMtlIndex, int mirrorMtlIndex, int checkerMtlIndex, int metalMtlIndex)
{
	// Start with classic cornell box
	createClassicCornellBox(rtxTriangles, bvhTriangles, roomSize, redMtlIndex, greenMtlIndex, whiteMtlIndex, lightMtlIndex);

	float half = roomSize * 0.5f;

	// Add diverse cubes with different materials, positions, rotations, and scales
	struct CubeSpec {
		glm::vec3 position;
		glm::vec3 size;
		glm::vec3 rotation;
		int materialIndex;
	};

	std::vector<CubeSpec> cubes = {
		// Glass cube
		{glm::vec3(-half * 0.7f, -half + 0.1f, half * 0.6f), glm::vec3(0.15f, 0.15f, 0.15f), glm::vec3(0.0f, 0.785f, 0.0f), glassMtlIndex},

		// Mirror sphere-like (small cubes)
		{glm::vec3(half * 0.6f, -half + 0.05f, -half * 0.4f), glm::vec3(0.08f, 0.08f, 0.08f), glm::vec3(0.2f, 0.5f, 0.3f), mirrorMtlIndex},

		// Checker cube
		{glm::vec3(0.0f, -half + 0.2f, -half * 0.7f), glm::vec3(0.25f, 0.4f, 0.25f), glm::vec3(0.0f, 0.0f, 0.1f), checkerMtlIndex},

		// Metal cube (tall and thin)
		{glm::vec3(half * 0.3f, -half + 0.3f, half * 0.2f), glm::vec3(0.1f, 0.6f, 0.1f), glm::vec3(0.1f, 1.2f, 0.0f), metalMtlIndex},

		// Another glass cube (different size and position)
		{glm::vec3(-half * 0.2f, -half + 0.15f, -half * 0.2f), glm::vec3(0.2f, 0.1f, 0.3f), glm::vec3(0.5f, 0.0f, 0.2f), glassMtlIndex},

		// Small mirror cubes cluster
		{glm::vec3(half * 0.8f, -half + 0.03f, half * 0.8f), glm::vec3(0.05f, 0.05f, 0.05f), glm::vec3(0.0f, 0.0f, 0.0f), mirrorMtlIndex},
		{glm::vec3(half * 0.75f, -half + 0.08f, half * 0.75f), glm::vec3(0.06f, 0.06f, 0.06f), glm::vec3(0.3f, 0.3f, 0.3f), mirrorMtlIndex},

		// Flat checker platform
		{glm::vec3(-half * 0.5f, -half + 0.02f, -half * 0.6f), glm::vec3(0.3f, 0.04f, 0.3f), glm::vec3(0.0f, 0.7f, 0.0f), checkerMtlIndex},

		// Tilted metal cube
		{glm::vec3(half * 0.1f, -half + 0.25f, half * 0.5f), glm::vec3(0.18f, 0.18f, 0.18f), glm::vec3(0.6f, 0.4f, 0.8f), metalMtlIndex},
	};

	// Add all the diverse cubes
	for (const auto& cube : cubes) {
		addCube(rtxTriangles, bvhTriangles, cube.position, cube.size, cube.rotation, cube.materialIndex);
	}
}

/**
 * @brief Callback to update the OpenGL viewport when the window is resized.
 */

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

GLfloat ndcVertices[] =
{
	// Top left
	-1.0f, -1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	// Bottom right
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	1.0f, 1.0f, 0.0f
};

/**
 * @brief Resets OpenGL state to a clean default configuration.
 *
 * Unbinds all texture/image units, buffers, framebuffers, and shader programs.
 * Clears OpenGL errors and issues a memory barrier followed by `glFinish()` to ensure GPU operations complete.
 */

void resetOpenGLState() {
	// Reset all texture units
	for (int i = 0; i < 32; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindImageTexture(i, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	}
	glActiveTexture(GL_TEXTURE0);

	// Reset buffers
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Reset framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Reset shader programs
	glUseProgram(0);

	// Clear any pending errors
	while (glGetError() != GL_NO_ERROR);

	// Memory barrier to ensure all operations complete
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	glFinish();
}
/**
 * @brief Forces a full GPU cleanup and synchronization of all pending OpenGL operations.
 *
 * This function ensures that all GPU-side rendering or compute operations are completed and that
 * the OpenGL state is reset to a clean baseline. It is useful when performing GPU context resets,
 * resource unloading, debugging operations, or preparing for major state transitions between rendering passes.
 *
 * The cleanup steps include:
 * - `glFinish()`: Blocks until all OpenGL commands have fully completed execution.
 * - `glMemoryBarrier(GL_ALL_BARRIER_BITS)`: Ensures memory visibility and ordering across all shader and buffer operations.
 * - `resetOpenGLState()`: Application-defined function that resets OpenGL bindings and state (see below).
 * - `glFlush()` and final `glFinish()`: Ensures all remaining commands are flushed and completed before proceeding.
 *
 * @note
 * The `resetOpenGLState()` function performs the following:
 * - Unbinds all 32 texture/image units and resets to `GL_TEXTURE0`.
 * - Unbinds `GL_SHADER_STORAGE_BUFFER`, `GL_UNIFORM_BUFFER`, and `GL_ARRAY_BUFFER`.
 * - Unbinds the framebuffer and currently active shader program.
 * - Clears all pending OpenGL errors using `glGetError()`.
 * - Issues a memory barrier and a `glFinish()` to ensure the pipeline is idle.
 */


void forceGPUCleanup() {
	// Force completion of all GPU operations
	glFinish();

	// Clear all caches
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	// Reset OpenGL state
	resetOpenGLState();

	// Force a context flush
	glFlush();
	glFinish();
}

int main(int argc, char* argv[])
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSwapInterval(0);


	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);


	forceGPUCleanup();

	// Loading mesh data
	std::vector<RTXTriangle> rtxTriangles;
	std::vector<BVHTriangle> bvhTriangles;
	std::vector<Material> materials;
	std::vector<Texture2D> textures;

	// Prompt user to select a directory
	const char* folderPath = tinyfd_selectFolderDialog("Select Model Folder", nullptr);

	if (!folderPath) {
		std::cerr << "No folder was selected. Exiting." << std::endl;
		return 1;
	}

	std::string modelFolderPath = folderPath;

	getTrianglesData_("Data\\" + modelFolderName, 1, rtxTriangles, bvhTriangles, materials, textures);

	Material red;
	red.makeDiffusive(glm::vec3(1.0f, 0.0f, 0.0f));
	materials.push_back(red);
	Material green;
	green.makeDiffusive(glm::vec3(0.0f, 1.0f, 0.0f));
	materials.push_back(green);

	Material wall;
	wall.makeDiffusive(glm::vec3(1.0f));
	materials.push_back(wall);
	Material light;
	light.makeLight(glm::vec3(1.0f), CORNELL_LIGHT_BRIGHTNESS);
	materials.push_back(light);
	Material mirror;
	mirror.makeSpecular(glm::vec3(1.0f), glm::vec3(1.0f), 1.0f, 1.0f);
	materials.push_back(mirror);

	// createClassicCornellBox(rtxTriangles, bvhTriangles, 10, materials.size() - 5, materials.size() - 4, materials.size() - 3, materials.size() - 2);
	// createDiverseCornellBox(rtxTriangles, bvhTriangles, 10, materials.size() - 5, materials.size() - 4, materials.size() - 3, materials.size() - 2);

	// addCornellBox(rtxTriangles, bvhTriangles, CORNELL_LIGHT_SIZE, CORNELL_PADDING, materials.size() - 2, true);
	// addMirrorCornellBox(rtxTriangles, bvhTriangles, CORNELL_LIGHT_SIZE, CORNELL_PADDING, materials.size() - 2, materials.size() - 1);
	// addSideLitCornellBox(rtxTriangles, bvhTriangles, CORNELL_LIGHT_SIZE, CORNELL_PADDING, materials.size() - 2, materials.size() - 3, 1);
	// addSkyLightPlane(rtxTriangles, bvhTriangles, materials.size() - 2);

	BVH BVH(bvhTriangles, rtxTriangles);

	// for (Material& mat : materials)
	// 	mat.addSpecular(1.0f, 0.02f);
		// mat.makeGlass(glm::vec3(1.0f), 1.6f);
		// mat.makeDiffusive(glm::vec3(0.8, 0.8, 0.8));

	// build and compile shaders
	// -------------------------
	std::string shaderFolderPath = getPath("Assets\\Shaders", 1);
	Shader renderShader(shaderFolderPath + "\\vert.glsl", shaderFolderPath + "\\newFrag.glsl");
	ComputeShader computeShader(shaderFolderPath + "\\compute.glsl");
	std::cout << "Shader folder path: " << shaderFolderPath << std::endl;
	renderShader.Activate();
	renderShader.setInt("tex", 5);

	int textureUnits[MAX_TEXTURES] = { 0, 1, 2, 3, 4 };
	renderShader.setIntArray("textures", textureUnits, sizeof(textureUnits));

	// Texture for the compute shader to draw on
	Texture2D screenTexture(SCR_WIDTH, SCR_HEIGHT, GL_TEXTURE5);
	glBindImageTexture(0, screenTexture.ID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
	for (int i = 0; i < textures.size(); i++)
	{
		computeShader.setInt(("texture" + std::to_string(i)).c_str(), i);
		textures[i].SetActive();
		textures[i].Bind();
	}

	// SSBOs for triangles and nodes
	SSBO trianglesSSBO(rtxTriangles.data(), sizeof(RTXTriangle) * rtxTriangles.size(), 1);
	SSBO nodesSSBO(BVH.allNodes.data(), sizeof(Node) * BVH.allNodes.size(), 2);
	SSBO materialsSSBO(materials.data(), sizeof(Material) * materials.size(), 3);

	// Set shader's constants
	computeShader.bindSSBOToBlock(trianglesSSBO, "TrianglesBlock");
	computeShader.bindSSBOToBlock(nodesSSBO, "NodesBlock");
	computeShader.bindSSBOToBlock(materialsSSBO, "MaterialsBlock");

	// Transfer uniforms with UBO
	GlobalUniforms uniforms;
	UBO UBO(3, sizeof(GlobalUniforms));

	// Create camera
	Camera camera = Camera(SCR_WIDTH, SCR_HEIGHT, maxSpeed, cameraPos, hfov, pitch, yaw, focusDistance, defocusAngle, zoom);

	// Is later used by glfwGetWindowUserPointer in glfwSetCursorPosCallback and glfwSetScrollCallback to get the camera, avoiding global variables
	glfwSetWindowUserPointer(window, &camera);
	glfwSetCursorPosCallback(window, mouseCallback);
	glfwSetScrollCallback(window, scrollCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);


	// VAO, VBO
	VAO VAO;
	VAO.Bind();
	VBO VBO(ndcVertices, sizeof(ndcVertices));
	VAO.LinkAttrib(VBO, 0, 3, GL_FLOAT, 3 * sizeof(GLfloat), (void*)0);
	VAO.Unbind();
	VBO.Unbind();

	// render loop
	// -----------
	int frameIndex = 0;
	float lastFrame = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
		// Get deltaTime
		float currentFrame = glfwGetTime();
		float deltaTime = currentFrame - lastFrame;
		if (deltaTime < SPF)
			continue;
		lastFrame = currentFrame;

		// Set window title with FPS
		std::ostringstream fps;
		fps << std::fixed << std::setprecision(2) << 1.0f / deltaTime;
		std::string title = "Demo - FPS:" + fps.str();
		glfwSetWindowTitle(window, title.c_str());

		// Keyboard input
		bool isScreenshot;
		keyBoardInput(window, camera, deltaTime, isScreenshot);

		// Screenshot
		bool terminateProgram = false;
		if (isScreenshot)
			screenshot(window, camera, VAO, UBO, uniforms, renderShader, computeShader, screenTexture, terminateProgram);

		if (terminateProgram)
			glfwSetWindowShouldClose(window, true);

		// Uniforms
		uniforms.numTextures = textures.size();
		uniforms.width = SCR_WIDTH;
		uniforms.height = SCR_HEIGHT;
		uniforms.numSpheres = 0;
		uniforms.numTriangles = rtxTriangles.size();
		uniforms.basicShading = BASIC_SHADING;
		uniforms.basicShadingShadow = BASIC_SHADING_SHADOW;
		uniforms.basicShadingLightPosition = glm::vec4(LIGHT_POSITION, 0.0f);
		uniforms.environmentalLight = BASIC_SHADING_ENVIRONMENTAL_LIGHT;
		uniforms.maxBounceCount = MAX_BOUNCE_COUNT;
		uniforms.numRaysPerPixel = numRaysPerPixel;
		uniforms.frameIndex = frameIndex;
		frameIndex++;
		// Update uniforms based on changes of position, rotation, and zooming
		camera.updateUniforms(uniforms);

		UBO.Update(&uniforms, sizeof(GlobalUniforms));

		computeShader.Activate();
		glDispatchCompute(ceil(SCR_WIDTH / WORK_SIZE_X), ceil(SCR_HEIGHT / WORK_SIZE_Y), 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		// render image to quad
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		renderShader.Activate();
		VAO.Bind();

		screenTexture.SetActive();
		screenTexture.Bind();
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	screenTexture.Delete();
	computeShader.Delete();
	renderShader.Delete();
	VAO.Delete();
	VBO.Delete();
	UBO.Delete();
	trianglesSSBO.Delete();
	nodesSSBO.Delete();
	materialsSSBO.Delete();

	for (Texture2D& tex : textures)
		tex.Delete();

	glfwTerminate();

	return EXIT_SUCCESS;
}