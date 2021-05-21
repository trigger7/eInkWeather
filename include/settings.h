#include <Timezone.h>

TimeChangeRule daylight_saving_time = {"PDT", First, Sun, Mar, 2, -420};
TimeChangeRule standard_time = {"PST", First, Sun, Oct, 2, -480};
Timezone tz(daylight_saving_time, standard_time);

struct WifiNetwork
{
    const char* ssid;
    const char* password;
};

WifiNetwork networks[1] = {
    {"my-ssid", "my-wifi-password"}
};

struct Location
{
    const char* title;
    const char* name;
};

Location locations[1] = {
    {"Paradise", "Paradise,Us"}
};

#define API_LANG "en"
#define API_KEY "my-openweathermap-api-key"
