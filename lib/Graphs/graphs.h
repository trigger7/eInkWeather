#include <etl/map.h>
#include <Timezone.h>
#include <Adafruit_GFX.h>

#define MARGIN 14

enum graph_type {line, bar};

class Graph {

  public:
    Graph(Adafruit_GFX *gfx, const char *title, const char *parameter_a, const char *unit_a, const char *parameter_b, const char *unit_b, graph_type type_a = line, graph_type type_b = line);
    Graph(Adafruit_GFX *gfx, const char *title);

    void draw(void);

    void push_a(unsigned long timestamp, float value);
    void push_b(unsigned long timestamp, float value);

    void set_parameter_a(const char *parameter_a, const char *unit_a, graph_type type_a = line);
    void set_parameter_b(const char *parameter_b, const char *unit_b, graph_type type_b = line);
    void set_timezone(Timezone& tz);

  //protected:
    void draw_axes(void);
    void draw_axis(etl::map<unsigned long, float, 25>* _map, uint16_t color, bool right_side = false, bool draw_lines = true);
    void draw_labels(void);

    int screen_x(unsigned long timestamp);
    int screen_y(etl::map<unsigned long, float, 25>* _map, float value);

    int find_min_max_value(etl::map<unsigned long, float, 25>* _map, float &min_value, float &max_value);
    void find_min_max_timestamp(unsigned long &min_timestamp, unsigned long &max_timestamp);
    int get_x_spacing();

    void draw_line_graph(etl::map<unsigned long, float, 25>* _map, uint16_t color);
    void draw_bar_graph(etl::map<unsigned long, float, 25>* _map, uint16_t color);

    uint8_t _left_margin();
    uint8_t _right_margin();

  //private:
    Adafruit_GFX *_gfx;
    const char *_title;
    Timezone *_tz;

    etl::map<unsigned long, float, 25> _map_a;
    etl::map<unsigned long, float, 25> _map_b;

    uint8_t _top_margin = 10;
    uint8_t _bottom_margin = 18;

    bool _linked = false;

    const char *_parameter_a;
    graph_type _type_a = line;
    const char *_unit_a;
    uint16_t _color_a = 0xF800;

    const char *_parameter_b;
    graph_type _type_b = line;
    const char *_unit_b;
    uint16_t _color_b = 0x0000;
};
