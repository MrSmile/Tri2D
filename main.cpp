// main.cpp : entry point
//

#include "delaunay.h"
#include "ntl/pointer.h"
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <GL/glext.h>
#include <iostream>

using namespace std;
using namespace NTL;



int sdl_error(const char *text)
{
    cout << text << SDL_GetError() << endl;  return 1;
}

const char vert_shader[] =
    "#version 330\n"
    "\n"
    "uniform mat4 mvp;\n"
    "layout(location = 0) in vec3 pos;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    gl_Position = vec4(pos, 1) * mvp;\n"
    "}\n";

const char frag_shader[] =
    "#version 330\n"
    "\n"
    "layout(location = 0) out vec4 color;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    color = vec4(1, 1, 1, 1);\n"
    "}\n";

GLuint load_shader(GLint type, const char *src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);  return shader;
}


struct Vec3D
{
    GLfloat x, y, z;

    Vec3D &operator = (const Point *pt)
    {
        x = pt->x;  y = pt->y;  z = 0;  return *this;
    }
};

GLsizei fill_buffer(const Triangle *tr, const Point *point, size_t pt_count)
{
    const size_t n = 2 * pt_count - 2;
    Array<Vec3D> buf(3 * n);  if(!buf)return 0;

    Vec3D *pt = buf;
    for(size_t i = 0; i < n; i++, tr++)
    {
        if(tr->pt[0] && tr->pt[1] && tr < tr->next[2])
        {
            *pt++ = tr->pt[0];  *pt++ = tr->pt[1];
        }
        if(tr->pt[1] && tr->pt[2] && tr < tr->next[0])
        {
            *pt++ = tr->pt[1];  *pt++ = tr->pt[2];
        }
        if(tr->pt[2] && tr->pt[0] && tr < tr->next[1])
        {
            *pt++ = tr->pt[2];  *pt++ = tr->pt[0];
        }
    }

    size_t total = pt - buf;
    glBufferData(GL_ARRAY_BUFFER, total * sizeof(Vec3D), buf, GL_STATIC_DRAW);
    return total;
}


struct Viewport
{
    GLint mvp_id;
    int width, height;
    GLfloat scale_mul, x0, y0;
    GLfloat mat[16];

    Viewport(GLint id) : mvp_id(id), scale_mul(1.2), x0(0.5), y0(0.5),
        mat{0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1}
    {
    }

    void update()
    {
        GLfloat mul = scale_mul * sqrt(GLfloat(width) * width + GLfloat(height) * height);
        mat[0] = mul / width;   mat[3] = -x0 * mat[0];
        mat[5] = mul / height;  mat[7] = -y0 * mat[5];
        glUniformMatrix4fv(mvp_id, 1, GL_FALSE, mat);
    }

    void resize(int w, int h)
    {
        SDL_SetVideoMode(w, h, 0, SDL_HWSURFACE | SDL_GL_DOUBLEBUFFER | SDL_OPENGL | SDL_RESIZABLE);
        glViewport(0, 0, width = w, height = h);  update();
    }

    void move(int dx, int dy)
    {
        x0 -= 2 * dx / (mat[0] * width);
        y0 += 2 * dy / (mat[5] * height);
        update();
    }

    void scale(int x, int y, GLfloat mul)
    {
        x0 -= (2 * x -  width) * (1 - mul) / (mul * mat[0] * width);
        y0 += (2 * y - height) * (1 - mul) / (mul * mat[5] * height);
        scale_mul *= mul;  update();
    }
};


int main_loop(const Triangle *tr, const Point *pt, size_t pt_count)
{
    const int width = 512, height = 512;
    SDL_Surface *surface = SDL_SetVideoMode(width, height, 0, SDL_HWSURFACE | SDL_GL_DOUBLEBUFFER | SDL_OPENGL | SDL_RESIZABLE);
    if(!surface)return sdl_error("Cannot create OpenGL context: ");  SDL_WM_SetCaption("Tri2D 1.0", 0);

    GLuint prog = glCreateProgram();
    GLuint vert = load_shader(GL_VERTEX_SHADER, vert_shader);
    GLuint frag = load_shader(GL_FRAGMENT_SHADER, frag_shader);
    glAttachShader(prog, vert);  glAttachShader(prog, frag);

    glLinkProgram(prog);  glUseProgram(prog);
    glDetachShader(prog, vert);  glDetachShader(prog, frag);
    glDeleteShader(vert);  glDeleteShader(frag);

    char msg[65536];  GLsizei len;
    glGetProgramInfoLog(prog, sizeof(msg), &len, msg);
    if(len)cout << "Shader program log:\n" << msg << endl;

    GLuint buffer;  glGenBuffers(1, &buffer);  glBindBuffer(GL_ARRAY_BUFFER, buffer);  glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3D), reinterpret_cast<const GLvoid *>(0));
    GLsizei total = fill_buffer(tr, pt, pt_count);

    Viewport viewport(glGetUniformLocation(prog, "mvp"));
    viewport.resize(width, height);

    bool update = true;
    for(SDL_Event evt;;)
    {
        if(!update)SDL_WaitEvent(&evt);
        else if(!SDL_PollEvent(&evt))
        {
            glClear(GL_COLOR_BUFFER_BIT);
            glDrawArrays(GL_LINES, 0, total);
            SDL_GL_SwapBuffers();  update = false;  continue;
        }
        switch(evt.type)
        {
        case SDL_MOUSEMOTION:
            if(!(evt.motion.state & SDL_BUTTON(1)))continue;
            viewport.move(evt.motion.xrel, evt.motion.yrel);  break;

        case SDL_MOUSEBUTTONDOWN:
            switch(evt.button.button)
            {
            case SDL_BUTTON_WHEELUP:    viewport.scale(evt.button.x, evt.button.y, 1 / 0.9);  break;
            case SDL_BUTTON_WHEELDOWN:  viewport.scale(evt.button.x, evt.button.y,     0.9);  break;
            default: continue;
            }
            break;

        case SDL_VIDEORESIZE:
            viewport.resize(evt.resize.w, evt.resize.h);  break;

        case SDL_VIDEOEXPOSE:
            break;

        case SDL_QUIT:
            return 0;

        default:
            continue;
        }
        update = true;
    }
}

int main()
{
    const size_t n = 100;
    Array<Point> pt(n);  Array<Triangle> tr(2 * n - 2);
    if(!pt || !tr)return -1;

    for(size_t i = 0; i < n; i++)pt[i] = Vec2D(rand() / double(RAND_MAX), rand() / double(RAND_MAX));
    triangulate(tr, pt, n);

    if(SDL_Init(SDL_INIT_VIDEO))return sdl_error("SDL_Init failed: ");
    int res = main_loop(tr, pt, n);  SDL_Quit();  return res;
}
