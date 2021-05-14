#include <etl/map.h>
#include <etl/algorithm.h>

#include <TimeLib.h>

#include <GxEPD2_GFX.h>
#include <Fonts/Picopixel.h>

#include "graphs.h"

#define SUBDIVISION 4

Graph::Graph(Adafruit_GFX *gfx, const char *parameter_a, const char *unit_a, const char *parameter_b, const char *unit_b, graph_type type_a, graph_type type_b)
{
  this->_gfx = gfx;
  this->_parameter_a = parameter_a;
  this->_type_a = type_a;
  this->_unit_a = unit_a;
  this->_parameter_b = parameter_b;
  this->_type_b = type_b;
  this->_unit_b = unit_b;
}

void Graph::draw(void)
{
  this->_gfx->fillScreen(GxEPD_WHITE);
  
  if ( this->_map_a.empty() && this->_map_b.empty() ) return;

  this->draw_axes();

  this->draw_line_graph(&this->_map_a, this->_color_a);
  this->draw_bar_graph(&this->_map_b, this->_color_b);
  //this->draw_line_graph(&this->_map_b, this->_color_b);

  this->draw_labels();
}

void Graph::draw_labels(void)
{
  this->_gfx->setTextColor(GxEPD_BLACK);
  this->_gfx->setFont(&Picopixel);

  if ( ! this->_map_a.empty() )
  {
    char label[15];
    sprintf(label, "%s (%s)", this->_parameter_a, this->_unit_a);
    int16_t tbx, tby; uint16_t tbw, tbh;
    this->_gfx->getTextBounds(label, 0, 0, &tbx, &tby, &tbw, &tbh);
    this->_gfx->setCursor(0, tbh);
    this->_gfx->print(label);
  }

  if ( ! this->_map_b.empty() )
  {
    char label[15];
    sprintf(label, "%s (%s)", this->_parameter_b, this->_unit_b);
    int16_t tbx, tby; uint16_t tbw, tbh;
    this->_gfx->getTextBounds(label, 0, 0, &tbx, &tby, &tbw, &tbh);
    this->_gfx->setCursor(this->_gfx->width() - tbw - 1, tbh);
    this->_gfx->print(label);
  }
}

void Graph::draw_axes(void)
{
  this->_gfx->setTextColor(GxEPD_BLACK);
  this->_gfx->setFont(&Picopixel);

  // Y-axis
  this->draw_axis(&this->_map_a, this->_color_a);
  this->draw_axis(&this->_map_b, this->_color_b, true, this->_map_a.empty());

  // X-axis
  auto it = this->_map_a.begin();
  while (it != this->_map_a.end())
  //int x_spacing = this->get_x_spacing();
  {
    int tick_length = 1;
    unsigned long timestamp = it->first;
    int x = this->screen_x(timestamp);

    tmElements_t date;
    breakTime(timestamp, date);

    if (date.Hour % 6 == 0 && x < this->_gfx->width() - 15)
    {
      tick_length = 2;
      this->_gfx->setCursor(x - 4, this->_gfx->height() - 8);
      this->_gfx->printf("%02dH", date.Hour);
    }
    if (date.Hour == 0 || it == this->_map_a.begin())
    {
      if (date.Hour == 0)
      {
        tick_length = 5;
      }
      this->_gfx->setCursor(x, this->_gfx->height() - 2);
      this->_gfx->printf("%02d/%02d", date.Day, date.Month);
    }

    this->_gfx->drawLine(x, this->_gfx->height() - this->_bottom_margin,
                         x, this->_gfx->height() - this->_bottom_margin + tick_length,
                         GxEPD_BLACK);
    it++;
  }
}

void Graph::push_a(unsigned long timestamp, float value)
{
  if ( !this->_map_a.full() ) this->_map_a[timestamp] = value;
}

void Graph::push_b(unsigned long timestamp, float value)
{
  if ( !this->_map_b.full() ) this->_map_b[timestamp] = value;
}

void Graph::find_min_max_timestamp(unsigned long &min_timestamp, unsigned long &max_timestamp)
{
  if ( this->_map_a.empty() && this->_map_b.empty() )
    return;
  else if ( this->_map_b.empty() ) {
    min_timestamp = this->_map_a.begin()->first;
    max_timestamp = this->_map_a.rbegin()->first;
  } else if ( this->_map_a.empty() ) {
    min_timestamp = this->_map_b.begin()->first;
    max_timestamp = this->_map_b.rbegin()->first;
  } else {
    min_timestamp = std::min(this->_map_a.begin()->first, this->_map_b.begin()->first);
    max_timestamp = std::max(this->_map_a.rbegin()->first, this->_map_b.rbegin()->first);
  }
}

int Graph::get_x_spacing()
{
  unsigned long timestamp_min, timestamp_max;
  this->find_min_max_timestamp(timestamp_min, timestamp_max);
  return (this->_gfx->width() - this->_right_margin() - this->_left_margin()) / ((timestamp_max - timestamp_min) / (3 * 3600));
}

int Graph::screen_x(unsigned long timestamp)
{
  unsigned long timestamp_min, timestamp_max;
  this->find_min_max_timestamp(timestamp_min, timestamp_max);

  return (timestamp - timestamp_min) * (this->_gfx->width() - this->_right_margin() - this->_left_margin()) / 
         (timestamp_max - timestamp_min) + this->_left_margin();
}

int Graph::screen_y(etl::map<unsigned long, float, 25>* _map, float value)
{
  float min_value, max_value;
  this->find_min_max_value(_map, min_value, max_value);
  int screen_min = this->_gfx->height() - this->_bottom_margin - 3;
  return (value - min_value) * (this->_top_margin - screen_min) / (max_value - min_value) + screen_min;
}

int Graph::find_min_max_value(etl::map<unsigned long, float, 25>* _map, float &min_value, float &max_value)
{
  auto minmax = std::minmax_element(_map->begin(), _map->end(),
                                    [](const std::pair<int, int>& p1, const std::pair<int, int>& p2) {
                                      return p1.second < p2.second; });
  int temp_min_value = floor(minmax.first->second);
  int temp_max_value = ceil(minmax.second->second);

  temp_max_value = max(temp_max_value, temp_min_value + SUBDIVISION + 1);
  int range = temp_max_value - temp_min_value;
  int step = range / SUBDIVISION;

  int isPositive = (int)(temp_max_value >= 0);
  max_value = ((temp_max_value + isPositive * (step - 1)) / step) * step;

  int isNegative = (int)(temp_min_value < 0);
  min_value = ((temp_min_value + isNegative * (step - 1)) / step) * step;

  return step;
}

uint8_t Graph::_left_margin()
{
  if ( this->_map_a.empty() ) return 0;
  return MARGIN;
}

uint8_t Graph::_right_margin()
{
  if ( this->_map_b.empty() ) return 0;
  return MARGIN;
}

void Graph::draw_axis(etl::map<unsigned long, float, 25>* _map, uint16_t color, bool right_side, bool draw_lines)
{
  if ( ! _map->empty() )
  {
    float min_value, max_value;
    int step = this->find_min_max_value(_map, min_value, max_value);
    this->_gfx->setTextColor(color);

    for (int j = min_value+step; j < max_value; j+=step)
    {
      this->_gfx->setCursor(right_side?this->_gfx->width() - this->_right_margin():0, this->screen_y(_map, j));
      this->_gfx->printf("%3d", j);
      if ( draw_lines )
      {
        this->_gfx->drawLine(this->_left_margin(), this->screen_y(_map, j),
                            this->_gfx->width() - this->_right_margin(), this->screen_y(_map, j),
                            GxEPD_BLACK);
      }
    }
  }
}

void Graph::draw_line_graph(etl::map<unsigned long, float, 25>* _map, uint16_t color)
{
  int previous_x = 0, previous_y = 0;
  auto it = _map->begin();

  while (it != _map->end())
  {
    int new_x = this->screen_x(it->first);
    int new_y = screen_y(_map, it->second);

    if (it != _map->begin()) 
    {
      this->_gfx->drawLine(previous_x, previous_y,
                           new_x, new_y,
                           color);
      this->_gfx->drawLine(previous_x, previous_y-1,
                           new_x, new_y-1,
                           color);
      this->_gfx->drawLine(previous_x, previous_y+1,
                           new_x, new_y+1,
                           color);
    }
    previous_x = new_x;
    previous_y = new_y;
    it++;
  }
}

void Graph::draw_bar_graph(etl::map<unsigned long, float, 25>* _map, uint16_t color)
{
  auto it = _map->begin();

  while (it != _map->end())
  {
    int x = this->screen_x(it->first);
    int y = screen_y(_map, it->second);

    this->_gfx->drawRect(x - 1, y, 3, this->_gfx->height() - y - this->_bottom_margin - 3, color);

    it++;
  }
}