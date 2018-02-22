#include <iostream>
#include <cstring>

#ifdef __PLATFORM_MACOSX__
  #include <OpenGL/gl3.h>
#endif
#ifdef __PLATFORM_LINUX__
  // #include <GL/gl.h>
  // #include <GL/glu.h>
  // #include <GL/glut.h>
  #include <glew/include/GL/glew.h>
#endif
#include <GLFW/include/glfw3.h>

class Mat4 {
public:
  Mat4() {
    setIdentity();
  }

  ~Mat4() {

  }

  void setIdentity() {
    memset(matrix, 0, sizeof(float) * 16);
    matrix[0]  = 1.0f;
    matrix[5]  = 1.0f;
    matrix[10] = 1.0f;
    matrix[15] = 1.0f;
  }

  void operator =(const Mat4& other) {
    matrix[0]  = other.matrix[0];  matrix[1]  = other.matrix[1];  matrix[2]  = other.matrix[2];  matrix[3]  = other.matrix[3];
    matrix[4]  = other.matrix[4];  matrix[5]  = other.matrix[5];  matrix[6]  = other.matrix[6];  matrix[7]  = other.matrix[7];
    matrix[8]  = other.matrix[8];  matrix[9]  = other.matrix[9];  matrix[10] = other.matrix[10]; matrix[11] = other.matrix[11];
    matrix[12] = other.matrix[12]; matrix[13] = other.matrix[13]; matrix[14] = other.matrix[14]; matrix[15] = other.matrix[15];
  }

  Mat4 operator *(const Mat4& other) {
    Mat4 result;
    result.matrix[0]  = matrix[0] * other.matrix[0] + matrix[1] * other.matrix[4] + matrix[2] * other.matrix[8]  + matrix[3] * other.matrix[12];
    result.matrix[1]  = matrix[0] * other.matrix[1] + matrix[1] * other.matrix[5] + matrix[2] * other.matrix[9]  + matrix[3] * other.matrix[13];
    result.matrix[2]  = matrix[0] * other.matrix[2] + matrix[1] * other.matrix[6] + matrix[2] * other.matrix[10] + matrix[3] * other.matrix[14];
    result.matrix[3]  = matrix[0] * other.matrix[3] + matrix[1] * other.matrix[7] + matrix[2] * other.matrix[11] + matrix[3] * other.matrix[15];
  
    result.matrix[4]  = matrix[4] * other.matrix[0] + matrix[5] * other.matrix[4] + matrix[6] * other.matrix[8]  + matrix[7] * other.matrix[12];
    result.matrix[5]  = matrix[4] * other.matrix[1] + matrix[5] * other.matrix[5] + matrix[6] * other.matrix[9]  + matrix[7] * other.matrix[13];
    result.matrix[6]  = matrix[4] * other.matrix[2] + matrix[5] * other.matrix[6] + matrix[6] * other.matrix[10] + matrix[7] * other.matrix[14];
    result.matrix[7]  = matrix[4] * other.matrix[3] + matrix[5] * other.matrix[7] + matrix[6] * other.matrix[11] + matrix[7] * other.matrix[15];
  
    result.matrix[8]  = matrix[8] * other.matrix[0] + matrix[9] * other.matrix[4] + matrix[10] * other.matrix[8]  + matrix[11] * other.matrix[12];
    result.matrix[9]  = matrix[8] * other.matrix[1] + matrix[9] * other.matrix[5] + matrix[10] * other.matrix[9]  + matrix[11] * other.matrix[13];
    result.matrix[10] = matrix[8] * other.matrix[2] + matrix[9] * other.matrix[6] + matrix[10] * other.matrix[10] + matrix[11] * other.matrix[14];
    result.matrix[11] = matrix[8] * other.matrix[3] + matrix[9] * other.matrix[7] + matrix[10] * other.matrix[11] + matrix[11] * other.matrix[15];
  
    result.matrix[12] = matrix[12] * other.matrix[0] + matrix[13] * other.matrix[4] + matrix[14] * other.matrix[8]  + matrix[15] * other.matrix[12];
    result.matrix[13] = matrix[12] * other.matrix[1] + matrix[13] * other.matrix[5] + matrix[14] * other.matrix[9]  + matrix[15] * other.matrix[13];
    result.matrix[14] = matrix[12] * other.matrix[2] + matrix[13] * other.matrix[6] + matrix[14] * other.matrix[10] + matrix[15] * other.matrix[14];
    result.matrix[15] = matrix[12] * other.matrix[3] + matrix[13] * other.matrix[7] + matrix[14] * other.matrix[11] + matrix[15] * other.matrix[15];
  
    return result;
  }

  void operator *=(const Mat4& other) {
    *this = *this * other;
  }


  float matrix[16];
};

static float fov = 60.0f;
static float near = 0.1f;
static float far = 1000.0f;

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }
}

static const char* vertex_shader_text = 
"#version 330 core\n"
"uniform mat4 MVP;\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec2 uv;\n"
"out vec2 o_uv;\n"
"void main() {\n"
"  gl_Position = MVP * vec4(position, 1.0);\n"
"  o_uv = uv;\n"
"}\n";

static const char* fragment_shader_text = 
"#version 330 core\n"
"in vec2 o_uv;\n"
"out vec4 color;\n"
"void main() {\n"
"  color = vec4(o_uv.x, o_uv.y, 0.0, 1.0);\n"
"}\n";

// static const char* vertex_shader_text = 
// "uniform mat4 MVP;\n"
// "attribute vec3 position;\n"
// "void main() {\n"
// "  gl_Position = MVP * vec4(position, 1.0);\n"
// "}\n";

// static const char* fragment_shader_text = 
// "void main() {\n"
// "  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
// "}\n";

bool CheckGLError(const char* tag = "") {
  GLenum error = glGetError();
  switch(error) {
    case GL_INVALID_OPERATION: {
      printf("Invalid operation: %s\n", tag);
      break;
    }
    case GL_INVALID_VALUE: {
      printf("Invalid value: %s\n", tag);
      break;
    }
    case GL_INVALID_ENUM: {
      printf("Invalid enum: %s\n", tag);
      break;
    }
    case GL_STACK_OVERFLOW: {
      printf("Stack overflow: %s\n", tag);
      break;
    }
    case GL_STACK_UNDERFLOW: {
      printf("Stack underflow: %s\n", tag);
      break;
    }
    case GL_OUT_OF_MEMORY: {
      printf("Out of memory: %s\n", tag);
      break;
    }
    // case GL_INVALID_FRAMEBUFFER_OPERATION: {
    //   printf("Invalid framebuffer operation: %s\n", tag);
    //   break;
    // }
  }

  return error != GL_NO_ERROR;
}

int main() {

  if (!glfwInit()) {
    printf("Failed to initialize GLFW\n");
    return 1;
  }

  // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  #ifdef __PLATFORM_MACOSX__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  #endif

  GLFWwindow* window = glfwCreateWindow(640, 480, "Window", NULL, NULL);
  if (!window) {
    glfwTerminate();
    printf("Failed to create window\n");
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSetKeyCallback(window, KeyCallback);

  glewInit();

  // OpenGL stuff
  GLenum error = GL_NO_ERROR;
  printf("GL_NO_ERROR code: %d\n", error);

  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  
  GLuint vertex_buffer_id;
  struct Vertex {
    float x, y, z;
  };
  Vertex vertices[6] = {
    {  0.9f,  0.9f, 0.0f },
    { -0.9f,  0.9f, 0.0f },
    { -0.9f, -0.9f, 0.0f },
    { -0.9f, -0.9f, 0.0f },
    {  0.9f, -0.9f, 0.0f },
    {  0.9f,  0.9f, 0.0f }
  };
  glGenBuffers(1, &vertex_buffer_id);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_id);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  GLuint other_vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
  CheckGLError("glCreateShader vertex 2");

  GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
  CheckGLError("glCreateShader vertex");
  glShaderSource(vertex_shader_id, 1, &vertex_shader_text, NULL);
  CheckGLError("glShaderSource vertex");
  glCompileShader(vertex_shader_id);
  CheckGLError("glCompileShader vertex");
  GLint vertex_shader_compiling_success = 0;
  glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &vertex_shader_compiling_success);
  if (!vertex_shader_compiling_success) {
    printf("Failed to compile vertex shader\n");
    GLint log_size = 0;
    glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &log_size);
    char* log = (char*)malloc(log_size);
    GLint read = 0;
    glGetShaderInfoLog(vertex_shader_id, log_size, &read, log);
    printf("Error: %s\n", log);
    free(log);
  }

  GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader_id, 1, &fragment_shader_text, NULL);
  glCompileShader(fragment_shader_id);
  CheckGLError("glCompileShader fragment");
  GLint fragment_shader_compiling_success = 0;
  glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &fragment_shader_compiling_success);
  if (!fragment_shader_compiling_success) {
    printf("Failed to compile fragment shader\n");
    GLint log_size = 0;
    glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &log_size);
    char* log = (char*)malloc(log_size);
    GLint read = 0;
    glGetShaderInfoLog(fragment_shader_id, log_size, &read, log);
    printf("Error: %s\n", log);
    free(log);
  }

  GLuint program_id = glCreateProgram();
  glAttachShader(program_id, vertex_shader_id);
  glAttachShader(program_id, fragment_shader_id);
  glLinkProgram(program_id);
  CheckGLError("glLinkProgram");

  GLint mvp_location = glGetUniformLocation(program_id, "MVP");
  GLint position_location = glGetAttribLocation(program_id, "position");

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_id);
  glEnableVertexAttribArray(position_location);
  CheckGLError("glEnableVertexAttribArray 1");
  glVertexAttribPointer(position_location, 3, GL_FLOAT, GL_FALSE, 0, 0);
  CheckGLError("glVertexAttribPointer 1");

  GLuint uvs_id;
  struct UV {
    float u, v;
  };
  UV uvs[6] = {
    {  1.0f,  1.0f },
    { -1.0f,  1.0f },
    { -1.0f, -1.0f },
    { -1.0f, -1.0f },
    {  1.0f, -1.0f },
    {  1.0f,  1.0f }
  };
  glGenBuffers(1, &uvs_id);
  glBindBuffer(GL_ARRAY_BUFFER, uvs_id);
  glBufferData(GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);

  GLint uvs_location = glGetAttribLocation(program_id, "uv");
  glEnableVertexAttribArray(uvs_location);
  CheckGLError("glEnableVertexAttribArray 2");
  glVertexAttribPointer(uvs_location, 2, GL_FLOAT, GL_FALSE, 0, 0);
  CheckGLError("glVertexAttribPointer 2");

  Mat4 m, p, mvp;
  m.setIdentity();

  float right = 1.0f;
  float left = -1.0f;
  float top = 1.0f;
  float bottom = -1.0f;

  // column-major order
  p.matrix[0] = 2.0f / (right - left);
  p.matrix[1] = 0.0f;
  p.matrix[2] = 0.0f;
  p.matrix[3] = 0.0f;

  p.matrix[4] = 0.0f;
  p.matrix[5] = 2.0f / (top - bottom);
  p.matrix[6] = 0.0f;
  p.matrix[7] = 0.0f;

  p.matrix[8]  = 0.0f;
  p.matrix[9]  = 0.0f;
  p.matrix[10] = 2.0f / (far - near);
  p.matrix[11] = 0.0f;

  p.matrix[12] = 0.0f;
  p.matrix[13] = 0.0f;
  p.matrix[14] = 0.0f;
  p.matrix[15] = 1.0f;

  glUseProgram(program_id);
  glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*)p.matrix);

  unsigned int indices[6] = { 3, 0, 1, 1, 2, 3 };
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    //glDrawElements(GL_LINES, 1, GL_UNSIGNED_INT, indices);
    //CheckGLError("glDrawArrays");
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}