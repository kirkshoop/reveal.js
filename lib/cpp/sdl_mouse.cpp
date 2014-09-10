///
///
/// ./emscripten/incoming/em++ -std=c++11 -stdlib=libc++ -o hello.html -O2 ./sdl_mouse.cpp
///
///

#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <assert.h>
#include <emscripten.h>

#include <cmath>

#include "../../../../source/rxcpp/Rx/v2/src/rxcpp/rx.hpp"
namespace rx=rxcpp;

float float_map(
  float value, 
  float minValue, 
  float maxValue, 
  float minResult, 
  float maxResult) {
    return minResult + (maxResult - minResult) * ((value - minValue) / (maxValue - minValue));
}

typedef std::pair<float, float> point;

point plus_point(const point& lhs, const point& rhs){
  return point(lhs.first + rhs.first, lhs.second + rhs.second);
}

SDL_Surface *screen;

rx::subjects::behavior<point> centers(point(0,0));

void render(){
    boxRGBA(screen, 0, 0, 600, 450, 0xff, 0xff, 0xff, 0xff);

    auto center = centers.get_value();
    filledEllipseRGBA(screen, center.first, center.second, 20, 20, 0, 0, 0xff, 0xff);

    SDL_UpdateRect(screen, 0, 0, 0, 0); 
}

rx::subjects::subject<SDL_MouseMotionEvent> mouse_moves;
auto mouse_move = mouse_moves.get_subscriber();
rx::subjects::subject<int> updates;
auto update = updates.get_subscriber();

void one() {

  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    switch(event.type) {
      case SDL_MOUSEMOTION: {
        SDL_MouseMotionEvent *m = (SDL_MouseMotionEvent*)&event;
        assert(m->state == 0);
        mouse_move.on_next(*m);
        break;
      }
    }
  }

  update.on_next(SDL_GetTicks());

  render();
}

void main_2(void* arg);

int main() {
  SDL_Init(SDL_INIT_VIDEO);

  screen = SDL_SetVideoMode(600, 450, 32, SDL_HWSURFACE);

  emscripten_async_call(main_2, NULL, 500); // avoid startup delays and intermittent errors

  return 0;
}

void main_2(void* arg) {

  const float orbit_period = 1.0;
  const int orbit_radius = 50;

  auto orbit_positions = updates.
    get_observable().
    map(
        [=](int ms){
            // map the tick into the range 0.0-1.0
            return float_map(ms % int(orbit_period * 1000), 0, int(orbit_period * 1000), 0.0, 1.0);
        }).
    map(
        [=](float t){
            // map the time value to a point on a circle
            return point(orbit_radius * std::cos(t * 2 * 3.14), orbit_radius * std::sin(t * 2 * 3.14));
        });

  auto mouse_positions = mouse_moves.
    get_observable().
    map(
      [](const SDL_MouseMotionEvent& m){
        return point(m.x, m.y);
      });

  mouse_positions.
    combine_latest(plus_point, orbit_positions).
    subscribe(centers.get_subscriber());

  emscripten_set_main_loop(one, 0, 0);
}

