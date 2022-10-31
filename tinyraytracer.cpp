#include <bits/stdint-uintn.h>
#include <sys/types.h>

#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

struct vec {
  float x = 0;
  float y = 0;
  float z = 0;
  float mag() const { return std::sqrt(x * x + y * y + z * z); }
  float sqrmag() const { return x * x + y * y + z * z; }
  vec operator*(const float v) const { return {x * v, y * v, z * v}; }
  vec operator*(const vec &v) const { return {x * v.x, y * v.y, z * v.z}; }
  vec operator+(const vec &v) const { return {x + v.x, y + v.y, z + v.z}; }
  vec operator/(const float v) const { return {x / v, y / v, z / v}; }
  vec operator/(const vec &v) const { return {x / v.x, y / v.y, z / v.z}; }
  vec operator-(const vec &v) const { return {x - v.x, y - v.y, z - v.z}; }
  vec operator-() const { return {-x, -y, -z}; }
  vec normalized() const { return (*this) / mag(); }
  float dot(const vec &v) const { return x * v.x + y * v.y + z * v.z; }
  vec cross(const vec &v) const { return {y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x}; }
  std::string str() const { return "("+std::to_string((*this).x)+", "+std::to_string((*this).y)+", "+std::to_string((*this).z)+")";}
};

uint32_t make_color(int r, int g, int b) { return r << 16 | g << 8 | b; }
u_short color_red(uint32_t enc_color) { return enc_color >> 16; }
u_short color_green(uint32_t enc_color) { return (enc_color >> 8) & 0xFF; }
u_short color_blue(uint32_t enc_color) { return enc_color & 0xFF; }

struct intersection {
  bool exists = false;
  float distance;
  vec point;
  vec normal;
  uint32_t color;
};

struct ray {
  vec start;
  vec norm_direction;
};

class Object {
 public:
  vec pos;
  uint32_t color;
  virtual intersection ray_intersect(const ray &r) {
    return intersection{false};
  };
};

class Sphere : public Object {
 public:
  Sphere(float r, vec v, uint32_t color = 0xFFFFFF) {
    (*this).radius = r;
    (*this).pos = v;
    (*this).color = color;
  }

  float radius;
  
  intersection ray_intersect(const ray &r) override {
    vec b = (*this).pos - r.start;
    vec a = r.norm_direction;
    vec proj = a * (a.dot(b) / a.sqrmag());
    float sqdist = (b-proj).sqrmag();
    if(sqdist < (*this).radius*(*this).radius){
      float dist_to_center = proj.mag();
      float dist_to_sphere = dist_to_center - std::sqrt((*this).radius * (*this).radius - sqdist);
      vec point = a * dist_to_sphere;
      vec normal = (point - b).normalized();
      return intersection{true, dist_to_sphere, point, normal, (*this).color};
    }
    return intersection{false};
  }
};

class Light : public Sphere {
 public:
  Light(float s, float r, vec v, uint32_t light_color = 0xFFFFFF) : Sphere(r, v, light_color) {
    (*this).light_color = color;
    (*this).strength = s;
  }

  uint32_t light_color;
  float strength;
};

std::vector<Object*> objects;
std::vector<Light> lights;

uint32_t cast_ray(const ray &r) {
  float closest = 99999;
  uint32_t color = 0;
  for (Object* obj : objects) {
    intersection i = (*obj).ray_intersect(r);
    if (i.exists && i.distance < closest) {
      closest = i.distance;
      color = i.color;
      color = make_color((int)(i.normal.x * 125 + 125),(int)(i.normal.y * 125 + 125),(int)(i.normal.z * 125 + 125));
    }
  }
  return color;
}

int main() {
  Sphere sp1 = Sphere(1.0f, vec{0, 0, 10});
  Sphere* p1 = &sp1;
  objects.push_back(p1);
  Sphere sp2 = Sphere(1.0f, vec{0, 1, 10}, make_color(10,20,250));
  Sphere* p2 = &sp2;
  objects.push_back(p2);

  std::cout << (*p1).ray_intersect({{0,0,0},vec{0, -1, 0}.normalized()}).exists;

  int start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  const int width = 1000;
  const int height = 1000;
  std::vector<uint32_t> img(width * height);

  const float fov = 1.05;
  const vec up = {0, 1, 0};
  vec camera_pos = {0, 0, 0};
  vec camera_fwd = {0, 0, (width / (2 * (float)tan(fov / 2.0)))};
  vec camera_right = up.cross(camera_fwd).normalized();

  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {
      //   img[x + y * width] = make_color((x) / 2, (400 - x) / 2, (y));
      vec screen_loc = (camera_pos + camera_fwd) + (camera_right * (x - width / 2)) + (up * (height / 2 - y));
        // std::cout << screen_loc.str();
      img[x + y * width] = cast_ray(ray{camera_pos, (screen_loc - camera_pos).normalized()});
    }
  }

  int end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  int time = end - start;
  float fps = 1000 / std::max(1, time);

  std::cout << "\nRendered in " + std::to_string(time) + "ms (" + std::to_string(fps) + " fps)\n\n";
  std::ofstream ofs("./out.ppm", std::ios::binary);
  ofs << "P6\n" << width << " " << height << "\n255\n";
  for (uint32_t color : img) {
    ofs << (char)color_red(color) << (char)color_green(color) << (char)color_blue(color);
  }
  return 0;
}