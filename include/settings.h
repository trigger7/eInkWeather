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

#define API_KEY "my-openweathermap-api-key"
